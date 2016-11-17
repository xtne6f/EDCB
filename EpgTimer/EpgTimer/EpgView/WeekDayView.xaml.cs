﻿using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;


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
            this.Background = CommonManager.Instance.EpgWeekdayBorderColor;
        }

        public void ClearInfo()
        {
            stackPanel_day.Children.Clear();
        }

        public void SetDay(List<DateTime> dayList)
        {
            try
            {
                stackPanel_day.Children.Clear();
                foreach (DateTime time in dayList)
                {
                    var item = ViewUtil.GetPanelTextBlock(time.ToString("M/d\r\n(ddd)"));
                    item.Width = Settings.Instance.ServiceWidth - 1;

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
                    item.Background = CommonManager._GetColorBrush(backgroundColor.ToString(), 0, Settings.Instance.EpgGradationHeader, 0.8, 1.2);

                    item.Padding = new Thickness(0, 0, 0, 2);
                    item.Margin = new Thickness(0, 1, 1, 1);
                    item.FontWeight = FontWeights.Bold;
                    stackPanel_day.Children.Add(item);
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
    }
}
