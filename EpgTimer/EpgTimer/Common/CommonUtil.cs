using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace EpgTimer
{
    static class CommonUtil
    {
        [DllImport("user32.dll")]
        static extern bool GetLastInputInfo(ref LASTINPUTINFO plii);

        [DllImport("kernel32.dll")]
        static extern uint GetTickCount();

        // Struct we'll need to pass to the function
        [StructLayout(LayoutKind.Sequential)]
        struct LASTINPUTINFO
        {
            public uint cbSize;
            public uint dwTime;
        }

        public static int GetIdleTimeSec()
        {
            // The number of ticks that passed since last input
            uint IdleTicks = 0;

            // Set the struct
            LASTINPUTINFO LastInputInfo = new LASTINPUTINFO();
            LastInputInfo.cbSize = (uint)Marshal.SizeOf(LastInputInfo);
            LastInputInfo.dwTime = 0;

            // If we have a value from the function
            if (GetLastInputInfo(ref LastInputInfo))
            {
                // Number of idle ticks = system uptime ticks - number of ticks at last input
                IdleTicks = unchecked(GetTickCount() - LastInputInfo.dwTime);
            }
            return (int)(IdleTicks / 1000);
        }

        [DllImport("kernel32", CharSet = CharSet.Unicode)]
        static extern IntPtr LoadLibrary(string lpFileName);

        [DllImport("kernel32")]
        static extern bool FreeLibrary(IntPtr hModule);

        [DllImport("kernel32")]
        static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

        delegate void Setting(IntPtr parentWnd);

        public static bool ShowPlugInSetting(string dllFilePath, IntPtr parentWnd)
        {
            IntPtr module = LoadLibrary(dllFilePath);
            if (module != IntPtr.Zero)
            {
                try
                {
                    IntPtr func = GetProcAddress(module, "Setting");
                    if (func != IntPtr.Zero)
                    {
                        Setting settingDelegate = (Setting)Marshal.GetDelegateForFunctionPointer(func, typeof(Setting));
                        settingDelegate(parentWnd);
                        return true;
                    }
                }
                finally
                {
                    FreeLibrary(module);
                }
            }
            return false;
        }
    }
}
