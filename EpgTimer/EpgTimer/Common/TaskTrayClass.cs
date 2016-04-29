using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.Drawing;
using System.ComponentModel;
using System.Windows;
using System.Windows.Forms;

namespace EpgTimer
{
    public enum TaskIconSpec : uint { TaskIconBlue, TaskIconRed, TaskIconGreen, TaskIconGray, TaskIconNone };

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
        private TaskIconSpec iconSpec = TaskIconSpec.TaskIconNone;
        public TaskIconSpec Icon
        {
            get { return iconSpec; }
            set
            {
                if (iconSpec == value) return;
                //
                iconSpec = value;
                notifyIcon.Icon = GetTaskTrayIcon(value);
            }
        }
        private Icon GetTaskTrayIcon(TaskIconSpec status)
        {
            switch (status)
            {
                case TaskIconSpec.TaskIconBlue:     return Properties.Resources.TaskIconBlue;
                case TaskIconSpec.TaskIconRed:      return Properties.Resources.TaskIconRed;
                case TaskIconSpec.TaskIconGreen:    return Properties.Resources.TaskIconGreen;
                case TaskIconSpec.TaskIconGray:     return Properties.Resources.TaskIconGray;
                default: return null;
            }
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

            // 指定タイムアウトでバルーンチップを強制的に閉じる
            var balloonTimer = new System.Windows.Threading.DispatcherTimer();
            balloonTimer.Tick += (sender, e) =>
            {
                if (notifyIcon.Visible)
                {
                    notifyIcon.Visible = false;
                    notifyIcon.Visible = true;
                }
                balloonTimer.Stop();
            };
            notifyIcon.BalloonTipShown += (sender, e) =>
            {
                if (Settings.Instance.ForceHideBalloonTipSec > 0)
                {
                    balloonTimer.Interval = TimeSpan.FromSeconds(Math.Max(Settings.Instance.ForceHideBalloonTipSec, 1));
                    balloonTimer.Start();
                }
            };
            notifyIcon.BalloonTipClicked += (sender, e) => balloonTimer.Stop();
            notifyIcon.BalloonTipClosed += (sender, e) => balloonTimer.Stop();
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
                        try
                        {
                            targetWindow.Show();
                            targetWindow.WindowState = LastViewState;
                            targetWindow.Activate();
                        }
                        catch { }
                    }
                }
            }
        }   
    }
}
