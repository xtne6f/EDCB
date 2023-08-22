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
        public event Action<DateTime> TimeButtonClick;
        public event Action<DateTime, ContextMenu, ContextMenuEventArgs> TimeButtonContextMenuOpening;

        public DateView()
        {
            InitializeComponent();
            button_prev.Tag = DateTime.MinValue;
            button_next.Tag = DateTime.MaxValue;
        }

        public void ClearInfo()
        {
            button_prev.IsEnabled = false;
            button_next.IsEnabled = false;
            button_prev.Visibility = Visibility.Collapsed;
            uniformGrid_day.Children.Clear();
            uniformGrid_time.Children.Clear();
        }

        public void SetTime(bool enablePrev, bool enableNext, DateTime startTime, DateTime endTime)
        {
            ClearInfo();
            button_prev.IsEnabled = enablePrev;
            button_next.IsEnabled = enableNext;
            button_prev.Visibility = enablePrev || enableNext ? Visibility.Visible : Visibility.Collapsed;
            if (startTime != default(DateTime))
            {
                DateTime itemTime = new DateTime(startTime.Year, startTime.Month, startTime.Day, 0, 0, 0);
                while (itemTime < endTime)
                {
                    Button day = new Button();
                    day.Content = itemTime.ToString("M\\/d(ddd)");
                    if (itemTime.DayOfWeek == DayOfWeek.Saturday)
                    {
                        day.Foreground = Brushes.Blue;
                    }
                    else if (itemTime.DayOfWeek == DayOfWeek.Sunday)
                    {
                        day.Foreground = Brushes.Red;
                    }
                    day.Tag = itemTime;
                    day.Click += button_time_Click;
                    uniformGrid_day.Children.Add(day);

                    for (int i = 6; i <= 18; i += 6)
                    {
                        Button hour = new Button();
                        hour.Content = i.ToString();
                        hour.Tag = itemTime.AddHours(i);
                        hour.Click += button_time_Click;
                        uniformGrid_time.Children.Add(hour);
                    }

                    itemTime = itemTime.AddDays(1);
                }
                columnDefinition.MinWidth = uniformGrid_time.Children.Count * 15;
                columnDefinition.MaxWidth = uniformGrid_time.Children.Count * 40;
            }
        }

        void button_time_Click(object sender, RoutedEventArgs e)
        {
            if (TimeButtonClick != null)
            {
                TimeButtonClick((DateTime)((Button)sender).Tag);
            }
        }

        void button_time_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            if (TimeButtonContextMenuOpening != null)
            {
                TimeButtonContextMenuOpening((DateTime)((Button)sender).Tag, ((Button)sender).ContextMenu, e);
            }
            else
            {
                e.Handled = true;
            }
        }
    }
}
