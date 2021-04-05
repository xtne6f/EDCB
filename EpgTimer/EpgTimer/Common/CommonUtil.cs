using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace EpgTimer
{
    static class CommonUtil
    {
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
            if (NativeMethods.GetLastInputInfo(ref LastInputInfo))
            {
                // Number of idle ticks = system uptime ticks - number of ticks at last input
                IdleTicks = unchecked(NativeMethods.GetTickCount() - LastInputInfo.dwTime);
            }
            return (int)(IdleTicks / 1000);
        }

        delegate void Setting(IntPtr parentWnd);

        public static bool ShowPlugInSetting(string dllFilePath, IntPtr parentWnd)
        {
            IntPtr module = NativeMethods.LoadLibrary(dllFilePath);
            if (module != IntPtr.Zero)
            {
                try
                {
                    IntPtr func = NativeMethods.GetProcAddress(module, "Setting");
                    if (func != IntPtr.Zero)
                    {
                        Setting settingDelegate = (Setting)Marshal.GetDelegateForFunctionPointer(func, typeof(Setting));
                        settingDelegate(parentWnd);
                        return true;
                    }
                }
                finally
                {
                    NativeMethods.FreeLibrary(module);
                }
            }
            return false;
        }

        struct BROWSEINFO
        {
            public IntPtr hwndOwner;
            public IntPtr pidlRoot;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string pszDisplayName;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string lpszTitle;
            public uint ulFlags;
            public IntPtr lpfn;
            public IntPtr lParam;
            public int iImage;
        }

        public static string BrowseFolder(IntPtr hwndOwner, string title)
        {
            const int MAX_PATH = 260;
            const uint BIF_NEWDIALOGSTYLE = 0x40;
            var bi = new BROWSEINFO();
            bi.hwndOwner = hwndOwner;
            bi.pidlRoot = IntPtr.Zero;
            bi.pszDisplayName = new string('\0', MAX_PATH);
            bi.lpszTitle = title;
            bi.ulFlags = BIF_NEWDIALOGSTYLE;
            bi.lpfn = IntPtr.Zero;
            bi.iImage = 0;
            IntPtr pidl = NativeMethods.SHBrowseForFolder(ref bi);
            if (pidl != IntPtr.Zero)
            {
                try
                {
                    var buff = new StringBuilder(MAX_PATH);
                    if (NativeMethods.SHGetPathFromIDList(pidl, buff))
                    {
                        return buff.ToString();
                    }
                }
                finally
                {
                    Marshal.FreeCoTaskMem(pidl);
                }
            }
            return null;
        }

        static uint msgTaskbarCreated;
        public static uint RegisterTaskbarCreatedWindowMessage()
        {
            if (msgTaskbarCreated == 0)
            {
                msgTaskbarCreated = NativeMethods.RegisterWindowMessage("TaskbarCreated");
            }
            return msgTaskbarCreated;
        }

        public static bool SetForegroundWindow(IntPtr hWnd)
        {
            return NativeMethods.SetForegroundWindow(hWnd);
        }

        private static class NativeMethods
        {
            [DllImport("user32.dll")]
            public static extern bool GetLastInputInfo(ref LASTINPUTINFO plii);

            [DllImport("kernel32.dll")]
            public static extern uint GetTickCount();

            [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
            public static extern IntPtr LoadLibrary(string lpFileName);

            [DllImport("kernel32.dll")]
            public static extern bool FreeLibrary(IntPtr hModule);

            [DllImport("kernel32.dll", BestFitMapping = false, ThrowOnUnmappableChar = true)]
            public static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

            [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
            public static extern IntPtr SHBrowseForFolder([In] ref BROWSEINFO lpbi);

            [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
            public static extern bool SHGetPathFromIDList(IntPtr pidl, StringBuilder pszPath);

            [DllImport("user32.dll", CharSet = CharSet.Unicode)]
            public static extern uint RegisterWindowMessage(string lpString);

            [DllImport("user32.dll")]
            public static extern bool SetForegroundWindow(IntPtr hWnd);
        }
    }
}
