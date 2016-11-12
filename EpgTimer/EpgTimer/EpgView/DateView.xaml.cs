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
            stackPanel_day.Children.Clear();
            stackPanel_time.Children.Clear();
        }

        public void SetTime(List<DateTime> timeList)
        {
            try
            {
                ClearInfo();
                if (timeList.Count() == 0) return;

                DateTime itemTime = timeList.First().Date;
                while (itemTime <= timeList.Last())
                {
                    var day_btn = new Button();
                    day_btn.Padding = new Thickness(1);
                    day_btn.Width = 105;
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
                    stackPanel_day.Children.Add(day_btn);

                    for (int i = 6; i <= 18; i += 6)
                    {
                        var hour_btn = new Button();
                        hour_btn.Padding = new Thickness(1);
                        hour_btn.Width = 35;
                        hour_btn.Content = itemTime.ToString(i.ToString() + "時");
                        hour_btn.DataContext = itemTime.AddHours(i);
                        hour_btn.Click += new RoutedEventHandler(button_time_Click);
                        stackPanel_time.Children.Add(hour_btn);
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
