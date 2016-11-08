using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace EpgTimer.EpgView
{
    /// <summary>
    /// DateView.xaml の相互作用ロジック
    /// </summary>
    public partial class DateView : UserControl
    {
        public event RoutedEventHandler TimeButtonClick = null;

        public DateView()
        {
            InitializeComponent();
        }

        public void ClearInfo()
        {
            grid_days.Children.Clear();
        }

        public void SetTime(List<DateTime> timeList)
        {
            try
            {
                grid_days.Children.Clear();
                if (timeList.Count() == 0) return;

                bool isWrap = false; //true にすると画面端で折り返すようになるが‥(オプション未使用)
                var stack_days = isWrap == false ? (Panel)new StackPanel { Orientation = Orientation.Horizontal } : new WrapPanel();
                grid_days.Children.Add(stack_days);

                DateTime itemTime = timeList.First().Date;
                while (itemTime <= timeList.Last())
                {
                    var stack_1day = new StackPanel();
                    stack_days.Children.Add(stack_1day);

                    var day_btn = new Button();
                    var stack_hour = new StackPanel { Orientation = Orientation.Horizontal };
                    stack_1day.Children.Add(day_btn);
                    stack_1day.Children.Add(stack_hour);

                    day_btn.Padding = new Thickness(1);
                    day_btn.Width = 120;
                    day_btn.Content = itemTime.ToString("M/d(ddd)");
                    if (itemTime.DayOfWeek == DayOfWeek.Saturday)
                    {
                        day_btn.Foreground = Brushes.Blue;
                    }
                    else if (itemTime.DayOfWeek == DayOfWeek.Sunday)
                    {
                        day_btn.Foreground = Brushes.Red;
                    }
                    day_btn.DataContext = itemTime;
                    day_btn.Click += new RoutedEventHandler(button_time_Click);

                    for (int i = 6; i <= 18; i += 6)
                    {
                        var hour_btn = new Button();
                        hour_btn.Padding = new Thickness(1);
                        hour_btn.Width = 40;
                        hour_btn.Content = itemTime.ToString(i.ToString() + "時");
                        hour_btn.DataContext = itemTime.AddHours(i);
                        hour_btn.Click += new RoutedEventHandler(button_time_Click);
                        stack_hour.Children.Add(hour_btn);
                    }

                    itemTime += TimeSpan.FromDays(1);
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        void button_time_Click(object sender, RoutedEventArgs e)
        {
            if (TimeButtonClick != null)
            {
                TimeButtonClick(sender, e);
            }
        }
    }
}
