using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace EpgTimer
{
    public class ViewUtil
    {
        private CtrlCmdUtil cmd = null;
        private MenuUtil mutil = null;

        public ViewUtil(CtrlCmdUtil ctrlCmd, MenuUtil MUtil)
        {
            cmd = ctrlCmd;
            mutil = MUtil;
        }

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

        public void SetSpecificChgAppearance(Control obj)
        {
            obj.Background = new SolidColorBrush(Colors.LavenderBlush);
            //obj.BorderBrush = new SolidColorBrush(Colors.Red);
            obj.BorderThickness = new Thickness(2);
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

        public bool ReloadReserveData(Control Owner = null)
        {
            if (EpgTimerNWNotConnect() == true) return false;

            ErrCode err = CommonManager.Instance.DB.ReloadReserveInfo();
            if (CommonManager.CmdErrMsgTypical(err, "予約情報の取得", Owner) == false) return false;

            return true;
        }
        
        public bool EpgTimerNWNotConnect()
        {
            return CommonManager.Instance.NWMode == true && CommonManager.Instance.NW.IsConnected == false;
        }

        //ジャンル絞り込み
        public bool ContainsContent(EpgEventInfo info, Dictionary<UInt16, UInt16> ContentKindList)
        {
            //絞り込み無し
            if (ContentKindList.Count == 0) return true;

            //ジャンルデータなしは該当無し扱い
            if (info.ContentInfo == null || info.ContentInfo.nibbleList.Count == 0) return false;

            foreach (EpgContentData contentInfo in info.ContentInfo.nibbleList)
            {
                UInt16 ID1 = (UInt16)(((UInt16)contentInfo.content_nibble_level_1) << 8 | 0xFF);
                UInt16 ID2 = (UInt16)(((UInt16)contentInfo.content_nibble_level_1) << 8 | contentInfo.content_nibble_level_2);
                
                //CS検索の仮対応
                if (ID2 == 0x0e01)
                {
                    ID1 = (UInt16)(((UInt16)contentInfo.user_nibble_1) << 8 | 0x70FF);
                    ID2 = (UInt16)(((UInt16)contentInfo.user_nibble_1) << 8 | 0x7000 | contentInfo.user_nibble_2);
                }

                if (ContentKindList.ContainsKey(ID1) == true)
                {
                    return true;
                }
                else if (ContentKindList.ContainsKey(ID2) == true)
                {
                    return true;
                }
            }

            //見つからない
            return false;
        }

        //パネルアイテムにマージンを適用。
        public void ApplyMarginForPanelView(ReserveData resInfo,
            ref int duration, ref DateTime startTime, bool already_set = false)
        {
            if (already_set == false)
            {
                duration = (Int32)resInfo.DurationSecond;
                startTime = resInfo.StartTime;
            }
            int StartMargine = mutil.GetMargin(resInfo.RecSetting, true);
            int EndMargine = mutil.GetMargin(resInfo.RecSetting, false);

            if (StartMargine < 0)
            {
                startTime = startTime.AddSeconds(StartMargine * -1);
                duration += StartMargine;
            }
            if (EndMargine < 0)
            {
                duration += EndMargine;
            }
        }

        //最低表示行数を適用
        public void ModifierMinimumHeight<T, S>(List<S> list) where S : ViewPanelItem<T>
        {
            if (Settings.Instance.MinimumHeight <= 0) return;

            list.Sort((x, y) => Math.Sign(x.LeftPos - y.LeftPos) * 2 + Math.Sign(x.TopPos - y.TopPos));
            double minimum = (Settings.Instance.FontSizeTitle + 2) * Settings.Instance.MinimumHeight;
            double lastLeft = double.MinValue;
            double lastBottom = 0;
            foreach (S item in list)
            {
                if (lastLeft != item.LeftPos)
                {
                    lastLeft = item.LeftPos;
                    lastBottom = double.MinValue;
                }
                item.Height = Math.Max(item.Height, minimum);
                if (item.TopPos < lastBottom)
                {
                    item.Height = Math.Max(item.TopPos + item.Height - lastBottom, minimum);
                    item.TopPos = lastBottom;
                }
                lastBottom = item.TopPos + item.Height;
            }
        }

        public void DisableControlChildren(Control ctrl)
        {
            ctrl.Foreground = new SolidColorBrush(Colors.Gray);
            ChangeChildren(ctrl, false);
        }
        public void ChangeChildren(UIElement ele, bool enabled)
        {
            foreach (var child in LogicalTreeHelper.GetChildren(ele))
            {
                if (child is UIElement)
                {
                    (child as UIElement).IsEnabled = enabled;
                }
            }
        }
    }
}
