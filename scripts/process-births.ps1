# Dot-sourced by the selection checkers. Counts every process this host and
# its descendants start, so a receipt can say "no CMake, compiler or probe
# started" from a measurement rather than a constant.
#
# Start-ProcessBirthCount puts this host in a private Windows Job Object (jobs
# nest on Windows 8 and later). A job counts every process ever assigned to it,
# and children join their parent's job when they start, so Get-ProcessBirthCount
# is the number started since. Compile the type first: Windows PowerShell's
# Add-Type itself starts csc.exe, which would otherwise be counted.
# Get-ProcessBirthLiving lists the job's live processes, for a check that
# starts a console host on purpose and must tell its conhost.exe apart.

if (-not ('ColourWars.Checks.ProcessBirths' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace ColourWars.Checks
{
    public static class ProcessBirths
    {
        [StructLayout(LayoutKind.Sequential)]
        private struct BasicAccounting
        {
            public long TotalUserTime;
            public long TotalKernelTime;
            public long ThisPeriodTotalUserTime;
            public long ThisPeriodTotalKernelTime;
            public uint TotalPageFaultCount;
            public uint TotalProcesses;
            public uint ActiveProcesses;
            public uint TotalTerminatedProcesses;
        }

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern IntPtr CreateJobObject(IntPtr attributes, string name);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool AssignProcessToJobObject(IntPtr job, IntPtr process);
        [DllImport("kernel32.dll")]
        private static extern IntPtr GetCurrentProcess();
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool QueryInformationJobObject(IntPtr job, int informationClass,
            out BasicAccounting information, int length, IntPtr returned);
        [DllImport("kernel32.dll", SetLastError = true, EntryPoint = "QueryInformationJobObject")]
        private static extern bool QueryProcessIds(IntPtr job, int informationClass,
            IntPtr information, int length, IntPtr returned);

        private static IntPtr job = IntPtr.Zero;

        // Idempotent: the host joins one job for its lifetime.
        public static void Start()
        {
            if (job != IntPtr.Zero) return;
            IntPtr created = CreateJobObject(IntPtr.Zero, null);
            if (created == IntPtr.Zero) throw new Win32Exception(Marshal.GetLastWin32Error());
            if (!AssignProcessToJobObject(created, GetCurrentProcess()))
                throw new Win32Exception(Marshal.GetLastWin32Error());
            job = created;
        }

        // Processes started since Start, not counting this host.
        public static long Count()
        {
            if (job == IntPtr.Zero) throw new InvalidOperationException("Process birth count was not started.");
            BasicAccounting information;
            if (!QueryInformationJobObject(job, 1, out information, Marshal.SizeOf(typeof(BasicAccounting)), IntPtr.Zero))
                throw new Win32Exception(Marshal.GetLastWin32Error());
            return (long)information.TotalProcesses - 1;
        }

        // Ids of the job's live processes, this host included.
        public static long[] Living()
        {
            if (job == IntPtr.Zero) throw new InvalidOperationException("Process birth count was not started.");
            const int capacity = 256;
            int length = 8 + capacity * IntPtr.Size;
            IntPtr buffer = Marshal.AllocHGlobal(length);
            try
            {
                if (!QueryProcessIds(job, 3, buffer, length, IntPtr.Zero))
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                int listed = Marshal.ReadInt32(buffer, 4);
                var ids = new long[listed];
                for (int index = 0; index < listed; ++index)
                    ids[index] = Marshal.ReadIntPtr(buffer, 8 + index * IntPtr.Size).ToInt64();
                return ids;
            }
            finally { Marshal.FreeHGlobal(buffer); }
        }
    }
}
'@
}

function Start-ProcessBirthCount { [ColourWars.Checks.ProcessBirths]::Start() }
function Get-ProcessBirthCount { return [ColourWars.Checks.ProcessBirths]::Count() }
function Get-ProcessBirthLiving { return [long[]][ColourWars.Checks.ProcessBirths]::Living() }
