using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Threading;

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

        public Brush EpgDataContentBrush(EpgEventInfo EventInfo)
        {
            if (EventInfo == null) return Brushes.White;
            if (EventInfo.ContentInfo == null) return CommonManager.Instance.CustContentColorList[0x10];

            return EpgDataContentBrush(EventInfo.ContentInfo.nibbleList);
        }
        public Brush EpgDataContentBrush(List<EpgContentData> nibbleList)
        {
            if (nibbleList != null)
            {
                EpgContentData info = nibbleList.Find(info1 =>
                    info1.content_nibble_level_1 <= 0x0B || info1.content_nibble_level_1 == 0x0F);

                if (info != null)
                {
                    return CommonManager.Instance.CustContentColorList[info.content_nibble_level_1];
                }
            }
            return CommonManager.Instance.CustContentColorList[0x10];
        }

        public SolidColorBrush ReserveErrBrush(ReserveData ReserveData)
        {
            if (ReserveData != null)
            {
                if (ReserveData.RecSetting.RecMode == 5)
                {
                    return CommonManager.Instance.ResNoBackColor;
                }
                if (ReserveData.OverlapMode == 2)
                {
                    return CommonManager.Instance.ResErrBackColor;
                }
                if (ReserveData.OverlapMode == 1)
                {
                    return CommonManager.Instance.ResWarBackColor;
                }
                if (ReserveData.IsAutoAddMissing() == true)
                {
                    return CommonManager.Instance.ResAutoAddMissingBackColor;
                }
            }
            return CommonManager.Instance.ResDefBackColor;
        }
        
        public void SetSpecificChgAppearance(Control obj)
        {
            obj.Background = Brushes.LavenderBlush;
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

        public void view_PreviewMouseWheel<T>(object sender, MouseWheelEventArgs e, ScrollViewer scrollViewer, bool auto, double scrollSize)
        {
            try
            {
                e.Handled = true;
                if (sender.GetType() == typeof(T))
                {
                    if (auto == true)
                    {
                        scrollViewer.ScrollToVerticalOffset(scrollViewer.VerticalOffset - e.Delta);
                    }
                    else
                    {
                        if (e.Delta < 0)
                        {
                            //下方向
                            scrollViewer.ScrollToVerticalOffset(scrollViewer.VerticalOffset + scrollSize);
                        }
                        else
                        {
                            //上方向
                            if (scrollViewer.VerticalOffset < scrollSize)
                            {
                                scrollViewer.ScrollToVerticalOffset(0);
                            }
                            else
                            {
                                scrollViewer.ScrollToVerticalOffset(scrollViewer.VerticalOffset - scrollSize);
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

            //ジャンルデータ'なし'扱い
            if (info.ContentInfo == null || info.ContentInfo.nibbleList.Count == 0)
            {
                return ContentKindList.ContainsKey(0xFFFF) == true;
            }

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
        public void ApplyMarginForPanelView(ReserveData resInfo, ref DateTime startTime, ref int duration)
        {
            int StartMargin = mutil.GetMargin(resInfo.RecSetting, true);
            int EndMargin = mutil.GetMargin(resInfo.RecSetting, false);

            if (StartMargin < 0)
            {
                //メモ:番組長より長いマイナスマージンの扱いは?
                startTime = startTime.AddSeconds(StartMargin * -1);
                duration += StartMargin;
            }
            if (EndMargin < 0)
            {
                duration += EndMargin;
            }
        }

        public void ApplyMarginForTunerPanelView(ReserveData resInfo, ref DateTime startTime, ref int duration)
        {
            int StartMargin = mutil.GetMargin(resInfo.RecSetting, true);
            int EndMargin = mutil.GetMargin(resInfo.RecSetting, false);

            startTime = resInfo.StartTime.AddSeconds(StartMargin * -1);
            duration = (int)resInfo.DurationSecond + StartMargin + EndMargin;
        }

        public GlyphTypeface GetGlyphTypeface(string fontName, bool isBold)
        {
            try
            {
                var fontWeights = (isBold == true ? FontWeights.Bold : FontWeights.Normal);
                Typeface typeface = new Typeface(new FontFamily(fontName),
                                                FontStyles.Normal, fontWeights, FontStretches.Normal);

                GlyphTypeface glyphTypeface;
                if (typeface.TryGetGlyphTypeface(out glyphTypeface) == false)
                {
                    typeface = new Typeface(new FontFamily(System.Drawing.SystemFonts.DefaultFont.Name),
                                                    FontStyles.Normal, fontWeights, FontStretches.Normal);

                    if (typeface.TryGetGlyphTypeface(out glyphTypeface) == false)
                    {
                        MessageBox.Show("フォント指定が不正です");
                        return null;
                    }
                }
                return glyphTypeface;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return null;
            }
        }

        //最低表示高さ
        public const double PanelMinimumHeight = 2;

        //最低表示行数を適用。また、最低表示高さを確保して、位置も調整する。
        public void ModifierMinimumLine<T, S>(List<S> list, double minimumLine, double fontHeight) where S : ViewPanelItem<T>
        {
            list.Sort((x, y) => Math.Sign(x.LeftPos - y.LeftPos) * 2 + Math.Sign(x.TopPos - y.TopPos));
            double minimum = Math.Max((fontHeight + 2) * minimumLine, PanelMinimumHeight);
            double lastLeft = double.MinValue;
            double lastBottom = 0;
            foreach (S item in list)
            {
                if (lastLeft != item.LeftPos)
                {
                    lastLeft = item.LeftPos;
                    lastBottom = double.MinValue;
                }
                if (item.TopPos < lastBottom)
                {
                    item.Height = Math.Max(item.TopPos + item.Height - lastBottom, minimum);
                    item.TopPos = lastBottom;
                }
                else
                {
                    item.Height = Math.Max(item.Height, minimum);
                }
                lastBottom = item.TopPos + item.Height;
            }
        }

        public void ScrollToFindItem(SearchItem target_item, ListBox listBox, bool IsMarking)
        {
            try
            {
                ScrollToItem(target_item, listBox);

                //パネルビューと比較して、こちらでは最後までゆっくり点滅させる。全表示時間は同じ。
                //ただ、結局スクロールさせる位置がうまく調整できてないので効果は限定的。
                if (IsMarking == true)
                {
                    listBox.SelectedItem = null;

                    var notifyTimer = new System.Windows.Threading.DispatcherTimer();
                    notifyTimer.Interval = TimeSpan.FromSeconds(0.2);
                    TimeSpan RemainTime = TimeSpan.FromSeconds(Settings.Instance.DisplayNotifyJumpTime);
                    notifyTimer.Tick += (sender, e) =>
                    {
                        RemainTime -= notifyTimer.Interval;
                        if (RemainTime <= TimeSpan.FromSeconds(0))
                        {
                            target_item.NowJumpingTable = 0;
                            listBox.SelectedItem = target_item;
                            notifyTimer.Stop();
                        }
                        else
                        {
                            target_item.NowJumpingTable = target_item.NowJumpingTable != 1 ? 1 : 2;
                        }
                        listBox.Items.Refresh();
                    };
                    notifyTimer.Start();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void ScrollToItem(object target_item, ListBox listBox)
        {
            try
            {
                if (target_item == null) return;

                listBox.SelectedItem = target_item;
                listBox.ScrollIntoView(target_item);

                //いまいちな感じ
                //listView_event.ScrollIntoView(listView_event.Items[0]);
                //listView_event.ScrollIntoView(listView_event.Items[listView_event.Items.Count-1]);
                //int scrollpos = ((listView_event.SelectedIndex - 5) >= 0 ? listView_event.SelectedIndex - 5 : 0);
                //listView_event.ScrollIntoView(listView_event.Items[scrollpos]);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void DisableControlChildren(Control ctrl)
        {
            ctrl.Foreground = Brushes.Gray;
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

    public static class ViewUtilEx
    {
        ///<summary>同じアイテムがあってもスクロールするようにしたもの(ItemSource使用時無効)</summary>
        //ScrollIntoView()は同じアイテムが複数あると上手く動作しないので、ダミーを使って無理矢理移動させる。
        //同じ理由でSelectedItemも正しく動作しないので、スクロール位置はindexで取るようにする。
        public static void ScrollIntoViewFix(this ListBox box, int index)
        {
            try
            {
                if (box == null || index < 0 || index >= box.Items.Count) return;

                //リストに追加・削除をするので、ItemsSourceなどあるときは動作しない
                if (box.ItemsSource == null)
                {
                    object key = box.Items[index];
                    if (box.Items.OfType<object>().Count(item => item.Equals(key)) != 1)//==は失敗する
                    {
                        var dummy = new ListBoxItem();
                        dummy.Visibility = Visibility.Collapsed;//まだスクロールバーがピクピクする
                        box.Items.Insert(index + (index == 0 ? 0 : 1), dummy);
                        box.ScrollIntoView(dummy);

                        //ScrollIntoView()は遅延して実行されるので、実行後にダミーを削除する。
                        Dispatcher.CurrentDispatcher.BeginInvoke(new Action(() => box.Items.Remove(dummy)), DispatcherPriority.ContextIdle);

                        return;
                    }
                }

                box.ScrollIntoView(box.Items[index]);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
    }
}
