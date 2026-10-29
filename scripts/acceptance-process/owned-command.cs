using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using Microsoft.Win32.SafeHandles;

namespace ColourWars.Acceptance
{
    public sealed class OwnedCommandResult
    {
        public string Outcome { get; private set; }
        public int? ExitCode { get; private set; }
        public string Message { get; private set; }
        public long ElapsedMilliseconds { get; private set; }
        public int ProcessId { get; private set; }
        public bool CleanupCompleted { get; private set; }
        public string LogPath { get; private set; }
        public string RawLogPath { get; private set; }

        internal OwnedCommandResult(string outcome, int? exitCode, string message,
            long elapsedMilliseconds, int processId, bool cleanupCompleted, string logPath, string rawLogPath)
        {
            Outcome = outcome; ExitCode = exitCode; Message = message;
            ElapsedMilliseconds = elapsedMilliseconds; ProcessId = processId;
            CleanupCompleted = cleanupCompleted; LogPath = logPath; RawLogPath = rawLogPath;
        }
    }

    public sealed class OwnedRunControl : IDisposable
    {
        private readonly object gate = new object();
        private readonly List<OwnedCommand> commands = new List<OwnedCommand>();
        private int cancelled;

        public bool IsCancellationRequested { get { return Volatile.Read(ref cancelled) != 0; } }

        internal bool Register(OwnedCommand command)
        {
            lock (gate)
            {
                if (IsCancellationRequested) return false;
                commands.Add(command);
                return true;
            }
        }

        internal void Unregister(OwnedCommand command) { lock (gate) { commands.Remove(command); } }

        public void Cancel()
        {
            OwnedCommand[] owned;
            lock (gate)
            {
                if (Interlocked.Exchange(ref cancelled, 1) != 0) return;
                owned = commands.ToArray();
            }
            // Request every stop before waiting for any cleanup. A busy Poll or
            // launch observes the flag itself; Cancel never waits on its lock.
            foreach (OwnedCommand command in owned) command.RequestCancellation();
        }

        public void Dispose()
        {
            Cancel();
            OwnedCommand[] owned;
            lock (gate) { owned = commands.ToArray(); }
            long deadline = OwnedCommand.DeadlineAfter(3000);
            foreach (OwnedCommand command in owned) command.DisposeUntil(deadline);
        }
    }

    // Probe executables only. In particular this must not contain CL/MSPDB or
    // CMake: their shared child-process ownership has not been established.
    public sealed class OwnedCommand : IDisposable
    {
        private const int CleanupMilliseconds = 2000;
        private const uint ForcedExitCode = 0xC000013A;
        private readonly object gate = new object();
        private readonly OwnedRunControl control;
        private readonly string logPath, rawLogPath;
        private readonly Encoding encoding;
        private readonly long timeoutMilliseconds;
        private readonly Stopwatch clock = new Stopwatch();
        private readonly char[] characters = new char[16384];
        private Native.KernelHandle job, process, thread;
        private StreamReader reader;
        private StreamWriter writer;
        private OwnedCommandResult result;
        private string outcome, message = "";
        private int? exitCode;
        private int processId, cancellationRequested;
        private long cleanupStarted;
        private bool registered, assigned, stopIssued, nativeFinished, cleanupCompleted;
        private bool rawCreated, cancelControl;

        private OwnedCommand(string path, int codePage, int timeoutSeconds, OwnedRunControl runControl)
        {
            logPath = Path.GetFullPath(path);
            rawLogPath = logPath + ".raw";
            encoding = Encoding.GetEncoding(codePage);
            timeoutMilliseconds = (long)timeoutSeconds * 1000;
            control = runControl;
        }

        public OwnedCommandResult Result { get { lock (gate) { return result; } } }

