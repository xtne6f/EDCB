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
            if (Settings.ContextMenuResourceDictionary != null)
            {
                button_prev.ContextMenu.Resources.MergedDictionaries.Add(Settings.ContextMenuResourceDictionary);
                button_next.ContextMenu.Resources.MergedDictionaries.Add(Settings.ContextMenuResourceDictionary);
            }
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
                for (DateTime itemTime = startTime.Date; itemTime < endTime; itemTime = itemTime.AddDays(1))
                {
                    Button day = new Button();
                    day.Content = new Border()
                    {
                        BorderThickness = new Thickness(),
                        Child = new TextBlock()
                        {
                            Padding = new Thickness(2, 0, 2, 0),
                            Text = itemTime.ToString("M\\/d(ddd)"),
                            VerticalAlignment = VerticalAlignment.Center
                        },
                        Style = (Style)FindResource("AppEpgDateButtonTextBorderStyle")
                    };
                    day.FontWeight = FontWeights.Normal;
                    if (itemTime.DayOfWeek == DayOfWeek.Saturday)
                    {
                        day.Foreground = (Brush)FindResource("AppEpgDateSaturdayButtonForegroundBrush");
                    }
                    else if (itemTime.DayOfWeek == DayOfWeek.Sunday)
                    {
                        day.Foreground = (Brush)FindResource("AppEpgDateSundayButtonForegroundBrush");
                    }
                    day.Style = (Style)FindResource("AppEpgButtonStyle");
                    day.Tag = itemTime;
                    day.Click += button_time_Click;
                    uniformGrid_day.Children.Add(day);

                    for (int i = 6; i <= 18; i += 6)
                    {
                        Button hour = new Button();
                        hour.Content = i.ToString();
                        hour.Style = (Style)FindResource("AppEpgButtonStyle");
                        hour.Tag = itemTime.AddHours(i);
                        hour.Click += button_time_Click;
                        uniformGrid_time.Children.Add(hour);
                    }
                }
                columnDefinition.MinWidth = uniformGrid_time.Children.Count * columnDefinition_prev_next.Width.Value * 3 / 8;
                columnDefinition.MaxWidth = uniformGrid_time.Children.Count * columnDefinition_prev_next.Width.Value;
            }
        }

        public void SetTodayMark()
        {
            DateTime today = DateTime.UtcNow.AddHours(9).Date;
            Button todayButton = uniformGrid_day.Children.OfType<Button>().FirstOrDefault(btn => (DateTime)btn.Tag == today);
            Button markedButton = uniformGrid_day.Children.OfType<Button>().FirstOrDefault(btn => ((Border)btn.Content).BorderThickness.Left != 0);
            if (todayButton != markedButton)
            {
                if (markedButton != null)
                {
                    ((Border)markedButton.Content).BorderThickness = new Thickness();
                }
                if (todayButton != null)
                {
                    ((Border)todayButton.Content).BorderThickness = new Thickness(1);
                }
            }
        }

        public void SetScrollTime(DateTime time)
        {
            time = time.Date.AddHours(time.Hour / 6 * 6);
            for (int i = 0; i < 2; i++)
            {
                Button timeButton = (i == 0 ? uniformGrid_time : uniformGrid_day).Children.OfType<Button>().FirstOrDefault(btn => (DateTime)btn.Tag == time);
                Button markedButton = (i == 0 ? uniformGrid_time : uniformGrid_day).Children.OfType<Button>().FirstOrDefault(btn => btn.FontWeight == FontWeights.Bold);
                if (timeButton != markedButton)
                {
                    if (markedButton != null)
                    {
                        markedButton.FontWeight = FontWeights.Normal;
                    }
                    if (timeButton != null)
                    {
                        timeButton.FontWeight = FontWeights.Bold;
                    }
                }
                time = time.Date;
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
