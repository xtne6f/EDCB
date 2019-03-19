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
            stackPanel_time.Children.Clear();
            double height = Settings.Instance.EpgSettingList[0].MinHeight;
            if (60 * height > 4)
            {
                foreach (DateTime time in timeList)
                {
                    TextBlock item = new TextBlock();
                    item.Height = (60 * height) - 4;

                    string text = "";
                    if (time.Hour % 3 == 0 || NeedTimeOnly == true)
                    {
                        text += time.ToString("M\\/d\r\n");
                        if (height >= 1)
                        {
                            text += time.ToString("(ddd)\r\n");
                            if (height >= 1.5)
                            {
                                text += "\r\n";
                            }
                        }
                    }
                    else
                    {
                        if (height >= 1)
                        {
                            text += "\r\n";
                            if (height >= 1.5)
                            {
                                text += "\r\n\r\n";
                            }
                        }
                    }
                    item.Text = text + time.Hour;
                    item.Foreground = time.DayOfWeek == DayOfWeek.Saturday ? Brushes.Blue :
                                      time.DayOfWeek == DayOfWeek.Sunday ? Brushes.Red : Brushes.Black;
                    item.Margin = new Thickness(2, 2, 2, 2);
                    item.Background = Brushes.AliceBlue;
                    item.TextAlignment = TextAlignment.Center;
                    item.FontSize = 12;
                    stackPanel_time.Children.Add(item);
                }
            }
        }

        private void scrollViewer_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            e.Handled = true;
        }
    }
}