        public static OwnedCommand Start(string exe, string[] args, string workingDir, string logPath,
            int encodingCodePage, int timeoutSeconds, OwnedRunControl control)
        {
            if (String.IsNullOrWhiteSpace(exe)) throw new ArgumentException("An executable path is required.", "exe");
            if (exe.IndexOf('\0') >= 0) throw new ArgumentException("Executable paths cannot contain NUL.", "exe");
            if (args == null) throw new ArgumentNullException("args");
            foreach (string argument in args)
                if (argument == null || argument.IndexOf('\0') >= 0) throw new ArgumentException("Arguments must be non-null and contain no NUL.", "args");
            if (String.IsNullOrWhiteSpace(workingDir)) throw new ArgumentException("A working directory is required.", "workingDir");
            if (String.IsNullOrWhiteSpace(logPath)) throw new ArgumentException("A log path is required.", "logPath");
            if (timeoutSeconds < 1) throw new ArgumentOutOfRangeException("timeoutSeconds");
            if (control == null) throw new ArgumentNullException("control");
            string executable = Path.GetFullPath(exe);
            string directory = Path.GetFullPath(workingDir);
            OwnedCommand command = new OwnedCommand(logPath, encodingCodePage, timeoutSeconds, control);
            lock (command.gate)
            {
                command.registered = control.Register(command);
                command.Launch(executable, args, directory);
            }
            command.CancelControlIfNeeded();
            return command;
        }

        private bool CancellationRequested
        {
            get { return Volatile.Read(ref cancellationRequested) != 0 || control.IsCancellationRequested; }
        }

        private void Launch(string executable, string[] args, string directory)
        {
            Native.KernelHandle output = null, input = null;
            try
            {
                Directory.CreateDirectory(Path.GetDirectoryName(logPath));
                File.WriteAllText(logPath, "", new UTF8Encoding(false));
                Native.SecurityAttributes security = new Native.SecurityAttributes();
                security.Length = Marshal.SizeOf(typeof(Native.SecurityAttributes));
                security.InheritHandle = 1;
                output = Native.CreateFileW(rawLogPath, 0x40000000, 7, ref security, 2, 0x80, IntPtr.Zero);
                Native.Require(output != null && !output.IsInvalid, "Create output sidecar", false);
                rawCreated = true;
                if (CancellationRequested)
                {
                    outcome = "cancelled"; message = "Probe launch skipped because this run was cancelled.";
                    FinishNative(true);
                    return;
                }
                job = Native.CreateJobObjectW(IntPtr.Zero, null);
                Native.Require(job != null && !job.IsInvalid, "Create private job", true);
                Native.JobExtendedLimits limits = new Native.JobExtendedLimits();
                limits.Basic.LimitFlags = 0x00002000; // JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
                Native.Require(Native.SetInformationJobObject(job, 9, ref limits,
                    (uint)Marshal.SizeOf(typeof(Native.JobExtendedLimits))), "Set private job limits", true);
                input = Native.CreateFileW("NUL", 0x80000000, 3, ref security, 3, 0x80, IntPtr.Zero);
                Native.Require(input != null && !input.IsInvalid, "Open probe stdin", false);
                CreateSuspended(executable, args, directory, output, input);
                Native.Require(Native.AssignProcessToJobObject(job, process), "Assign suspended probe to private job", true);
                assigned = true;
                if (CancellationRequested) BeginStop("cancelled", "Probe cancelled before its first instruction.");
                else if (clock.ElapsedMilliseconds >= timeoutMilliseconds) BeginStop("timeout", "Probe launch exceeded its runtime deadline.");
                else
                {
                    uint suspended = Native.ResumeThread(thread);
                    if (suspended == 0xFFFFFFFF) throw Native.LastFailure("Resume contained probe", true);
                    if (suspended != 1) throw new InvalidOperationException("Contained probe had an unexpected suspension count.");
                }
            }
            catch (Exception exception)
            {
                Native.Failure failure = exception as Native.Failure;
                if (failure != null && failure.Containment && failure.Error == 5) cancelControl = true;
                BeginStop("launch_failed", exception.Message);
            }
            finally
            {
                if (thread != null) { thread.Dispose(); thread = null; }
                if (output != null) output.Dispose();
                if (input != null) input.Dispose();
            }
            PollCore(0);
        }

