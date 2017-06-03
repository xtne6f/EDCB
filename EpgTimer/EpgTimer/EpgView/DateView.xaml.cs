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
            uniformGrid_day.Children.Clear();
            uniformGrid_time.Children.Clear();
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

                    uniformGrid_day.Children.Add(day);

                    for (int i = 6; i <= 18; i += 6)
                    {
                        Button hour = new Button();
                        hour.Content = i.ToString();
                        hour.DataContext = itemTime.AddHours(i);
                        hour.Click += new RoutedEventHandler(button_time_Click);
                        uniformGrid_time.Children.Add(hour);
                    }

                    itemTime = itemTime.AddDays(1);
                }
                columnDefinition.MinWidth = uniformGrid_time.Children.Count * 15;
                columnDefinition.MaxWidth = uniformGrid_time.Children.Count * 40;
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
