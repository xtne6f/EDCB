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

        public void SetTime(List<DateTime> timeList, double heightPerHour, List<Brush> brushList)
        {
            ClearInfo();
            if (heightPerHour > 1)
            {
                foreach (DateTime time in timeList)
                {
                    // 高さ合わせのため上下に同じものを置く
                    var items = new TextBlock[2];
                    for (int i = 0; i < 2; i++)
                    {
                        items[i] = new TextBlock() { Style = (Style)FindResource("AppEpgTimeHeaderDateTextBlockStyle") };
                        items[i].Inlines.Add(new Run(time.ToString("M\\/d")));
                        if (heightPerHour >= 60)
                        {
                            var weekday = new Run(time.ToString("ddd"))
                            {
                                Style = (Style)FindResource(
                                    time.DayOfWeek == DayOfWeek.Saturday ? "AppEpgTimeHeaderSaturdayRunStyle" :
                                    time.DayOfWeek == DayOfWeek.Sunday ? "AppEpgTimeHeaderSundayRunStyle" : "AppEpgTimeHeaderDayRunStyle")
                            };
                            items[i].Inlines.Add(new LineBreak());
                            items[i].Inlines.Add(new Run("("));
                            items[i].Inlines.Add(weekday);
                            items[i].Inlines.Add(new Run(")"));
                        }
                    }

                    var grid = new Grid()
                    {
                        Background = brushList[time.Hour / 6],
                        Height = heightPerHour - 1,
                        Margin = new Thickness(1, 1, 1, 0)
                    };
                    grid.RowDefinitions.Add(new RowDefinition() { Height = GridLength.Auto });
                    grid.RowDefinitions.Add(new RowDefinition());
                    grid.RowDefinitions.Add(new RowDefinition() { Height = GridLength.Auto });
                    grid.RowDefinitions.Add(new RowDefinition());
                    grid.RowDefinitions.Add(new RowDefinition() { Height = GridLength.Auto });
                    grid.Children.Add(items[0]);
                    items[1].Visibility = Visibility.Hidden;
                    Grid.SetRow(items[1], 4);
                    grid.Children.Add(items[1]);
                    var hour = new TextBlock()
                    {
                        Style = (Style)FindResource("AppEpgTimeHeaderHourTextBlockStyle"),
                        Text = time.Hour.ToString()
                    };
                    Grid.SetRow(hour, 2);
                    grid.Children.Add(hour);
                    stackPanel_time.Children.Add(grid);
                }
            }
        }

        private void scrollViewer_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            e.Handled = true;
        }
    }
}
