using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.Drawing;
using System.ComponentModel;
using System.Windows;
using System.Windows.Interop;
using System.Runtime.InteropServices;

namespace EpgTimer
{
    interface ITaskTrayClickHandler
    {
        void TaskTrayLeftClick();
        void TaskTrayRightClick();
    }

    class TaskTrayClass : IDisposable
    {
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct NOTIFYICONDATA
        {
            public int cbSize;
            public IntPtr hWnd;
            public uint uID;
            public uint uFlags;
            public uint uCallbackMessage;
            public IntPtr hIcon;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
            public string szTip;
            public uint dwState;
            public uint dwStateMask;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
            public string szInfo;
            public uint uTimeoutOrVersion;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
            public string szInfoTitle;
            public uint dwInfoFlags;
        }

        [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
        private static extern bool Shell_NotifyIcon(uint dwMessage, [In] ref NOTIFYICONDATA lpData);

        private const int WM_APP_TRAY = 0x8100;
        private static Dictionary<TaskTrayClass, HwndSource> hwndDictionary;
        private Window targetWindow;
        private HwndSourceHook sourceHook;
        private System.Windows.Threading.DispatcherTimer balloonTimer;
        private string _text = "";
        private Uri _iconUri;
        private bool _visible;

        public TaskTrayClass(Window target)
        {
            targetWindow = target;
        }

        public string Text
        {
            get { return _text; }
            set
            {
                _text = value;
                Visible = Visible;
            }
        }

        public Uri IconUri
        {
            get { return _iconUri; }
            set
            {
                _iconUri = value;
                Visible = Visible;
            }
        }

        public int ForceHideBalloonTipSec
        {
            get;
            set;
        }

        public bool Visible
        {
            get { return _visible; }
            set
            {
                if (value)
                {
                    const uint NIF_MESSAGE = 0x01;
                    const uint NIF_ICON = 0x02;
                    const uint NIF_TIP = 0x04;
                    if (hwndDictionary == null)
                    {
                        hwndDictionary = new Dictionary<TaskTrayClass, HwndSource>();
                    }
                    if (hwndDictionary.ContainsKey(this) == false)
                    {
                        // ネイティブウィンドウがなければ生成する(PresentationSourceがこれで取得可能になるわけではないので注意)
                        hwndDictionary[this] = HwndSource.FromHwnd(new WindowInteropHelper(targetWindow).EnsureHandle());
                    }
                    _visible = true;
                    var nid = new NOTIFYICONDATA();
                    nid.cbSize = Marshal.SizeOf(nid);
                    nid.hWnd = hwndDictionary[this].Handle;
                    nid.uID = 1;
                    nid.uFlags = NIF_MESSAGE | NIF_TIP;
                    nid.uCallbackMessage = WM_APP_TRAY;
                    nid.szTip = Text.Length > 95 ? Text.Substring(0, 92) + "..." : Text;
                    nid.szInfo = "";
                    nid.szInfoTitle = "";
                    if (IconUri != null)
                    {
                        // SystemParametersは論理ピクセル単位
                        var m = hwndDictionary[this].CompositionTarget.TransformToDevice;
                        using (var stream = Application.GetResourceStream(IconUri).Stream)
                        using (var icon = new Icon(stream, ((int)(SystemParameters.SmallIconWidth * m.M11) + 15) / 16 * 16,
                                                           ((int)(SystemParameters.SmallIconHeight * m.M22) + 15) / 16 * 16))
                        {
                            nid.uFlags |= NIF_ICON;
                            nid.hIcon = icon.Handle;
                            if (Shell_NotifyIcon(1, ref nid) == false)
                            {
                                Shell_NotifyIcon(0, ref nid);
                            }
                        }
                    }
                    else if (Shell_NotifyIcon(1, ref nid) == false)
                    {
                        Shell_NotifyIcon(0, ref nid);
                    }
                    if (sourceHook == null && CommonUtil.RegisterTaskbarCreatedWindowMessage() != 0)
                    {
                        sourceHook = WndProc;
                        hwndDictionary[this].AddHook(sourceHook);
                    }
                }
                else if (_visible)
                {
                    _visible = false;
                    if (sourceHook != null)
                    {
                        hwndDictionary[this].RemoveHook(sourceHook);
                        sourceHook = null;
                    }
                    var nid = new NOTIFYICONDATA();
                    nid.cbSize = Marshal.SizeOf(nid);
                    nid.hWnd = hwndDictionary[this].Handle;
                    nid.uID = 1;
                    nid.uFlags = 0;
                    nid.szTip = "";
                    nid.szInfo = "";
                    nid.szInfoTitle = "";
                    Shell_NotifyIcon(2, ref nid);
                }
            }
        }

        public void ShowBalloonTip(string title, string tips, int timeOutMSec)
        {
            if (Visible)
            {
                const uint NIF_INFO = 0x10;
                const uint NIIF_INFO = 1;
                var nid = new NOTIFYICONDATA();
                nid.cbSize = Marshal.SizeOf(nid);
                nid.hWnd = hwndDictionary[this].Handle;
                nid.uID = 1;
                nid.uFlags = NIF_INFO;
                nid.szTip = "";
                nid.szInfo = tips.Length > 255 ? tips.Substring(0, 252) + "..." : tips;
                nid.uTimeoutOrVersion = (uint)timeOutMSec;
                nid.szInfoTitle = title.Length > 63 ? title.Substring(0, 60) + "..." : title;
                nid.dwInfoFlags = NIIF_INFO;
                Shell_NotifyIcon(1, ref nid);
            }
        }

        public void Dispose()
        {
            Visible = false;
            if (hwndDictionary != null)
            {
                // FromHwnd(targetWindow)で得たHwndSourceはtargetWindowに紐づいた共有物なのでDispose()してはいけない
                hwndDictionary.Remove(this);
            }
        }

        private static IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            TaskTrayClass self = hwndDictionary.SingleOrDefault(a => a.Value.Handle == hwnd).Key;
            if (msg == WM_APP_TRAY && self != null)
            {
                const int WM_LBUTTONUP = 0x0202;
                const int WM_RBUTTONUP = 0x0205;
                const int NIN_BALLOONSHOW = 0x0402;
                const int NIN_BALLOONHIDE = 0x0403;
                const int NIN_BALLOONTIMEOUT = 0x0404;
                switch (lParam.ToInt32() & 0xFFFF)
                {
                    case WM_LBUTTONUP:
                        if (self.targetWindow is ITaskTrayClickHandler)
                        {
                            ((ITaskTrayClickHandler)self.targetWindow).TaskTrayLeftClick();
                        }
                        break;
                    case WM_RBUTTONUP:
                        if (self.targetWindow is ITaskTrayClickHandler)
                        {
                            ((ITaskTrayClickHandler)self.targetWindow).TaskTrayRightClick();
                        }
                        break;
                    case NIN_BALLOONSHOW:
                        if (self.ForceHideBalloonTipSec > 0)
                        {
                            // 指定タイムアウトでバルーンチップを強制的に閉じる
                            if (self.balloonTimer == null)
                            {
                                self.balloonTimer = new System.Windows.Threading.DispatcherTimer();
                                self.balloonTimer.Tick += (sender, e) =>
                                {
                                    if (self.Visible)
                                    {
                                        self.Visible = false;
                                        self.Visible = true;
                                    }
                                    self.balloonTimer.Stop();
                                };
                            }
                            self.balloonTimer.Interval = TimeSpan.FromSeconds(self.ForceHideBalloonTipSec);
                            self.balloonTimer.Start();
                        }
                        break;
                    case NIN_BALLOONHIDE:
                    case NIN_BALLOONTIMEOUT:
                        if (self.balloonTimer != null)
                        {
                            self.balloonTimer.Stop();
                        }
                        break;
                }
            }
            else if (msg == (int)CommonUtil.RegisterTaskbarCreatedWindowMessage() && self != null)
            {
                self.Visible = self.Visible;
            }
            return IntPtr.Zero;
        }
    }
}
