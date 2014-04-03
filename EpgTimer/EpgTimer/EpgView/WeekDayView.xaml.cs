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

                    item.Width = Settings.Instance.ServiceWidth - 4;
                    item.Text = time.ToString("M/d\r\n(ddd)");


                    if (time.DayOfWeek == DayOfWeek.Saturday)
                    {
                        item.Foreground = Brushes.Blue;
                    }
                    else if (time.DayOfWeek == DayOfWeek.Sunday)
                    {
                        item.Foreground = Brushes.Red;
                    }
                    else
                    {
                        item.Foreground = Brushes.Black;
                    }

                    item.Margin = new Thickness(2, 2, 2, 2);
                    item.Background = Brushes.AliceBlue;
                    item.TextAlignment = TextAlignment.Center;
                    item.FontSize = 12;
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
