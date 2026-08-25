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
        private List<DateTime> canvasTimeList = new List<DateTime>();
        private double canvasHeightPerHour;

        public TimeView()
        {
            InitializeComponent();
        }

        public void ClearMarker()
        {
            foreach (Line item in canvas.Children.OfType<Line>().ToArray())
            {
                canvas.Children.Remove(item);
            }
        }

        public void ClearInfo()
        {
            stackPanel_time.Children.Clear();
            ClearMarker();
            canvasTimeList.Clear();
            canvas.Height = 0;
        }

        public void SetTime(IEnumerable<DateTime> sortedTimeList, double heightPerHour, bool needTimeOnly, List<Brush> brushList, bool weekMode)
        {
            ClearInfo();
            if (heightPerHour > 1)
            {
                foreach (DateTime time in sortedTimeList)
                {
                    canvasTimeList.Add(time);

                    // 高さ合わせのため上下に同じものを置く
                    var items = new TextBlock[2];
                    for (int i = 0; i < 2; i++)
                    {
                        if (weekMode == false && (time.Hour % 3 == 0 || needTimeOnly))
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
                    if (items[0] != null)
                    {
                        grid.Children.Add(items[0]);
                        items[1].Visibility = Visibility.Hidden;
                        Grid.SetRow(items[1], 4);
                        grid.Children.Add(items[1]);
                    }
                    var hour = new TextBlock()
                    {
                        Style = (Style)FindResource("AppEpgTimeHeaderHourTextBlockStyle"),
                        Text = time.Hour.ToString()
                    };
                    Grid.SetRow(hour, 2);
                    grid.Children.Add(hour);
                    stackPanel_time.Children.Add(grid);
                }
                canvasHeightPerHour = heightPerHour;
                canvas.Height = canvasHeightPerHour * canvasTimeList.Count + stackPanel_time.Margin.Bottom;
            }
        }

        public void AddMarker(IEnumerable<KeyValuePair<DateTime, TimeSpan>> timeRanges, Brush brush)
        {
            if (canvasTimeList.Count > 0)
            {
                var yRanges = new List<Tuple<double, double>>();
                foreach (KeyValuePair<DateTime, TimeSpan> timeRange in timeRanges)
                {
                    // 時間の範囲をY軸の範囲に変換する
                    DateTime startTime = timeRange.Key;
                    int index = canvasTimeList.BinarySearch(new DateTime(startTime.Year, startTime.Month, startTime.Day, startTime.Hour, 0, 0));
                    double y1 = canvasHeightPerHour * (index < 0 ? ~index : index + (startTime - canvasTimeList[index]).TotalMinutes / 60);
                    DateTime endTime = startTime + timeRange.Value;
                    index = canvasTimeList.BinarySearch(new DateTime(endTime.Year, endTime.Month, endTime.Day, endTime.Hour, 0, 0));
                    double y2 = canvasHeightPerHour * (index < 0 ? ~index : index + (endTime - canvasTimeList[index]).TotalMinutes / 60);
                    if (y1 < y2)
                    {
                        yRanges.Add(new Tuple<double, double>(y1, y2));
                    }
                }
                yRanges.Sort();

                var blurEffect = new System.Windows.Media.Effects.DropShadowEffect();
                blurEffect.BlurRadius = 10;
                blurEffect.ShadowDepth = 2;
                blurEffect.Freeze();
                for (int i = 0; i < yRanges.Count(); i++)
                {
                    var item = new Line();
                    item.X1 = 5;
                    item.X2 = 5;
                    item.Y1 = yRanges[i].Item1;
                    // 重なっていればつなげる
                    double y2 = yRanges[i].Item2;
                    for (; i + 1 < yRanges.Count() && y2 >= yRanges[i + 1].Item1; i++)
                    {
                        y2 = Math.Max(y2, yRanges[i + 1].Item2);
                    }
                    item.Y2 = y2;
                    item.Stroke = brush;
                    item.StrokeThickness = 3;
                    item.Effect = blurEffect;
                    canvas.Children.Add(item);
                    Canvas.SetZIndex(item, 10);
                }
            }
        }

        private void scrollViewer_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            e.Handled = true;
        }
    }
}
