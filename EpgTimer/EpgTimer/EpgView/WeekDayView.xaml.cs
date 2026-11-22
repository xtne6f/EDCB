using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
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
    /// WeekDayView.xaml の相互作用ロジック
    /// </summary>
    public partial class WeekDayView : UserControl
    {
        public event Action<DateTime> Click;

        public WeekDayView()
        {
            InitializeComponent();
        }

        public void ClearInfo()
        {
            stackPanel_day.Children.Clear();
        }

        public void SetDay(List<DateTime> dayList, double serviceWidth, bool gradationHeader, bool isClickLeft)
        {
            stackPanel_day.Children.Clear();
            if (serviceWidth > 2)
            {
                uint tickCountToPreventAccidentalClick = (uint)Environment.TickCount;
                foreach (DateTime time in dayList)
                {
                    var item = new TextBlock()
                    {
                        Style = (Style)FindResource(
                            time.DayOfWeek == DayOfWeek.Saturday ? "AppEpgWeekDayHeaderSaturdayTextBlockStyle" :
                            time.DayOfWeek == DayOfWeek.Sunday ? "AppEpgWeekDayHeaderSundayTextBlockStyle" : "AppEpgWeekDayHeaderTextBlockStyle"),
                        Text = time.ToString("M\\/d\r\n(ddd)"),
                    };
                    var border = new Border()
                    {
                        BorderThickness = new Thickness(),
                        Child = item,
                        Style = (Style)FindResource("AppEpgWeekDayHeaderTextBorderStyle"),
                    };
                    var backgroundColor = (Color)FindResource(
                        time.DayOfWeek == DayOfWeek.Saturday ? "AppEpgWeekDayHeaderSaturdayTextBackgroundColor" :
                        time.DayOfWeek == DayOfWeek.Sunday ? "AppEpgWeekDayHeaderSundayTextBackgroundColor" : "AppEpgWeekDayHeaderTextBackgroundColor");
                    var gridItem = new UniformGrid();
                    if (gradationHeader == false)
                    {
                        gridItem.Background = new SolidColorBrush(backgroundColor);
                        gridItem.Background.Freeze();
                    }
                    else
                    {
                        gridItem.Background = ColorDef.GradientBrush(backgroundColor, 0.8);
                    }

                    gridItem.Margin = new Thickness(1, 1, 1, 1);
                    gridItem.Width = serviceWidth - 2;
                    if (isClickLeft)
                    {
                        gridItem.MouseLeftButtonDown += (sender, e) =>
                        {
                            if (Click != null && (uint)e.Timestamp - tickCountToPreventAccidentalClick > 500)
                            {
                                Click((DateTime)((FrameworkElement)sender).Tag);
                            }
                        };
                    }
                    else
                    {
                        gridItem.MouseRightButtonUp += (sender, e) =>
                        {
                            if (Click != null && (uint)e.Timestamp - tickCountToPreventAccidentalClick > 500)
                            {
                                Click((DateTime)((FrameworkElement)sender).Tag);
                            }
                        };
                    }
                    gridItem.Tag = time;
                    gridItem.Children.Add(border);
                    stackPanel_day.Children.Add(gridItem);
                }
            }
        }

        public void SetTodayMark(int startHour)
        {
            DateTime today = DateTime.UtcNow.AddHours(9 - startHour).Date;
            UniformGrid todayItem = stackPanel_day.Children.OfType<UniformGrid>().FirstOrDefault(grid => (DateTime)grid.Tag == today);
            UniformGrid markedItem = stackPanel_day.Children.OfType<UniformGrid>().FirstOrDefault(grid => ((Border)grid.Children[0]).BorderThickness.Left != 0);
            if (todayItem != markedItem)
            {
                if (markedItem != null)
                {
                    ((Border)markedItem.Children[0]).BorderThickness = new Thickness();
                }
                if (todayItem != null)
                {
                    ((Border)todayItem.Children[0]).BorderThickness = new Thickness(1);
                }
            }
        }
    }
}
