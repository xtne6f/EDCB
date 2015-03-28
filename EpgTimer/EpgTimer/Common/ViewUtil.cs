using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public class ViewUtil
    {
        public Brush EventDataBorderBrush(EpgEventInfo EventInfo)
        {
            Brush color1 = Brushes.White;
            if (EventInfo != null)
            {
                if (EventInfo.ContentInfo != null)
                {
                    if (EventInfo.ContentInfo.nibbleList.Count > 0)
                    {
                        try
                        {
                            foreach (EpgContentData info1 in EventInfo.ContentInfo.nibbleList)
                            {
                                if (info1.content_nibble_level_1 <= 0x0B || info1.content_nibble_level_1 == 0x0F && Settings.Instance.ContentColorList.Count > info1.content_nibble_level_1)
                                {
                                    color1 = CommonManager.Instance.CustContentColorList[info1.content_nibble_level_1];
                                    break;
                                }
                            }
                        }
                        catch
                        {
                        }
                    }
                    else
                    {
                        color1 = CommonManager.Instance.CustContentColorList[0x10];
                    }
                }
                else
                {
                    color1 = CommonManager.Instance.CustContentColorList[0x10];
                }
            }

            return color1;
        }

        public void view_ScrollChanged<T>(object sender, ScrollChangedEventArgs e, ScrollViewer main_scroll, ScrollViewer v_scroll, ScrollViewer h_scroll)
        {
            try
            {
                if (sender.GetType() == typeof(T))
                {
                    //時間軸の表示もスクロール
                    v_scroll.ScrollToVerticalOffset(main_scroll.VerticalOffset);
                    //サービス名表示もスクロール
                    h_scroll.ScrollToHorizontalOffset(main_scroll.HorizontalOffset);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void view_PreviewMouseWheel<T>(object sender, MouseWheelEventArgs e, ScrollViewer scrollViewer)
        {
            try
            {
                e.Handled = true;
                if (sender.GetType() == typeof(T))
                {
                    if (Settings.Instance.MouseScrollAuto == true)
                    {
                        scrollViewer.ScrollToVerticalOffset(scrollViewer.VerticalOffset - e.Delta);
                    }
                    else
                    {
                        if (e.Delta < 0)
                        {
                            //下方向
                            scrollViewer.ScrollToVerticalOffset(scrollViewer.VerticalOffset + Settings.Instance.ScrollSize);
                        }
                        else
                        {
                            //上方向
                            if (scrollViewer.VerticalOffset < Settings.Instance.ScrollSize)
                            {
                                scrollViewer.ScrollToVerticalOffset(0);
                            }
                            else
                            {
                                scrollViewer.ScrollToVerticalOffset(scrollViewer.VerticalOffset - Settings.Instance.ScrollSize);
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public bool GetProgramItem(Point cursorPos, ref EpgEventInfo data, SortedList<DateTime, List<ProgramViewItem>> timeList)
        {
            try
            {
                int timeIndex = (int)(cursorPos.Y / (60 * Settings.Instance.MinHeight));
                if (0 <= timeIndex && timeIndex < timeList.Count)
                {
                    return GetHitItem<EpgEventInfo, ProgramViewItem>(cursorPos, ref data, timeList.Values[timeIndex]);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return false;
        }
        public bool GetReserveItem(Point cursorPos, ref ReserveData data, List<ReserveViewItem> list)
        {
            return GetHitItem<ReserveData, ReserveViewItem>(cursorPos, ref data, list);
        }
        public bool GetHitItem<T, S>(Point cursorPos, ref T data, List<S> list) where S : ViewPanelItem<T>
        {
            try
            {
                int idx = list.FindIndex(info => info == null ? false : info.IsPicked(cursorPos));
                if (idx >= 0)
                {
                    data = (T)list[idx]._Data;
                }
                return idx >= 0;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return false;
        }

    }
}
