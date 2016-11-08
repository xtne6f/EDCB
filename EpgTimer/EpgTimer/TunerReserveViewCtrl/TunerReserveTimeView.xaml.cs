using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace EpgTimer.TunerReserveViewCtrl
{
    /// <summary>
    /// TunerReserveTimeView.xaml の相互作用ロジック
    /// </summary>
    public partial class TunerReserveTimeView : UserControl
    {
        public TunerReserveTimeView()
        {
            InitializeComponent();
        }

        public void ClearInfo()
        {
            stackPanel_time.Children.Clear();
        }

        public void SetTime(List<DateTime> timeList)
        {
            try
            {
                bool? use28 = Settings.Instance.LaterTimeUse == true ? null : (bool?)false;
                stackPanel_time.Children.Clear();
                foreach (DateTime time1 in timeList)
                {
                    var item = new TextBlock();
                    var timeMod = new DateTime28(time1, use28);
                    DateTime time = timeMod.DateTimeMod;
                    string HourMod = timeMod.HourMod.ToString();

                    double height = Settings.Instance.TunerMinHeight;
                    item.Height = (60 * height) - 4;

                    if (height < 1)
                    {
                        item.Text = time.ToString("M/d\r\n") + HourMod;
                    }
                    else if (height < 1.5)
                    {
                        item.Text = time.ToString("M/d\r\n(ddd)\r\n") + HourMod;
                    }
                    else
                    {
                        item.Text = time.ToString("M/d\r\n(ddd)\r\n\r\n") + HourMod;
                    }

                    if (time.DayOfWeek == DayOfWeek.Saturday)
                    {
                        item.Foreground = Brushes.Blue;
                    }
                    else if (time.DayOfWeek == DayOfWeek.Sunday)
                    {
                        item.Foreground = Brushes.Red;
                    }
                    else
                    {
                        item.Foreground = Brushes.Black;
                    }

                    item.Margin = new Thickness(2, 2, 2, 2);
                    item.Background = Brushes.AliceBlue;
                    item.TextAlignment = TextAlignment.Center;
                    item.FontSize = 12;
                    stackPanel_time.Children.Add(item);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }

        }

        private void scrollViewer_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            e.Handled = true;
        }
    }
}
