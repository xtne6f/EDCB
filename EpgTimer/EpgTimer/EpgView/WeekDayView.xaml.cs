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
    /// WeekDayView.xaml の相互作用ロジック
    /// </summary>
    public partial class WeekDayView : UserControl
    {
        public WeekDayView()
        {
            InitializeComponent();
        }

        public void ClearInfo()
        {
            stackPanel_day.Children.Clear();
        }

        public void SetDay(List<DateTime> dayList, double serviceWidth, bool gradationHeader)
        {
            stackPanel_day.Children.Clear();
            if (serviceWidth > 2)
            {
                foreach (DateTime time in dayList)
                {
                    TextBlock item = new TextBlock();

                    item.Width = serviceWidth - 2;
                    item.Text = time.ToString("M\\/d\r\n(ddd)");

                    Color backgroundColor;
                    if (time.DayOfWeek == DayOfWeek.Saturday)
                    {
                        item.Foreground = Brushes.DarkBlue;
                        backgroundColor = Colors.Lavender;
                    }
                    else if (time.DayOfWeek == DayOfWeek.Sunday)
                    {
                        item.Foreground = Brushes.DarkRed;
                        backgroundColor = Colors.MistyRose;
                    }
                    else
                    {
                        item.Foreground = Brushes.Black;
                        backgroundColor = Colors.White;
                    }
                    var gridItem = new System.Windows.Controls.Primitives.UniformGrid();
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
                    item.TextAlignment = TextAlignment.Center;
                    item.FontSize = 12;
                    item.FontWeight = FontWeights.Bold;
                    // 単にCenterだとやや重い感じになるので上げる
                    item.Padding = new Thickness(0, 0, 0, 4);
                    item.VerticalAlignment = VerticalAlignment.Center;
                    gridItem.Children.Add(item);
                    stackPanel_day.Children.Add(gridItem);
                }
            }
        }
    }
}