        private void CreateSuspended(string executable, string[] args, string directory,
            Native.KernelHandle output, Native.KernelHandle input)
        {
            IntPtr size = IntPtr.Zero, attributes = IntPtr.Zero, handles = IntPtr.Zero;
            bool initialized = false;
            try
            {
                Native.InitializeProcThreadAttributeList(IntPtr.Zero, 1, 0, ref size);
                if (size == IntPtr.Zero) throw Native.LastFailure("Size inherited-handle list", true);
                attributes = Marshal.AllocHGlobal(size);
                Native.Require(Native.InitializeProcThreadAttributeList(attributes, 1, 0, ref size), "Initialize inherited-handle list", true);
                initialized = true;
                handles = Marshal.AllocHGlobal(IntPtr.Size * 2);
                Marshal.WriteIntPtr(handles, 0, output.DangerousGetHandle());
                Marshal.WriteIntPtr(handles, IntPtr.Size, input.DangerousGetHandle());
                Native.Require(Native.UpdateProcThreadAttribute(attributes, 0, new IntPtr(0x00020002),
                    handles, new IntPtr(IntPtr.Size * 2), IntPtr.Zero, IntPtr.Zero), "Set inherited-handle list", true);
                Native.StartupInfoEx startup = new Native.StartupInfoEx();
                startup.Startup.Size = Marshal.SizeOf(typeof(Native.StartupInfoEx));
                startup.Startup.Flags = 0x00000101; // STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW
                startup.Startup.ShowWindow = 0;
                startup.Startup.StandardOutput = output.DangerousGetHandle();
                startup.Startup.StandardError = output.DangerousGetHandle();
                startup.Startup.StandardInput = input.DangerousGetHandle();
                startup.Attributes = attributes;
                StringBuilder commandLine = new StringBuilder(QuoteArgument(executable));
                foreach (string argument in args) commandLine.Append(' ').Append(QuoteArgument(argument));
                if (commandLine.Length >= 32767) throw new ArgumentException("Probe command line exceeds the Windows limit.");
                // Allocate wrappers before the native call. Its returned raw
                // handles transfer in finally even if a PS5 pipeline abort
                // arrives between CreateProcess returning and managed code.
                process = new Native.KernelHandle();
                thread = new Native.KernelHandle();
                Native.ProcessInformation created = new Native.ProcessInformation();
                bool launched = false;
                int launchError = 0;
                clock.Start(); // No queued-runspace or compiler time is charged here.
                try
                {
                    launched = Native.CreateProcessW(executable, commandLine, IntPtr.Zero, IntPtr.Zero, true,
                        0x08080404, IntPtr.Zero, directory, ref startup, out created);
                    if (!launched) launchError = Marshal.GetLastWin32Error();
                }
                finally
                {
                    if (created.Process != IntPtr.Zero) process.Attach(created.Process);
                    if (created.Thread != IntPtr.Zero) thread.Attach(created.Thread);
                    processId = unchecked((int)created.ProcessId);
                }
                if (!launched) throw new Native.Failure("Create suspended probe", launchError, false);
            }
            finally
            {
                if (initialized) Native.DeleteProcThreadAttributeList(attributes);
                if (attributes != IntPtr.Zero) Marshal.FreeHGlobal(attributes);
                if (handles != IntPtr.Zero) Marshal.FreeHGlobal(handles);
                GC.KeepAlive(output); GC.KeepAlive(input);
            }
        }

        // The CRT/wmain quoting rule: double backslashes before quotes and at
        // the closing quote. Always quoting also preserves empty arguments.
        private static string QuoteArgument(string value)
        {
            StringBuilder quoted = new StringBuilder("\"");
            int slashes = 0;
            foreach (char character in value)
            {
                if (character == '\\') { ++slashes; continue; }
                if (character == '"') quoted.Append('\\', slashes * 2 + 1).Append('"');
                else quoted.Append('\\', slashes).Append(character);
                slashes = 0;
            }
            return quoted.Append('\\', slashes * 2).Append('"').ToString();
        }

        public bool Poll(int waitMs)
        {
            if (waitMs < 0 || waitMs > 100) throw new ArgumentOutOfRangeException("waitMs", "Poll waits must be between 0 and 100 ms.");
            bool complete;
            lock (gate) { PollCore(waitMs); complete = result != null; }
            CancelControlIfNeeded();
            return complete;
        }

        private void CancelControlIfNeeded()
        {
            bool cancel;
            lock (gate) { cancel = cancelControl; cancelControl = false; }
            if (cancel) control.Cancel();
        }

        internal void RequestCancellation()
        {
            Interlocked.Exchange(ref cancellationRequested, 1);
            if (!Monitor.TryEnter(gate)) return;
            try { if (result == null && !nativeFinished) BeginStop("cancelled", "Probe cancelled."); }
            finally { Monitor.Exit(gate); }
        }

