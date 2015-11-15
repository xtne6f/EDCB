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
        public void ModifierMinimumHeight<T, S>(List<S> list, double MinLineHeight) where S : ViewPanelItem<T>
        {
            if (MinLineHeight <= 0) return;

            list.Sort((x, y) => Math.Sign(x.LeftPos - y.LeftPos) * 2 + Math.Sign(x.TopPos - y.TopPos));
            double minimum = (Settings.Instance.FontSizeTitle + 2) * MinLineHeight;
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

        public void ScrollToFindItem<T>(ViewPanelItem<T> target_item, ScrollViewer scrollViewer, Canvas canvas, bool IsMarking)
        {
            //TunerReserveViewとProgramViewをくくれたらそっちに移動する
            try
            {
                //可能性低いが0では無さそう
                if (target_item == null) return;

                scrollViewer.ScrollToHorizontalOffset(target_item.LeftPos - 100);
                scrollViewer.ScrollToVerticalOffset(target_item.TopPos - 100);

                //マーキング要求のあるとき
                if (IsMarking == true)
                {
                    Rectangle rect = new Rectangle();

                    rect.Stroke = new SolidColorBrush(Colors.Red);
                    rect.StrokeThickness = 5;
                    rect.Opacity = 1;
                    rect.Fill = System.Windows.Media.Brushes.Transparent;
                    rect.Effect = new System.Windows.Media.Effects.DropShadowEffect() { BlurRadius = 10 };

                    rect.Width = target_item.Width + 20;
                    rect.Height = target_item.Height + 20;
                    rect.IsHitTestVisible = false;

                    Canvas.SetLeft(rect, target_item.LeftPos - 10);
                    Canvas.SetTop(rect, target_item.TopPos - 10);
                    Canvas.SetZIndex(rect, 20);

                    // 一定時間枠を表示する
                    var notifyTimer = new System.Windows.Threading.DispatcherTimer();
                    notifyTimer.Interval = TimeSpan.FromSeconds(0.1);
                    TimeSpan RemainTime = TimeSpan.FromSeconds(Settings.Instance.DisplayNotifyJumpTime);
                    int Brinks = 3;
                    bool IsDisplay = false;
                    notifyTimer.Tick += (sender, e) =>
                    {
                        RemainTime -= notifyTimer.Interval;
                        if (RemainTime <= TimeSpan.FromSeconds(0))
                        {
                            canvas.Children.Remove(rect);
                            notifyTimer.Stop();
                        }
                        else if (IsDisplay == false)
                        {
                            canvas.Children.Add(rect);
                            IsDisplay = true;
                        }
                        else if (Brinks > 0)
                        {
                            canvas.Children.Remove(rect);
                            IsDisplay = false;
                            Brinks--;
                        }
                    };

                    notifyTimer.Start();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
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
