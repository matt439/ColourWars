# Embed this source in each check's Add-Type compilation. A distinct class name
# lets multiple checks run in one shell without duplicate type definitions.
function Get-CheckWindowSource([string] $ClassName) {
    if ($ClassName -cnotmatch '^[A-Za-z][A-Za-z0-9]*$') { throw 'Invalid window helper class name.' }
    return @'
namespace ColourWars
{
    internal static class CHECK_WINDOW_CLASS
    {
        private delegate bool EnumWindowProc(System.IntPtr window, System.IntPtr parameter);
        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern bool EnumWindows(EnumWindowProc callback, System.IntPtr parameter);
        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(System.IntPtr window, out uint processId);
        [System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
        private static extern int GetClassName(System.IntPtr window, System.Text.StringBuilder name, int maximum);
        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern bool PostMessage(System.IntPtr window, uint message, System.IntPtr wParam, System.IntPtr lParam);

        public static System.IntPtr[] ForProcess(int processId)
        {
            var result = new System.Collections.Generic.List<System.IntPtr>();
            EnumWindows((window, parameter) => {
                uint owner;
                GetWindowThreadProcessId(window, out owner);
                if (owner == processId) result.Add(window);
                return true;
            }, System.IntPtr.Zero);
            return result.ToArray();
        }

        public static string ClassName(System.IntPtr window)
        {
            var name = new System.Text.StringBuilder(256);
            GetClassName(window, name, name.Capacity);
            return name.ToString();
        }

        public static System.IntPtr Find(int processId, string className)
        {
            foreach (var window in ForProcess(processId))
                if (ClassName(window) == className) return window;
            return System.IntPtr.Zero;
        }

        public static void Close(int processId)
        {
            foreach (var window in ForProcess(processId))
                PostMessage(window, 0x0010, System.IntPtr.Zero, System.IntPtr.Zero);
        }
    }
}
'@.Replace('CHECK_WINDOW_CLASS', $ClassName)
}