        private void PollCore(int waitMs)
        {
            long pollStarted = Stopwatch.GetTimestamp();
            try { PollNativeAndLog(waitMs, pollStarted); }
            catch (Exception exception)
            {
                Native.Failure failure = exception as Native.Failure;
                if (failure != null && failure.Containment && failure.Error == 5) cancelControl = true;
                AppendMessage(exception.Message); outcome = "cleanup_uncertain";
                FinishNative(false); PumpLog();
            }
        }

        private void PollNativeAndLog(int waitMs, long pollStarted)
        {
            if (result != null) return;
            if (nativeFinished) { PumpLog(); return; }
            if (!stopIssued)
            {
                if (CancellationRequested) BeginStop("cancelled", "Probe cancelled.");
                else if (clock.ElapsedMilliseconds >= timeoutMilliseconds) BeginStop("timeout", "Probe exceeded its runtime deadline.");
                else
                {
                    int wait = (int)Math.Min(waitMs, timeoutMilliseconds - clock.ElapsedMilliseconds);
                    uint state = Native.WaitForSingleObject(process, (uint)Math.Max(0, wait));
                    if (state == 0) { CaptureExit(); BeginStop("exited", ""); }
                    else if (state == 0xFFFFFFFF) BeginStop("cleanup_uncertain", Native.LastFailure("Wait for owned probe", true).Message);
                    else if (CancellationRequested) BeginStop("cancelled", "Probe cancelled.");
                    else if (clock.ElapsedMilliseconds >= timeoutMilliseconds) BeginStop("timeout", "Probe exceeded its runtime deadline.");
                }
            }
            if (stopIssued && !nativeFinished)
            {
                if (!InspectCleanup() && waitMs > 0)
                {
                    int remaining = (int)Math.Max(0, CleanupMilliseconds - Since(cleanupStarted));
                    int pollRemaining = Math.Max(0, waitMs - (int)Since(pollStarted));
                    Thread.Sleep(Math.Min(pollRemaining, remaining));
                    InspectCleanup();
                }
            }
            if (nativeFinished) PumpLog();
        }

        private void BeginStop(string reason, string detail)
        {
            if (outcome == null) { outcome = reason; message = detail; }
            if (stopIssued || nativeFinished) return;
            stopIssued = true; cleanupStarted = Stopwatch.GetTimestamp();
            if (!assigned && (process == null || process.IsInvalid)) { FinishNative(true); return; }
            bool stopped = assigned ? Native.TerminateJobObject(job, ForcedExitCode) : Native.TerminateProcess(process, ForcedExitCode);
            if (!stopped)
            {
                Native.Failure failure = Native.LastFailure(assigned ? "Terminate private probe job" : "Terminate known suspended probe", true);
                if (failure.Error == 5) cancelControl = true;
                AppendMessage(failure.Message);
                outcome = "cleanup_uncertain";
                // Closing this private kill-on-close job is ordinary handle
                // disposal, not a fallback launch or a second termination API.
                FinishNative(false);
            }
        }

        private void CaptureExit()
        {
            if (process == null || process.IsInvalid) return;
            uint code;
            if (!Native.GetExitCodeProcess(process, out code))
                throw Native.LastFailure("Read owned probe exit code", false);
            exitCode = unchecked((int)code);
            process.Dispose(); process = null;
        }

        private bool InspectCleanup()
        {
            if (nativeFinished) return true;
            try
            {
                bool primaryDone = process == null || process.IsInvalid;
                if (!primaryDone)
                {
                    uint state = Native.WaitForSingleObject(process, 0);
                    if (state == 0) { CaptureExit(); primaryDone = true; }
                    else if (state == 0xFFFFFFFF) throw Native.LastFailure("Wait for owned probe cleanup", true);
                }
                bool empty = !assigned;
                if (assigned)
                {
                    Native.JobAccounting accounting;
                    Native.Require(Native.QueryInformationJobObject(job, 1, out accounting,
                        (uint)Marshal.SizeOf(typeof(Native.JobAccounting)), IntPtr.Zero), "Inspect private probe job cleanup", true);
                    empty = accounting.ActiveProcesses == 0;
                }
                if (primaryDone && empty) { FinishNative(true); return true; }
                if (Since(cleanupStarted) >= CleanupMilliseconds)
                {
                    AppendMessage("Owned process/job cleanup did not finish within 2000 ms.");
                    outcome = "cleanup_uncertain"; FinishNative(false); return true;
                }
            }
            catch (Exception exception)
            {
                Native.Failure failure = exception as Native.Failure;
                if (failure != null && failure.Containment && failure.Error == 5) cancelControl = true;
                AppendMessage(exception.Message); outcome = "cleanup_uncertain";
                FinishNative(false); return true;
            }
            return false;
        }

