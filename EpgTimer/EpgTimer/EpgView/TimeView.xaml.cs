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
    /// TimeView.xaml の相互作用ロジック
    /// </summary>
    public partial class TimeView : UserControl
    {
        public TimeView()
        {
            InitializeComponent();
        }

        public void ClearInfo()
        {
            stackPanel_time.Children.Clear();
        }

        public void SetTime(List<DateTime> timeList, bool NeedTimeOnly, bool weekMode)
        {
            try
            {
                stackPanel_time.Children.Clear();
                foreach (DateTime time in timeList)
                {
                    TextBlock item = new TextBlock();

                    double height = Settings.Instance.MinHeight;
                    item.Height = (60 * height) - 1;

                    if (weekMode == true)
                    {
                        if (height >= 1) item.Inlines.Add(new LineBreak());
                        if (height >= 1.5) item.Inlines.Add(new LineBreak());
                        PutTime(item.Inlines, time);
                    }
                    else
                    {
                        if (time.Hour % 3 == 0 || NeedTimeOnly == true)
                        {
                            if (height < 1)
                            {
                                PutDate(item.Inlines, time);
                                PutTime(item.Inlines, time);

                            }
                            else if (height < 1.5)
                            {
                                PutDate(item.Inlines, time);
                                PutWeekDay(item.Inlines, time);
                                PutTime(item.Inlines, time);
                            }
                            else
                            {
                                PutDate(item.Inlines, time);
                                PutWeekDay(item.Inlines, time);
                                item.Inlines.Add(new LineBreak());
                                PutTime(item.Inlines, time);
                            }
                        }
                        else
                        {
                            if (height < 1)
                            {
                                PutTime(item.Inlines, time);
                            }
                            else if (height < 1.5)
                            {
                                item.Inlines.Add(new LineBreak());
                                PutTime(item.Inlines, time);
                            }
                            else
                            {
                                item.Inlines.Add(new LineBreak());
                                item.Inlines.Add(new LineBreak());
                                item.Inlines.Add(new LineBreak());
                                PutTime(item.Inlines, time);
                            }
                        }
                    }

                    item.Margin = new Thickness(1, 1, 1, 0);
                    item.Background = CommonManager.Instance.CustTimeColorList[time.Hour / 6];
                    item.TextAlignment = TextAlignment.Center;
                    item.Foreground = Brushes.White;
                    item.FontSize = 12;
                    stackPanel_time.Children.Add(item);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }

        }

        private static void PutDate(InlineCollection inline, DateTime time)
        {
            Run text = new Run(time.ToString("M/d"));
            inline.Add(text);
            inline.Add(new LineBreak());
        }

        private static void PutWeekDay(InlineCollection inline, DateTime time)
        {
            SolidColorBrush color = Brushes.White;
            if (time.DayOfWeek == DayOfWeek.Saturday)
            {
                color = Brushes.Blue;
            }
            else if (time.DayOfWeek == DayOfWeek.Sunday)
            {
                color = Brushes.Red;
            }

            Run weekday = new Run(time.ToString("ddd"));
            weekday.Foreground = color;
            weekday.FontWeight = FontWeights.Bold;
            inline.Add(new Run("("));
            inline.Add(weekday);
            inline.Add(new Run(")"));
            inline.Add(new LineBreak());
        }

        private static void PutTime(InlineCollection inline, DateTime time)
        {
            Run text = new Run(time.ToString("%H"));
            text.FontSize = 13;
            text.FontWeight = FontWeights.Bold;
            inline.Add(text);
        }

        private void scrollViewer_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            e.Handled = true;
        }
    }
}
