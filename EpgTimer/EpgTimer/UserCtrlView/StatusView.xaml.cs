using System;
using System.Windows.Controls;
using System.Windows.Threading;

namespace EpgTimer.UserCtrlView
{
    /// <summary>
    /// StatusView.xaml の相互作用ロジック
    /// </summary>
    public partial class StatusView : UserControl
    {
        private string[] st = { "", "", "", "" };
        private DispatcherTimer displayTimer = new DispatcherTimer();
        private TimeSpan timerInterval = TimeSpan.FromSeconds(5);
        public TimeSpan TimerInterval { get { return timerInterval; } set { timerInterval = value; } }
        public StatusView()
        {
            InitializeComponent();

            displayTimer.Tick += (sender, e) =>
            {
                displayTimer.Stop();
                st[3] = "";
                Refresh();
            };
        }
        public void SetText(string s1 = null, string s2 = null, string s3 = null, TimeSpan? interval = null)
        {
            if (s1 != null) st[1] = s1;
            if (s2 != null) st[2] = s2;
            if (s3 != null) st[3] = s3;
            displayTimer.Interval = interval != null ? (TimeSpan)interval : timerInterval;
            if (string.IsNullOrEmpty(s3) == false) displayTimer.Start();
            Refresh();
        }
        public void AppendText(string s1 = "", string s2 = "", string s3 = "", TimeSpan? interval = null)
        {
            SetText(st[1] + s1, st[2] + s2, s3 + st[3], interval);
        }
        public void ClearText()
        {
            SetText("", "", "", timerInterval);
        }
        private void Refresh()
        {
            this.Status.Text = (st[1] + "  " + st[2]).Trim();
            this.Notify.Text = st[3];
        }
    }
}
