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

        public void SetMode(byte reboot, byte suspendMode)
        {
            label_mode0.Visibility = reboot != 1 && (suspendMode < 1 || suspendMode > 3) ? Visibility.Visible : Visibility.Collapsed;
            label_mode1.Visibility = reboot != 1 && suspendMode == 1 ? Visibility.Visible : Visibility.Collapsed;
            label_mode2.Visibility = reboot != 1 && suspendMode == 2 ? Visibility.Visible : Visibility.Collapsed;
            label_mode3.Visibility = reboot != 1 && suspendMode == 3 ? Visibility.Visible : Visibility.Collapsed;
            label_reboot.Visibility = reboot == 1 ? Visibility.Visible : Visibility.Collapsed;
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
                if (DialogResult == null)
                {
                    DialogResult = true;
                }
            }
        }

        private void button_ok_Click(object sender, RoutedEventArgs e)
        {
            countTimer.Stop();
            DialogResult = true;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            progressBar.Value = 20;
            countTimer.Start();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            countTimer.Stop();
        }
    }
}
