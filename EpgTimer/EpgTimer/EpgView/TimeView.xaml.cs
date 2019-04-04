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

namespace EpgTimer.EpgView
{
    /// <summary>
    /// TimeView.xaml の相互作用ロジック
    /// </summary>
    public partial class TimeView : UserControl
    {
        public TimeView()
        {
            InitializeComponent();
        }

        public void ClearInfo()
        {
            stackPanel_time.Children.Clear();
        }

        public void SetTime(List<DateTime> timeList, double height, bool needTimeOnly, List<Brush> brushList, bool weekMode)
        {
            stackPanel_time.Children.Clear();
            if (60 * height > 1)
            {
                foreach (DateTime time in timeList)
                {
                    TextBlock item = new TextBlock();
                    item.Height = (60 * height) - 1;

                    if (weekMode == false)
                    {
                        if (time.Hour % 3 == 0 || needTimeOnly)
                        {
                            item.Inlines.Add(new Run(time.ToString("M\\/d")));
                            item.Inlines.Add(new LineBreak());
                            if (height >= 1)
                            {
                                Run weekday = new Run(time.ToString("ddd"));
                                weekday.Foreground = time.DayOfWeek == DayOfWeek.Saturday ? Brushes.Blue :
                                                     time.DayOfWeek == DayOfWeek.Sunday ? Brushes.Red : Brushes.White;
                                weekday.FontWeight = FontWeights.Bold;
                                item.Inlines.Add(new Run("("));
                                item.Inlines.Add(weekday);
                                item.Inlines.Add(new Run(")"));
                            }
                        }
                        else
                        {
                            if (height >= 1.5)
                            {
                                item.Inlines.Add(new LineBreak());
                            }
                        }
                    }
                    if (height >= 1)
                    {
                        item.Inlines.Add(new LineBreak());
                        if (height >= 1.5)
                        {
                            item.Inlines.Add(new LineBreak());
                        }
                    }
                    Run text = new Run(time.Hour.ToString());
                    text.FontSize = 13;
                    text.FontWeight = FontWeights.Bold;
                    item.Inlines.Add(text);

                    item.Margin = new Thickness(1, 1, 1, 0);
                    item.Background = brushList[time.Hour / 6];
                    item.TextAlignment = TextAlignment.Center;
                    item.Foreground = Brushes.White;
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
