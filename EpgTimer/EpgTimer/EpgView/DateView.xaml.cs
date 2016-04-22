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
            stackPanel_time.Children.Clear();
            stackPanel_day.Children.Clear();
        }

        public void SetTime(List<DateTime> timeList)
        {
            try
            {
                ClearInfo();
                if (timeList.Count == 0)
                {
                    return;
                }

                DateTime startTime = timeList[0];
                DateTime endTime = timeList[timeList.Count - 1];
                DateTime itemTime = new DateTime(startTime.Year, startTime.Month, startTime.Day, 0, 0, 0);
                while (itemTime < endTime)
                {
                    Button day = new Button();
                    day.Padding = new Thickness(1);
                    day.Width = 120;
                    day.Content = itemTime.ToString("M/d(ddd)");
                    if (itemTime.DayOfWeek == DayOfWeek.Saturday)
                    {
                        day.Foreground = Brushes.Blue;
                    }
                    else if (itemTime.DayOfWeek == DayOfWeek.Sunday)
                    {
                        day.Foreground = Brushes.Red;
                    }
                    day.DataContext = itemTime;
                    day.Click += new RoutedEventHandler(button_time_Click);

                    stackPanel_day.Children.Add(day);

                    for (int i = 6; i <= 18; i += 6)
                    {
                        Button hour_btn = new Button();
                        hour_btn.Padding = new Thickness(1);
                        hour_btn.Width = 40;
                        hour_btn.Content = itemTime.ToString(i.ToString() + "時");
                        hour_btn.DataContext = itemTime.AddHours(i);
                        hour_btn.Click += new RoutedEventHandler(button_time_Click);
                        stackPanel_time.Children.Add(hour_btn);
                    }

                    itemTime = itemTime.AddDays(1);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
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
