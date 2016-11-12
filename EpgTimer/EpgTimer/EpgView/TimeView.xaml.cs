using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;

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
            scrollViewer.PreviewMouseWheel += new MouseWheelEventHandler((sende, e) => e.Handled = true);
            this.Background = CommonManager.Instance.EpgTimeBorderColor;
        }

        public void ClearInfo()
        {
            stackPanel_time.Children.Clear();
        }

        public void SetTime(List<DateTime> timeList, bool weekMode, bool tunerMode = false)
        {
            try
            {
                stackPanel_time.Children.Clear();
                bool? use28 = Settings.Instance.LaterTimeUse == true ? null : (bool?)false;
                double h3L = (12 + 3) * 3 * ViewUtil.MainWindow.DeviceMatrix.M22;
                double h6L = h3L * 2;

                foreach (DateTime time1 in timeList)
                {
                    var timeMod = new DateTime28(time1, use28);
                    DateTime time = timeMod.DateTimeMod;
                    string HourMod = timeMod.HourMod.ToString();

                    var item = new TextBlock();
                    stackPanel_time.Children.Add(item);
                    item.TextAlignment = TextAlignment.Center;
                    item.FontSize = 12;
                    item.Margin = new Thickness(1, 0, 1, 1);

                    if (tunerMode == false)
                    {
                        item.Foreground = CommonManager.Instance.EpgTimeFontColor;
                        item.Background = CommonManager.Instance.CustTimeColorList[time1.Hour / 6];
                        item.Height = 60 * Settings.Instance.MinHeight - item.Margin.Top - item.Margin.Bottom;
                        if (weekMode == false)
                        {
                            item.Inlines.Add(new Run(time.ToString("M/d\r\n")));
                            if (item.Height >= h3L)
                            {
                                var color = time.DayOfWeek == DayOfWeek.Sunday ? Brushes.Red : time.DayOfWeek == DayOfWeek.Saturday ? Brushes.Blue : item.Foreground;
                                var weekday = new Run(time.ToString("ddd")) { Foreground = color, FontWeight = FontWeights.Bold };
                                item.Inlines.AddRange(new Run[] { new Run("("), weekday, new Run(")") });
                            }
                        }
                        if (item.Height >= h3L) item.Inlines.Add(new LineBreak());
                        if (item.Height >= h6L) item.Inlines.Add(new LineBreak());
                        item.Inlines.Add(new Run(HourMod) { FontSize = 13, FontWeight = FontWeights.Bold });
                    }
                    else
                    {
                        item.Foreground = time.DayOfWeek == DayOfWeek.Sunday ? Brushes.Red : time.DayOfWeek == DayOfWeek.Saturday ? Brushes.Blue : CommonManager.Instance.TunerTimeFontColor;
                        item.Background = CommonManager.Instance.TunerTimeBackColor;
                        item.Height = 60 * Settings.Instance.TunerMinHeight - item.Margin.Top - item.Margin.Bottom;
                        item.Text = time.ToString("M/d\r\n" + (item.Height >= h3L ? "(ddd)\r\n" : ""))
                                                            + (item.Height >= h6L ? "\r\n" : "") + HourMod;
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
    }
}
