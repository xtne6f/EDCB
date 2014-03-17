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

        public void SetDay(System.Collections.SortedList dayList)
        {
            try
            {
                stackPanel_day.Children.Clear();
                foreach (DateTime time in dayList.Values)
                {
                    TextBlock item = new TextBlock();

                    item.Width = Settings.Instance.ServiceWidth - 2;
                    item.Text = time.ToString("M/d\r\n(ddd)");


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
                    if (Settings.Instance.EpgGradationHeader == false)
                    {
                        item.Background = new SolidColorBrush(backgroundColor);
                    }
                    else
                    {
                        item.Background = ColorDef.GradientBrush(backgroundColor, 0.8);
                    }

                    item.Margin = new Thickness(1, 1, 1, 1);
                    item.TextAlignment = TextAlignment.Center;
                    item.FontSize = 12;
                    item.FontWeight = FontWeights.Bold;
                    stackPanel_day.Children.Add(item);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
    }
}