        private void FinishNative(bool confirmed)
        {
            nativeFinished = true; cleanupCompleted = confirmed;
            if (job != null) { job.Dispose(); job = null; }
            if (process != null) { process.Dispose(); process = null; }
            if (thread != null) { thread.Dispose(); thread = null; }
        }

        private void PumpLog()
        {
            try
            {
                if (reader == null)
                {
                    if (!rawCreated) { FinishResult(); return; }
                    reader = new StreamReader(new FileStream(rawLogPath, FileMode.Open, FileAccess.Read,
                        FileShare.ReadWrite | FileShare.Delete), encoding, false);
                    writer = new StreamWriter(logPath, false, new UTF8Encoding(false));
                }
                // Bound each managed call's decoding work, retaining the raw
                // sidecar even if a caller stops between Poll invocations.
                int count = reader.Read(characters, 0, characters.Length);
                if (count != 0) writer.Write(characters, 0, count);
                else { CloseLog(); FinishResult(); }
            }
            catch (Exception exception)
            {
                AppendMessage("Output log failed: " + exception.Message);
                if (outcome == "exited") outcome = "output_failed";
                CloseLog(); FinishResult();
            }
        }

        private void CloseLog()
        {
            StreamReader closingReader = reader; reader = null;
            StreamWriter closingWriter = writer; writer = null;
            try { if (closingReader != null) closingReader.Dispose(); }
            catch (Exception exception) { LogCloseFailed(exception); }
            try { if (closingWriter != null) closingWriter.Dispose(); }
            catch (Exception exception) { LogCloseFailed(exception); }
        }

        private void LogCloseFailed(Exception exception)
        {
            AppendMessage("Output log close failed: " + exception.Message);
            if (outcome == "exited") outcome = "output_failed";
        }

        private void FinishResult()
        {
            if (result != null) return;
            clock.Stop();
            result = new OwnedCommandResult(outcome, exitCode, message, clock.ElapsedMilliseconds,
                processId, cleanupCompleted, logPath, rawLogPath);
            if (registered) { control.Unregister(this); registered = false; }
        }

        private void AppendMessage(string detail) { message = String.IsNullOrEmpty(message) ? detail : message + " " + detail; }
        internal static long DeadlineAfter(int milliseconds) { return Stopwatch.GetTimestamp() + (long)(Stopwatch.Frequency * (milliseconds / 1000.0)); }
        private static long Since(long timestamp) { return (long)((Stopwatch.GetTimestamp() - timestamp) * (1000.0 / Stopwatch.Frequency)); }
        private static int Remaining(long deadline) { return (int)Math.Max(0, Math.Min(Int32.MaxValue, (deadline - Stopwatch.GetTimestamp()) * (1000.0 / Stopwatch.Frequency))); }

        public void Dispose() { DisposeUntil(DeadlineAfter(3000)); CancelControlIfNeeded(); }

        internal void DisposeUntil(long deadline)
        {
            Interlocked.Exchange(ref cancellationRequested, 1);
            if (!Monitor.TryEnter(gate, Remaining(deadline))) return;
            try
            {
                if (result != null) return;
                if (!nativeFinished) BeginStop("cancelled", "Probe disposed before completion.");
                PollCore(0);
                while (result == null && Remaining(deadline) > 0) PollCore(Math.Min(50, Remaining(deadline)));
                if (result == null)
                {
                    if (!nativeFinished)
                    {
                        AppendMessage("Shared cleanup budget expired before owned termination was confirmed.");
                        outcome = "cleanup_uncertain"; FinishNative(false);
                    }
                    else
                    {
                        AppendMessage("Output conversion exceeded the disposal budget; raw output was retained.");
                        if (outcome == "exited") outcome = "output_failed";
                    }
                    CloseLog(); FinishResult();
                }
            }
            finally { Monitor.Exit(gate); }
        }
    }

