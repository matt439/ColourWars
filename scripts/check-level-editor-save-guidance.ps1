# Dot-sourced by check-level-editor.ps1 after its ordinary hidden-window checks.
# All commands go to the probe's owned hidden windows; no global keyboard,
# mouse, foreground or focus operations are used.
function Test-EditorSaveGuidance {
    param([string] $Executable, [string] $Level, [string] $RunDirectory, [string] $SourceLevel)

    if (-not ('ColourWars.EditorSaveProbe' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace ColourWars
{
    public static class EditorSaveProbe
    {
        private delegate bool EnumWindowProc(IntPtr window, IntPtr parameter);
        private static readonly HashSet<IntPtr> hiddenDialogs = new HashSet<IntPtr>();
        [DllImport("user32.dll")]
        private static extern bool EnumWindows(EnumWindowProc callback, IntPtr parameter);
        [DllImport("user32.dll")]
        private static extern bool EnumChildWindows(IntPtr parent, EnumWindowProc callback, IntPtr parameter);
        [DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetClassName(IntPtr window, StringBuilder text, int maximum);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetWindowTextW(IntPtr window, StringBuilder text, int maximum);
        [DllImport("user32.dll")]
        private static extern IntPtr GetWindow(IntPtr window, uint command);
        [DllImport("user32.dll")]
        private static extern IntPtr GetParent(IntPtr window);
        [DllImport("user32.dll")]
        private static extern int GetDlgCtrlID(IntPtr window);
        [DllImport("user32.dll")]
        private static extern bool IsWindowEnabled(IntPtr window);
        [DllImport("user32.dll")]
        private static extern bool IsWindowVisible(IntPtr window);
        [DllImport("user32.dll")]
        private static extern bool ShowWindow(IntPtr window, int show);
        [DllImport("user32.dll")]
        private static extern bool PostMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
        [DllImport("user32.dll")]
        private static extern IntPtr SendMessageTimeout(IntPtr window, uint message,
            IntPtr wParam, IntPtr lParam, uint flags, uint timeout, out IntPtr result);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr SendMessageTimeout(IntPtr window, uint message,
            IntPtr wParam, StringBuilder lParam, uint flags, uint timeout, out IntPtr result);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr SendMessageTimeout(IntPtr window, uint message,
            IntPtr wParam, string lParam, uint flags, uint timeout, out IntPtr result);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr FindWindowEx(IntPtr parent, IntPtr after, string name, string title);
        [DllImport("user32.dll")]
        private static extern IntPtr GetMenu(IntPtr window);
        [DllImport("user32.dll")]
        private static extern IntPtr GetSubMenu(IntPtr menu, int index);
        [DllImport("user32.dll")]
        private static extern uint GetMenuItemID(IntPtr menu, int index);

        private static string ClassName(IntPtr window)
        {
            var text = new StringBuilder(256);
            GetClassName(window, text, text.Capacity);
            return text.ToString();
        }
        private static void Send(IntPtr window, uint message, IntPtr wparam, IntPtr lparam)
        {
            IntPtr result;
            if (SendMessageTimeout(window, message, wparam, lparam, 2, 2000, out result) == IntPtr.Zero)
                throw new InvalidOperationException("Owned editor command timed out");
        }
        public static void MenuCommand(IntPtr window, int menu, int item, bool asynchronous)
        {
            uint command = GetMenuItemID(GetSubMenu(GetMenu(window), menu), item);
            if (command == 0xffffffff) throw new InvalidOperationException("Editor menu command missing");
            if (asynchronous)
            {
                // Match a queued menu command; a sent message can constrain the
                // common dialog's Shell calls. Check completion through each
                // operation's observable postconditions after its modal closes.
                hiddenDialogs.Clear();
                if (!PostMessage(window, 0x0111, new IntPtr(command), IntPtr.Zero))
                    throw new InvalidOperationException("Could not queue owned editor command");
            }
            else Send(window, 0x0111, new IntPtr(command), IntPtr.Zero);
        }
        public static string Describe(int processId, IntPtr root)
        {
            var result = new StringBuilder("main-enabled=" + IsWindowEnabled(root));
            EnumWindows((window, parameter) => {
                uint owner;
                GetWindowThreadProcessId(window, out owner);
                if (owner != processId || ClassName(window) != "#32770") return true;
                var title = new StringBuilder(1024);
                GetWindowTextW(window, title, title.Capacity);
                result.Append(";dialog=" + window.ToInt64().ToString("x") + ":" + title +
                    ":owner=" + GetWindow(window, 4).ToInt64().ToString("x") +
                    ":visible=" + IsWindowVisible(window) + ":enabled=" + IsWindowEnabled(window));
                EnumChildWindows(window, (child, unused) => {
                    string name = ClassName(child);
                    if (name == "Edit" || name == "ComboBox" || name == "ComboBoxEx32" || name == "Button")
                        result.Append(";control=" + name + ":" + GetDlgCtrlID(child) +
                            ":parent=" + ClassName(GetParent(child)) + ":" + GetDlgCtrlID(GetParent(child)) +
                            ":visible=" + IsWindowVisible(child) + ":enabled=" + IsWindowEnabled(child));
                    return true;
                }, IntPtr.Zero);
                return true;
            }, IntPtr.Zero);
            return result.ToString();
        }
        private static bool OwnedBy(IntPtr window, IntPtr root)
        {
            for (int depth = 0; window != IntPtr.Zero && depth < 32; ++depth) {
                window = GetWindow(window, 4);
                if (window == root) return true;
            }
            return false;
        }
        public static IntPtr Dialog(int processId, IntPtr root)
        {
            IntPtr found = IntPtr.Zero;
            EnumWindows((window, parameter) => {
                uint owner;
                GetWindowThreadProcessId(window, out owner);
                if (owner != processId || ClassName(window) != "#32770" || !OwnedBy(window, root)) return true;
                // The Shell keeps hidden dialog templates in this process.
                // Only track a modal that became visible under our editor;
                // keep recognizing it after this probe hides it.
                if (IsWindowVisible(window)) {
                    hiddenDialogs.Add(window);
                    ShowWindow(window, 0);
                }
                if (hiddenDialogs.Contains(window) &&
                    (found == IntPtr.Zero || (!IsWindowEnabled(found) && IsWindowEnabled(window)))) found = window;
                return true;
            }, IntPtr.Zero);
            return found;
        }
        public static bool Ready(IntPtr window)
        {
            IntPtr result;
            return IsWindowEnabled(window) &&
                SendMessageTimeout(window, 0, IntPtr.Zero, IntPtr.Zero, 2, 2000, out result) != IntPtr.Zero;
        }
        public static void Answer(IntPtr dialog, int command)
        {
            if (!PostMessage(dialog, 0x0111, new IntPtr(command), IntPtr.Zero))
                throw new InvalidOperationException("Could not answer owned editor dialog");
        }
        public static void AcknowledgeError(IntPtr dialog)
        {
            var buttons = new List<IntPtr>();
            EnumChildWindows(dialog, (child, parameter) => {
                if (ClassName(child) == "Button" && IsWindowEnabled(child)) buttons.Add(child);
                return true;
            }, IntPtr.Zero);
            if (buttons.Count != 1) throw new InvalidOperationException("Owned save error did not have exactly one acknowledgement button");
            IntPtr button = buttons[0];
            if (!PostMessage(dialog, 0x0111, new IntPtr(GetDlgCtrlID(button)), button))
                throw new InvalidOperationException("Could not acknowledge owned save error");
        }
        private static IntPtr FileNameEdit(IntPtr dialog)
        {
            var legacy = new List<IntPtr>();
            var modern = new List<IntPtr>();
            EnumChildWindows(dialog, (child, parameter) => {
                if (ClassName(child) != "Edit" || !IsWindowEnabled(child)) return true;
                IntPtr parent = GetParent(child);
                int identifier = GetDlgCtrlID(child);
                // Legacy common dialogs use edt1/cmb13; the modern wrapper
                // hosts its filename edit (1001) inside an unnumbered ComboBox.
                if (identifier == 1152 || GetDlgCtrlID(parent) == 1148) legacy.Add(child);
                else if (identifier == 1001 && ClassName(parent) == "ComboBox" && GetDlgCtrlID(parent) == 0)
                    modern.Add(child);
                return true;
            }, IntPtr.Zero);
            var candidates = legacy.Count != 0 ? legacy : modern;
            if (candidates.Count > 1) throw new InvalidOperationException("Save dialog filename control is ambiguous");
            return candidates.Count == 1 ? candidates[0] : IntPtr.Zero;
        }
        public static bool FileNameReady(IntPtr dialog)
        {
            return dialog != IntPtr.Zero && IsWindowEnabled(dialog) && FileNameEdit(dialog) != IntPtr.Zero;
        }
        public static void FileName(IntPtr dialog, string path)
        {
            IntPtr edit = FileNameEdit(dialog);
            if (edit == IntPtr.Zero) throw new InvalidOperationException("Save dialog file-name control missing");
            IntPtr parent = GetParent(edit);
            bool combo = ClassName(parent) == "ComboBox";
            IntPtr target = combo ? parent : edit;
            IntPtr result;
            if (SendMessageTimeout(target, 0x000C, IntPtr.Zero, path, 2, 2000, out result) == IntPtr.Zero || result == IntPtr.Zero)
                throw new InvalidOperationException("Could not set owned save dialog's file name");
            var chosen = new StringBuilder(path.Length + 1);
            if (SendMessageTimeout(edit, 0x000D, new IntPtr(chosen.Capacity), chosen, 2, 2000, out result) == IntPtr.Zero ||
                chosen.ToString() != path)
                throw new InvalidOperationException("Owned save dialog did not retain the controlled file name");
            // Notify the filename ComboBox's actual parent, as an edit would.
            // Queue it before IDOK so the Shell updates its filename model.
            if (combo && !PostMessage(GetParent(parent), 0x0111,
                new IntPtr(GetDlgCtrlID(parent) | (5 << 16)), parent))
                throw new InvalidOperationException("Could not notify owned filename ComboBox");
        }
        public static string StaticText(IntPtr window)
        {
            var result = new StringBuilder();
            EnumChildWindows(window, (child, parameter) => {
                if (ClassName(child) != "Static") return true;
                var text = new StringBuilder(8192);
                IntPtr count;
                if (SendMessageTimeout(child, 0x000D, new IntPtr(text.Capacity), text, 2, 2000, out count) != IntPtr.Zero)
                    result.AppendLine(text.ToString());
                return true;
            }, IntPtr.Zero);
            return result.ToString();
        }
        public static string Status(IntPtr window)
        {
            foreach (string line in StaticText(window).Split(new[] {'\r','\n'}, StringSplitOptions.RemoveEmptyEntries))
                if (line.StartsWith("Ready") || line.StartsWith("Needs attention") || line.StartsWith("Saved ")) return line;
            return "";
        }
        public static void NudgeFirstSpawn(IntPtr window)
        {
            IntPtr list = FindWindowEx(window, IntPtr.Zero, "ListBox", null);
            IntPtr canvas = FindWindowEx(window, IntPtr.Zero, "ColourWarsEditorCanvas", null);
            if (list == IntPtr.Zero || canvas == IntPtr.Zero) throw new InvalidOperationException("Editor controls missing");
            Send(list, 0x0186, new IntPtr(5), IntPtr.Zero);
            Send(window, 0x0111, new IntPtr(GetDlgCtrlID(list) | (1 << 16)), list);
            Send(canvas, 0x0100, new IntPtr(0x27), IntPtr.Zero);
        }
    }
}
'@
    }

    [IO.Directory]::CreateDirectory($RunDirectory) | Out-Null
    $previousProfile = $env:LOCALAPPDATA
    $previousRoaming = $env:APPDATA
    $previousUserProfile = $env:USERPROFILE
    $userProfile = Join-Path $RunDirectory 'profile'
    $profile = Join-Path $userProfile 'AppData/Local'
    $roaming = Join-Path $userProfile 'AppData/Roaming'
    [IO.Directory]::CreateDirectory($profile) | Out-Null
    [IO.Directory]::CreateDirectory($roaming) | Out-Null
    # Shell dialogs resolve Desktop as well as the chosen directory. Keep that
    # dependency in this complete scratch profile, without changing user folders.
    [IO.Directory]::CreateDirectory((Join-Path $userProfile 'Desktop')) | Out-Null
    $myStages = Join-Path $profile 'ColourWars/stages'
    $userFile = Join-Path $myStages 'my_guidance.json'
    $elsewhere = Join-Path $RunDirectory 'elsewhere.json'
    $process = $null
    $sourceHash = (Get-FileHash -LiteralPath $Level -Algorithm SHA256).Hash
    $rows = [Collections.Generic.List[object]]::new()
    $checks = [Collections.Generic.List[object]]::new()
    $result = [ordered]@{ outcome = 'failed'; level = $Level; profile = $profile; user_profile = $userProfile; transitions = $rows;
        invariants = $checks; exit_code = $null; failure = $null }
    try {
        $env:LOCALAPPDATA = $profile
        $env:APPDATA = $roaming
        $env:USERPROFILE = $userProfile
        if ((Test-Path -LiteralPath $myStages) -or (Test-Path -LiteralPath $elsewhere)) {
            throw 'Native save-guidance fixture is not fresh: My Stages or the Save As target already exists.'
        }
        $process = Start-Process -FilePath $Executable -ArgumentList ('"' + $Level + '"') `
            -WorkingDirectory $RunDirectory -WindowStyle Hidden -PassThru `
            -RedirectStandardError (Join-Path $RunDirectory 'gui.err') `
            -RedirectStandardOutput (Join-Path $RunDirectory 'gui.out')
        $null = $process.Handle
        $window = [IntPtr]::Zero
        function Wait-GuidanceCondition([scriptblock] $Condition, [string] $Message) {
            $timer = [Diagnostics.Stopwatch]::StartNew()
            while ($timer.Elapsed.TotalSeconds -lt 15) {
                if ($process.HasExited) { throw "Editor exited during $Message ($($process.ExitCode))." }
                if (& $Condition) { return }
                Start-Sleep -Milliseconds 50
            }
            $nativeState = [ColourWars.EditorSaveProbe]::Describe($process.Id, $window)
            $checks.Add([pscustomobject]@{ step = 'timeout'; waiting_for = $Message; native_state = $nativeState;
                status = [ColourWars.EditorSaveProbe]::Status($window); title = [ColourWars.EditorWindowProbe]::Title($window);
                stage_folder_retained = Test-Path -LiteralPath $myStages;
                user_file_exists = Test-Path -LiteralPath $userFile -PathType Leaf;
                elsewhere_exists = Test-Path -LiteralPath $elsewhere -PathType Leaf })
            throw "Editor guidance timeout: $Message; $nativeState"
        }
        function Wait-GuidanceDialog([switch] $FilePicker) {
            Wait-GuidanceCondition {
                $dialogWindow = [ColourWars.EditorSaveProbe]::Dialog($process.Id, $window)
                $dialogWindow -ne [IntPtr]::Zero -and
                    (-not $FilePicker -or [ColourWars.EditorSaveProbe]::FileNameReady($dialogWindow))
            } $(if ($FilePicker) { 'owned file-name control' } else { 'owned dialog' })
            return [ColourWars.EditorSaveProbe]::Dialog($process.Id, $window)
        }
        function Wait-GuidanceReady([scriptblock] $Completed = { $true }, [string] $Message = 'return from owned dialog') {
            Wait-GuidanceCondition {
                [ColourWars.EditorSaveProbe]::Dialog($process.Id, $window) -eq [IntPtr]::Zero -and
                [ColourWars.EditorSaveProbe]::Ready($window) -and (& $Completed) } $Message
        }
        function Assert-GuidanceStatus([string] $Step, [string] $Prefix, [string] $Words) {
            $status = [ColourWars.EditorSaveProbe]::Status($window)
            if (-not $status.StartsWith($Prefix) -or -not $status.Contains($Words)) {
                throw "$Step has wrong editor guidance: $status"
            }
            $rows.Add([pscustomobject]@{ step = $Step; status = $status; title = [ColourWars.EditorWindowProbe]::Title($window) })
            return $status
        }
        Wait-GuidanceCondition {
            [ColourWars.EditorWindowProbe]::Find($process.Id) -ne [IntPtr]::Zero -and
            [ColourWars.EditorSaveProbe]::Ready([ColourWars.EditorWindowProbe]::Find($process.Id))
        } 'hidden launch'
        $window = [ColourWars.EditorWindowProbe]::Find($process.Id)
        $initialStatus = Assert-GuidanceStatus 'elsewhere-open' 'Ready' 'does not discover this destination'
        $initialTitle = [ColourWars.EditorWindowProbe]::Title($window)

        # Cancel the actual My Stages dialog before it has saved or retained its folder.
        [ColourWars.EditorSaveProbe]::MenuCommand($window, 0, 4, $true)
        $dialog = Wait-GuidanceDialog -FilePicker
        [ColourWars.EditorSaveProbe]::Answer($dialog, 2)
        Wait-GuidanceReady { -not (Test-Path -LiteralPath $myStages) } 'cancelled My Stages folder cleanup'
        $cancelStatus = [ColourWars.EditorSaveProbe]::Status($window)
        $cancelTitle = [ColourWars.EditorWindowProbe]::Title($window)
        $cancelFolder = Test-Path -LiteralPath $myStages
        $cancelHash = (Get-FileHash -LiteralPath $Level -Algorithm SHA256).Hash
        $checks.Add([pscustomobject]@{ step = 'cancel-my-stages'; expected_status = $initialStatus; status = $cancelStatus;
            expected_title = $initialTitle; title = $cancelTitle; stage_folder_retained = $cancelFolder;
            source_hash_unchanged = $cancelHash -eq $sourceHash })
        if ($cancelStatus -ne $initialStatus) { throw "Cancelled Save to My Stages changed status from '$initialStatus' to '$cancelStatus'." }
        if ($cancelTitle -ne $initialTitle) { throw "Cancelled Save to My Stages changed title/destination from '$initialTitle' to '$cancelTitle'." }
        if ($cancelFolder) { throw "Cancelled Save to My Stages retained the previously absent folder: $myStages" }
        if ($cancelHash -ne $sourceHash) { throw 'Cancelled Save to My Stages changed source file bytes.' }

        [ColourWars.EditorSaveProbe]::MenuCommand($window, 0, 4, $true)
        $dialog = Wait-GuidanceDialog -FilePicker
        [ColourWars.EditorSaveProbe]::FileName($dialog, $userFile)
        [ColourWars.EditorSaveProbe]::Answer($dialog, 1)
        Wait-GuidanceReady { (Test-Path -LiteralPath $userFile -PathType Leaf) -and
            [ColourWars.EditorSaveProbe]::Status($window).StartsWith('Saved to My Stages') } 'My Stages save status and file'
        $null = Assert-GuidanceStatus 'save-to-my-stages' 'Saved to My Stages' 'no rebuild needed'
        if (-not (Test-Path -LiteralPath $userFile -PathType Leaf)) { throw 'My Stages save did not write its chosen scratch file.' }
        $userHash = (Get-FileHash -LiteralPath $userFile -Algorithm SHA256).Hash
        [ColourWars.EditorSaveProbe]::NudgeFirstSpawn($window)
        $null = Assert-GuidanceStatus 'my-stages-edit-refresh' 'Ready' 'no rebuild needed'
        [ColourWars.EditorSaveProbe]::MenuCommand($window, 0, 2, $false)
        $null = Assert-GuidanceStatus 'my-stages-ordinary-save' 'Saved ' 'no rebuild needed'
        if ((Get-FileHash -LiteralPath $userFile -Algorithm SHA256).Hash -eq $userHash) { throw 'The ordinary Save did not persist its edit.' }
        [ColourWars.EditorSaveProbe]::MenuCommand($window, 2, 4, $false)
        $readyStatus = Assert-GuidanceStatus 'my-stages-explicit-refresh' 'Ready' 'no rebuild needed'
        $userTitle = [ColourWars.EditorWindowProbe]::Title($window)
        $userHash = (Get-FileHash -LiteralPath $userFile -Algorithm SHA256).Hash

        [ColourWars.EditorSaveProbe]::MenuCommand($window, 0, 3, $true)
        $dialog = Wait-GuidanceDialog -FilePicker
        [ColourWars.EditorSaveProbe]::Answer($dialog, 2)
        Wait-GuidanceReady
        $cancelStatus = [ColourWars.EditorSaveProbe]::Status($window)
        $cancelTitle = [ColourWars.EditorWindowProbe]::Title($window)
        $cancelHash = (Get-FileHash -LiteralPath $userFile -Algorithm SHA256).Hash
        $checks.Add([pscustomobject]@{ step = 'cancel-save-as'; expected_status = $readyStatus; status = $cancelStatus;
            expected_title = $userTitle; title = $cancelTitle; user_file_hash_unchanged = $cancelHash -eq $userHash })
        if ($cancelStatus -ne $readyStatus) { throw "Cancelled Save As changed status from '$readyStatus' to '$cancelStatus'." }
        if ($cancelTitle -ne $userTitle) { throw "Cancelled Save As changed title/destination from '$userTitle' to '$cancelTitle'." }
        if ($cancelHash -ne $userHash) { throw 'Cancelled Save As changed the current user-stage file bytes.' }

        [ColourWars.EditorSaveProbe]::NudgeFirstSpawn($window)
        $failedStatus = [ColourWars.EditorSaveProbe]::Status($window)
        $failedTitle = [ColourWars.EditorWindowProbe]::Title($window)
        [IO.File]::AppendAllText($userFile, "`n ", [Text.UTF8Encoding]::new($false))
        $externalHash = (Get-FileHash -LiteralPath $userFile -Algorithm SHA256).Hash
        [ColourWars.EditorSaveProbe]::MenuCommand($window, 0, 2, $true)
        $dialog = Wait-GuidanceDialog
        if (-not [ColourWars.EditorSaveProbe]::StaticText($dialog).Contains('Save As')) {
            throw 'Failed ordinary Save did not report the external-change recovery action.'
        }
        [ColourWars.EditorSaveProbe]::AcknowledgeError($dialog)
        Wait-GuidanceReady
        $afterFailureStatus = [ColourWars.EditorSaveProbe]::Status($window)
        $afterFailureTitle = [ColourWars.EditorWindowProbe]::Title($window)
        $afterFailureHash = (Get-FileHash -LiteralPath $userFile -Algorithm SHA256).Hash
        $checks.Add([pscustomobject]@{ step = 'failed-ordinary-save'; expected_status = $failedStatus; status = $afterFailureStatus;
            expected_title = $failedTitle; title = $afterFailureTitle; external_hash_unchanged = $afterFailureHash -eq $externalHash })
        if ($afterFailureStatus -ne $failedStatus) { throw "Failed ordinary Save changed status from '$failedStatus' to '$afterFailureStatus'." }
        if ($afterFailureTitle -ne $failedTitle) { throw "Failed ordinary Save changed title/destination from '$failedTitle' to '$afterFailureTitle'." }
        if ($afterFailureHash -ne $externalHash) { throw 'Failed ordinary Save changed external file bytes.' }

        [ColourWars.EditorSaveProbe]::MenuCommand($window, 0, 3, $true)
        $dialog = Wait-GuidanceDialog -FilePicker
        [ColourWars.EditorSaveProbe]::FileName($dialog, $elsewhere)
        [ColourWars.EditorSaveProbe]::Answer($dialog, 1)
        Wait-GuidanceReady { (Test-Path -LiteralPath $elsewhere -PathType Leaf) -and
            [ColourWars.EditorSaveProbe]::Status($window).StartsWith('Saved elsewhere.json') } 'Save As status and file'
        $null = Assert-GuidanceStatus 'save-as-elsewhere' 'Saved ' 'does not discover this destination'
        [ColourWars.EditorSaveProbe]::MenuCommand($window, 2, 4, $false)
        $null = Assert-GuidanceStatus 'elsewhere-refresh' 'Ready' 'does not discover this destination'
        if (-not [ColourWars.EditorWindowProbe]::Title($window).StartsWith('elsewhere.json') -or
            (Get-FileHash -LiteralPath $userFile -Algorithm SHA256).Hash -ne $externalHash -or
            (Get-FileHash -LiteralPath $Level -Algorithm SHA256).Hash -ne $sourceHash) {
            throw 'Save As did not isolate its new destination from the previous files.'
        }
        [ColourWars.EditorWindowProbe]::Close($process.Id)
        if (-not $process.WaitForExit(10000)) { throw 'Save-guidance editor did not close gracefully.' }
        $process.WaitForExit()
        $result.exit_code = $process.ExitCode
        if ($process.ExitCode -ne 0) { throw "Save-guidance editor exited with $($process.ExitCode)." }
        $diagnostics = [string](Get-Content -LiteralPath (Join-Path $RunDirectory 'gui.err') -Raw)
        if (-not [string]::IsNullOrWhiteSpace($diagnostics)) { throw "Unexpected editor diagnostics: $diagnostics" }
        if (Test-Path -LiteralPath (Join-Path $profile 'ColourWars/save_data.json')) { throw 'The editor wrote a player save.' }
        if ($SourceLevel) {
            $sourceRun = Join-Path $RunDirectory 'source-read-only'
            [IO.Directory]::CreateDirectory($sourceRun) | Out-Null
            Test-EditorWindow -Executable $Executable -Level $SourceLevel -RunDirectory $sourceRun -ExpectStatus 'then rebuild the game'
        }
        $result.outcome = 'passed'
        Write-Host 'PASS hidden editor destination guidance, My Stages/ordinary Save/refresh, Save As, cancel and failed save'
    }
    catch {
        $result.failure = $_.Exception.Message
        if ($null -ne $process -and -not $process.HasExited) {
            $checks.Add([pscustomobject]@{ step = 'failure-state'; native_state = [ColourWars.EditorSaveProbe]::Describe($process.Id, $window) })
        }
        throw
    }
    finally {
        # Child processes inherited their own environment at launch. Restore
        # this caller even if process cleanup or evidence writing throws.
        $env:LOCALAPPDATA = $previousProfile
        $env:APPDATA = $previousRoaming
        $env:USERPROFILE = $previousUserProfile
        if ($null -ne $process -and -not $process.HasExited) {
            $dialog = [ColourWars.EditorSaveProbe]::Dialog($process.Id, $window)
            if ($dialog -ne [IntPtr]::Zero) { [ColourWars.EditorSaveProbe]::Answer($dialog, 2) }
            [ColourWars.EditorWindowProbe]::Close($process.Id)
            if (-not $process.WaitForExit(3000)) {
                # This process owns only the probe's scratch edits and files.
                $process.Kill()
                $process.WaitForExit()
            }
        }
        $result | ConvertTo-Json -Depth 6 | Out-File -LiteralPath (Join-Path $RunDirectory 'save-guidance-result.json') -Encoding utf8
        if ($null -ne $process) { $process.Dispose() }
    }
}
