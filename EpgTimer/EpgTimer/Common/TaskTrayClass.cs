using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.Drawing;
using System.ComponentModel;
using System.Windows;
using System.Windows.Forms;

using System.Threading;
using System.Runtime.InteropServices;
using System.Diagnostics;

namespace EpgTimer
{
    class TaskTrayClass : IDisposable
    {
        private NotifyIcon notifyIcon = new NotifyIcon();
        private Window targetWindow;

        public string Text {
            get { return notifyIcon.Text; }
            set
            {
                if (value.Length > 63)
                {
                    notifyIcon.Text = value.Substring(0,60) + "...";
                }
                else
                {
                    notifyIcon.Text = value;
                }
            }
        }
        public Icon Icon {
            get { return notifyIcon.Icon; }
            set { notifyIcon.Icon = value; }
        }
        public bool Visible{
            get { return notifyIcon.Visible; }
            set { notifyIcon.Visible = value; }
        }
        public WindowState LastViewState
        {
            get;
            set;
        }
        public event EventHandler ContextMenuClick = null;

        private delegate int EnumWindowsDelegate(IntPtr hWnd, int lParam);

        [DllImport("user32.dll")]
        private static extern int EnumWindows(EnumWindowsDelegate lpEnumFunc, int lParam);
        [DllImport("user32.dll")]
        private static extern int IsWindowVisible(IntPtr hWnd);
        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
        [DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out int lpdwProcessId);

        [DllImport("User32.dll", EntryPoint = "SendMessage")]
        private static extern Int32 SendMessage(IntPtr hWnd, Int32 Msg, Int32 wParam, Int32 lParam);
        [DllImport("User32.dll", EntryPoint = "SendMessage")]
        private static extern Int32 SendMessage(IntPtr hWnd, Int32 Msg, Int32 wParam, StringBuilder lParam);
        private const int WM_LBUTTONDOWN = 0x00000201;
        private const int WM_GETTEXT = 0x000D;

        private Thread _th = null;
        private System.Diagnostics.Stopwatch _sw = new System.Diagnostics.Stopwatch();

        // 
        private void _BalloonTipShown(object sender, EventArgs e)
        {
            Debug.Print(notifyIcon.BalloonTipText);

            // タイマーをリセット
            if (_sw.IsRunning) _sw.Stop();
            _sw.Reset();
            _sw.Start();

            // threadを作成
            if (_th != null)
            {
                _th.Abort();
            }
            _th = new Thread(new ThreadStart(ThreadFunction));
            _th.Start();
        }

        void ThreadFunction()
        {
            try
            {
                long TimeOut = (long)IniFileHandler.GetPrivateProfileInt("SET", "Baloon", 0, SettingPath.TimerSrvIniPath);
                if (TimeOut == 0) return;
                while (_sw.IsRunning)
                {
                    // 10秒以上経っていたら
                    if (_sw.ElapsedMilliseconds > TimeOut * 1000)
                    {
                        EnumWindows(new EnumWindowsDelegate(delegate(IntPtr hWnd, int lParam)
                        {
                            StringBuilder sb = new StringBuilder(0x1024);
                            if (IsWindowVisible(hWnd) != 0)
                            {
                                //GetWindowText(hWnd, sb, sb.Capacity);
                                //string title = sb.ToString();
                                //int pid;
                                //GetWindowThreadProcessId(hWnd, out pid);
                                //Process p = Process.GetProcessById(pid);

                                sb = new StringBuilder(0x1024);
                                SendMessage(hWnd, WM_GETTEXT, sb.Capacity, sb);
                                string title2 = sb.ToString();

                                Debug.Print("check : " + title2);

                                if (title2.Equals(notifyIcon.BalloonTipText))
                                {
                                    SendMessage(hWnd, WM_LBUTTONDOWN, 0, 0);
                                    return 0;
                                }
                            }
                            return 1;
                        }), 0);
                        _sw.Stop();
                        break;
                    }
                }
            }
            catch (ThreadAbortException)
            {
                // スレッドを中止した時の例外を無視する
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }

        }

        public TaskTrayClass(Window target)
        {
            Text = "";
            notifyIcon.BalloonTipIcon = ToolTipIcon.Info;
            notifyIcon.Click += NotifyIcon_Click;
            notifyIcon.BalloonTipClicked += NotifyIcon_Click;
            // 接続先ウィンドウ
            targetWindow = target;
            LastViewState = targetWindow.WindowState;
            // ウィンドウに接続
            if (targetWindow != null) {
                targetWindow.Closing += new System.ComponentModel.CancelEventHandler(target_Closing);
            }
            notifyIcon.ContextMenuStrip = new ContextMenuStrip();
            notifyIcon.BalloonTipShown += new EventHandler(_BalloonTipShown);
        }


        public void SetContextMenu(List<Object> list)
        {
            if( list.Count == 0 )
            {
                notifyIcon.ContextMenuStrip = null;
            }else{
                ContextMenuStrip menu = new ContextMenuStrip();
                foreach(Object item in list)
                {
                    ToolStripMenuItem newcontitem = new ToolStripMenuItem();
                    if (item.ToString().Length > 0)
                    {
                        newcontitem.Tag = item;
                        newcontitem.Text = item.ToString();
                        newcontitem.Click +=new EventHandler(newcontitem_Click);
                        menu.Items.Add(newcontitem);
                    }
                    else
                    {
                        menu.Items.Add(new ToolStripSeparator());
                    }

                }
                notifyIcon.ContextMenuStrip = menu;
            }
        }

        public void ShowBalloonTip(String title, String tips, Int32 timeOutMSec)
        {
            try
            {
                if (title.Length > 0)
                {
                    notifyIcon.BalloonTipTitle = title;
                }
                else
                {
                    notifyIcon.BalloonTipTitle = " ";
                }
                if (tips.Length > 0)
                {
                    notifyIcon.BalloonTipText = tips;
                }
                else
                {
                    notifyIcon.BalloonTipText = " ";
                }
                notifyIcon.BalloonTipIcon = ToolTipIcon.Info;
                if (Settings.Instance.NoBallonTips == false)
                {
                    notifyIcon.ShowBalloonTip(timeOutMSec);
                }
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        void  newcontitem_Click(object sender, EventArgs e)
        {
            if (sender.GetType() == typeof(ToolStripMenuItem))
            {
                if (ContextMenuClick != null)
                {
                    ToolStripMenuItem item = sender as ToolStripMenuItem;
                    ContextMenuClick(item.Tag, e);
                }
            }
        }

        public void Dispose()
        {
            // ウィンドウから切断
            if (targetWindow != null)
            {
                targetWindow.Closing -= new System.ComponentModel.CancelEventHandler(target_Closing);
                targetWindow = null;
            }
            // スレッドが生きていたら終了させる
            if (_th != null)
            {
                if (_th.IsAlive) _th.Abort();
            }
        }

        private void target_Closing(object sender, CancelEventArgs e)
        {
            if (e.Cancel == false)
            {
                notifyIcon.Dispose();
                notifyIcon = null;
            }
        }

        private void NotifyIcon_Click(object sender, EventArgs e)
        {
            if (e.GetType() == typeof(MouseEventArgs))
            {
                MouseEventArgs mouseEvent = e as MouseEventArgs;
                if (mouseEvent.Button == MouseButtons.Left)
                {
                    //左クリック
                    if (targetWindow != null)
                    {
                        targetWindow.Show();
                        targetWindow.WindowState = LastViewState;
                        targetWindow.Activate();
                    }
                }
            }
        }   
    }
}