    internal static class Native
    {
        internal sealed class Failure : Win32Exception
        {
            internal readonly bool Containment;
            internal readonly int Error;
            internal Failure(string operation, int error, bool containment)
                : base(error, operation + " failed (" + error + "): " + new Win32Exception(error).Message)
            { Error = error; Containment = containment; }
        }
        internal static Failure LastFailure(string operation, bool containment) { return new Failure(operation, Marshal.GetLastWin32Error(), containment); }
        internal static void Require(bool succeeded, string operation, bool containment) { if (!succeeded) throw LastFailure(operation, containment); }

        internal sealed class KernelHandle : SafeHandleZeroOrMinusOneIsInvalid
        {
            public KernelHandle() : base(true) { }
            internal void Attach(IntPtr value) { SetHandle(value); }
            protected override bool ReleaseHandle() { return CloseHandle(handle); }
        }
        [StructLayout(LayoutKind.Sequential)]
        internal struct SecurityAttributes { public int Length; public IntPtr Descriptor; public int InheritHandle; }
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        internal struct StartupInfo
        {
            public int Size;
            public IntPtr Reserved, Desktop, Title;
            public uint X, Y, Width, Height, CharacterWidth, CharacterHeight, FillAttribute, Flags;
            public short ShowWindow, ReservedSize;
            public IntPtr ReservedBytes, StandardInput, StandardOutput, StandardError;
        }
        [StructLayout(LayoutKind.Sequential)]
        internal struct StartupInfoEx { public StartupInfo Startup; public IntPtr Attributes; }
        [StructLayout(LayoutKind.Sequential)]
        internal struct ProcessInformation { public IntPtr Process, Thread; public uint ProcessId, ThreadId; }
        [StructLayout(LayoutKind.Sequential)]
        internal struct JobBasicLimits
        {
            public long ProcessTime, JobTime;
            public uint LimitFlags;
            public UIntPtr MinimumWorkingSet, MaximumWorkingSet;
            public uint ActiveProcessLimit;
            public UIntPtr Affinity;
            public uint PriorityClass, SchedulingClass;
        }
        [StructLayout(LayoutKind.Sequential)]
        internal struct IoCounters { public ulong ReadOperations, WriteOperations, OtherOperations, ReadBytes, WriteBytes, OtherBytes; }
        [StructLayout(LayoutKind.Sequential)]
        internal struct JobExtendedLimits
        {
            public JobBasicLimits Basic;
            public IoCounters Io;
            public UIntPtr ProcessMemory, JobMemory, PeakProcessMemory, PeakJobMemory;
        }
        [StructLayout(LayoutKind.Sequential)]
        internal struct JobAccounting
        {
            public long TotalUserTime, TotalKernelTime, PeriodUserTime, PeriodKernelTime;
            public uint PageFaults, TotalProcesses, ActiveProcesses, TerminatedProcesses;
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool CloseHandle(IntPtr handle);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        internal static extern KernelHandle CreateFileW(string path, uint access, uint share,
            ref SecurityAttributes security, uint creation, uint flags, IntPtr template);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        internal static extern KernelHandle CreateJobObjectW(IntPtr security, string name);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool SetInformationJobObject(KernelHandle job, int kind, ref JobExtendedLimits limits, uint size);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool QueryInformationJobObject(KernelHandle job, int kind, out JobAccounting information, uint size, IntPtr returned);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool AssignProcessToJobObject(KernelHandle job, KernelHandle process);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool TerminateJobObject(KernelHandle job, uint exitCode);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool TerminateProcess(KernelHandle process, uint exitCode);
        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern uint WaitForSingleObject(KernelHandle handle, uint milliseconds);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool GetExitCodeProcess(KernelHandle process, out uint exitCode);
        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern uint ResumeThread(KernelHandle thread);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool InitializeProcThreadAttributeList(IntPtr list, int count, int flags, ref IntPtr bytes);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool UpdateProcThreadAttribute(IntPtr list, uint flags, IntPtr attribute,
            IntPtr value, IntPtr bytes, IntPtr previous, IntPtr returned);
        [DllImport("kernel32.dll")]
        internal static extern void DeleteProcThreadAttributeList(IntPtr list);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool CreateProcessW(string application, StringBuilder commandLine,
            IntPtr processSecurity, IntPtr threadSecurity, [MarshalAs(UnmanagedType.Bool)] bool inherit,
            uint flags, IntPtr environment, string directory, ref StartupInfoEx startup, out ProcessInformation information);
    }
}
