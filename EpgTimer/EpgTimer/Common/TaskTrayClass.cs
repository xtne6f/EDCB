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
    class TaskTrayClass : IDisposable
    {
        // NotifyIconの生成は初回のVisibleまで遅延
        private NotifyIcon notifyIcon;
        private string _text = "";
        private Uri _iconUri;
        private List<KeyValuePair<string, Action<string>>> _contextMenuList;

        public event EventHandler Click;

        public string Text
        {
            get { return _text; }
            set
            {
                _text = value;
                if (notifyIcon != null)
                {
                    notifyIcon.Text = Text.Length > 63 ? Text.Substring(0, 60) + "..." : Text;
                }
            }
        }

        public Uri IconUri
        {
            get { return _iconUri; }
            set
            {
                _iconUri = value;
                if (notifyIcon != null)
                {
                    if (IconUri != null)
                    {
                        using (var stream = System.Windows.Application.GetResourceStream(IconUri).Stream)
                        {
                            System.Drawing.Size size = SystemInformation.SmallIconSize;
                            notifyIcon.Icon = new Icon(stream, (size.Width + 15) / 16 * 16, (size.Height + 15) / 16 * 16);
                        }
                    }
                    else
                    {
                        notifyIcon.Icon = null;
                    }
                }
            }
        }

        public List<KeyValuePair<string, Action<string>>> ContextMenuList
        {
            get { return _contextMenuList; }
            set
            {
                _contextMenuList = value;
                if (notifyIcon != null)
                {
                    if (ContextMenuList != null && ContextMenuList.Count > 0)
                    {
                        var menu = new ContextMenuStrip();
                        foreach (var item in ContextMenuList)
                        {
                            if (item.Key != null)
                            {
                                var newcontitem = new ToolStripMenuItem();
                                newcontitem.Text = item.Key;
                                // CS4のforeachと互換するため明示的に捕捉
                                var itemCap = item;
                                newcontitem.Click += (sender, e) => itemCap.Value(itemCap.Key);
                                menu.Items.Add(newcontitem);
                            }
                            else
                            {
                                menu.Items.Add(new ToolStripSeparator());
                            }
                        }
                        notifyIcon.ContextMenuStrip = menu;
                    }
                    else
                    {
                        notifyIcon.ContextMenuStrip = null;
                    }
                }
            }
        }

        public int ForceHideBalloonTipSec
        {
            get;
            set;
        }

        public bool Visible
        {
            get { return notifyIcon != null && notifyIcon.Visible; }
            set
            {
                if (notifyIcon != null)
                {
                    notifyIcon.Visible = value;
                }
                else if (value)
                {
                    notifyIcon = new NotifyIcon();
                    notifyIcon.Click += (sender, e) =>
                    {
                        MouseEventArgs mouseEvent = e as MouseEventArgs;
                        if (mouseEvent != null && mouseEvent.Button == MouseButtons.Left)
                        {
                            // 左クリック
                            if (Click != null)
                            {
                                Click(sender, e);
                            }
                        }
                    };

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
                        if (ForceHideBalloonTipSec > 0)
                        {
                            balloonTimer.Interval = TimeSpan.FromSeconds(ForceHideBalloonTipSec);
                            balloonTimer.Start();
                        }
                    };
                    notifyIcon.BalloonTipClicked += (sender, e) => balloonTimer.Stop();
                    notifyIcon.BalloonTipClosed += (sender, e) => balloonTimer.Stop();

                    // プロパティ反映のため
                    Text = Text;
                    IconUri = IconUri;
                    ContextMenuList = ContextMenuList;
                    notifyIcon.Visible = true;
                }
            }
        }

        public void ShowBalloonTip(string title, string tips, int timeOutMSec)
        {
            if (notifyIcon != null)
            {
                notifyIcon.ShowBalloonTip(timeOutMSec, title, tips, ToolTipIcon.Info);
            }
        }

        public void Dispose()
        {
            if (notifyIcon != null)
            {
                notifyIcon.Dispose();
                notifyIcon = null;
            }
        }
    }
}
