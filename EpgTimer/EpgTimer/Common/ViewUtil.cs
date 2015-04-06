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

        public bool ReloadReserveData(ContentControl Owner = null)
        {
            if (EpgTimerConnectCheck() == false) return false;

            ErrCode err = CommonManager.Instance.DB.ReloadReserveInfo();
            if (CommonManager.CmdErrMsgTypical(err, "予約情報の取得", Owner) == false) return false;

            return true;
        }

        public bool ReloadEpgData(ContentControl Owner = null)
        {
            if (EpgTimerConnectCheck() == false) return false;

            ErrCode err = CommonManager.Instance.DB.ReloadEpgData();
            if (CommonManager.CmdErrMsgTypical(err, "EPGデータの取得", Owner) == false) return false;

            return true;
        }

        //仮
        public Dictionary<UInt64, EpgServiceEventInfo> ReloadEpgDataForEpgView(CustomEpgTabInfo setViewInfo, ContentControl Owner = null)
        {
            if (EpgTimerConnectCheck() == false) return null;

            if (setViewInfo.SearchMode == false)
            {
                ErrCode err = CommonManager.Instance.DB.ReloadEpgData();
                if (CommonManager.CmdErrMsgTypical(err, "EPGデータの取得", Owner) == false) return null;

                return CommonManager.Instance.DB.ServiceEventList;
            }
            else
            {
                //番組情報の検索
                List<EpgSearchKeyInfo> keyList = mutil.GetList(setViewInfo.SearchKey);
                var list = new List<EpgEventInfo>();

                ErrCode err = (ErrCode)cmd.SendSearchPg(keyList, ref list);
                if (CommonManager.CmdErrMsgTypical(err, "EPGデータの取得", Owner) == false) return null;

                //サービス毎のリストに変換
                var serviceEventList = new Dictionary<UInt64, EpgServiceEventInfo>();
                foreach (EpgEventInfo eventInfo in list)
                {
                    UInt64 id = eventInfo.Create64Key();
                    EpgServiceEventInfo serviceInfo;
                    if (serviceEventList.TryGetValue(id, out serviceInfo) == false)
                    {
                        if (ChSet5.Instance.ChList.ContainsKey(id) == false)
                        {
                            //サービス情報ないので無効
                            continue;
                        }
                        serviceInfo = new EpgServiceEventInfo();
                        serviceInfo.serviceInfo = CommonManager.ConvertChSet5To(ChSet5.Instance.ChList[id]);

                        serviceEventList.Add(id, serviceInfo);
                    }
                    serviceInfo.eventList.Add(eventInfo);
                }
                return serviceEventList;
            }
        }

        public bool EpgTimerConnectCheck()
        {
            return CommonManager.Instance.NWMode == false || CommonManager.Instance.NW.IsConnected == true;
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

    }
}
