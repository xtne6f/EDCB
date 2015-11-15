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
using System.Windows.Navigation;
using System.Windows.Shapes;

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

        public void SetTime(List<DateTime> timeList, bool NeedTimeOnly)
        {
            try
            {
                stackPanel_time.Children.Clear();
                foreach (DateTime time in timeList)
                {
                    TextBlock item = new TextBlock();

                    double height = Settings.Instance.ReserveMinHeight;
                    item.Height = (60 * height) - 4;

                    if (time.Hour % 3 == 0 || NeedTimeOnly == true)
                    {
                        if (height < 1)
                        {
                            item.Text = time.ToString("M/d\r\nH");
                        }
                        else if (height < 1.5)
                        {
                            item.Text = time.ToString("M/d\r\n(ddd)\r\nH");
                        }
                        else
                        {
                            item.Text = time.ToString("M/d\r\n(ddd)\r\n\r\nH");
                        }
                    }
                    else
                    {
                        if (height < 1)
                        {
                            item.Text = time.Hour.ToString();
                        }
                        else if (height < 1.5)
                        {
                            item.Text = time.ToString("\r\nH");
                        }
                        else
                        {
                            item.Text = time.ToString("\r\n\r\n\r\nH");
                        }
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
