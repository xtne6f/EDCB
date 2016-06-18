using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace EpgTimer
{
    /// <summary>
    /// SuspendCheckWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SuspendCheckWindow : Window
    {
        private DispatcherTimer countTimer;

        public SuspendCheckWindow()
        {
            InitializeComponent();

            countTimer = new DispatcherTimer(DispatcherPriority.Normal);
            countTimer.Tick += new EventHandler(CountTimer);
            countTimer.Interval = TimeSpan.FromSeconds(1);
        }

        public void SetMode(Byte reboot, Byte suspendMode)
        {
            string preWord = (CommonManager.Instance.NWMode == false ? "" : "録画サーバを");
            string postWord = (CommonManager.Instance.NWMode == false ? "します。" : "させます。");

            if (reboot == 1)
            {
                label1.Content = preWord + "再起動" + postWord;
            }
            else
            {
                if (suspendMode == 1)
                {
                    label1.Content = preWord + "スタンバイに移行" + postWord;
                }
                else if (suspendMode == 2)
                {
                    label1.Content = preWord + "休止に移行" + postWord;
                }
                else if (suspendMode == 3)
                {
                    label1.Content = preWord + "シャットダウン" + postWord;
                }
            }
        }

        private void CountTimer(object sender, EventArgs e)
        {
            if (progressBar.Value != 0)
            {
                progressBar.Value--;
            }
            else
            {
                countTimer.Stop();
                DialogResult = false;
            }
            labelTimer.Content = progressBar.Value;
        }

        private void button_work_now_Click(object sender, RoutedEventArgs e)
        {
            countTimer.Stop();
            DialogResult = false;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            countTimer.Stop();
            DialogResult = true;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            button_cancel.Focus();
            progressBar.Maximum = Settings.Instance.SuspendChkTime;
            progressBar.Value = progressBar.Maximum;
            labelTimer.Content = progressBar.Value;
            countTimer.Start();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            countTimer.Stop();
            if (DialogResult == null)
            {
                DialogResult = true;
            }
        }
    }
}
