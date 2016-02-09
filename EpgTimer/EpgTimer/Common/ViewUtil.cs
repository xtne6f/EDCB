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
using System.Reflection;

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
                    info1.content_nibble_level_1 <= 0x0B || info1.content_nibble_level_1 == 0x0E || info1.content_nibble_level_1 == 0x0F);

                if (info != null)
                {
                    if (info.content_nibble_level_1 == 0x0E && info.content_nibble_level_2 == 0x01)
                    {
                        //CSのコード置き換え。通常は一般のジャンル情報も付いているので、効果は薄いかも。
                        switch (info.user_nibble_1)
                        {
                            case 0x00: return CommonManager.Instance.CustContentColorList[0x01];//スポーツ(CS)→スポーツ
                            case 0x01: return CommonManager.Instance.CustContentColorList[0x06];//洋画(CS)→映画
                            case 0x02: return CommonManager.Instance.CustContentColorList[0x06];//邦画(CS)→映画
                        }
                        //ラストへ
                    }
                    else
                    {
                        return CommonManager.Instance.CustContentColorList[info.content_nibble_level_1];
                    }
                }
            }
            return CommonManager.Instance.CustContentColorList[0x10];
        }

        public SolidColorBrush ReserveErrBrush(ReserveData ReserveData)
        {
            if (ReserveData != null)
            {
                if (ReserveData.IsEnabled == false)
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
                if (ReserveData.IsAutoAddInvalid == true)
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

        public bool ReloadReserveData(Control Owner = null)
        {
            if (CommonManager.Instance.IsConnected == false) return false;

            ErrCode err = CommonManager.Instance.DB.ReloadReserveInfo();
            if (CommonManager.CmdErrMsgTypical(err, "予約情報の取得", Owner) == false) return false;

            return true;
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
            int StartMargin = resInfo.RecSetting.GetTrueMargin(true);
            int EndMargin = resInfo.RecSetting.GetTrueMargin(false);

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
            int StartMargin = resInfo.RecSetting.GetTrueMargin(true);
            int EndMargin = resInfo.RecSetting.GetTrueMargin(false);

            startTime = resInfo.StartTime.AddSeconds(StartMargin * -1);
            duration = (int)resInfo.DurationSecond + StartMargin + EndMargin;
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

        public void SetTimeList(List<ProgramViewItem> programList, SortedList<DateTime, List<ProgramViewItem>> timeList)
        {
            foreach (ProgramViewItem item in programList)
            {
                double top = Math.Min(item.TopPos, item.TopPosDef);
                double bottom = Math.Max(item.TopPos + item.Height, item.TopPosDef + item.HeightDef);
                int index = Math.Max((int)(top / (60 * Settings.Instance.MinHeight)), 0);
                int end = (int)(bottom / (60 * Settings.Instance.MinHeight)) + 1;

                //必要時間リストの修正。最低表示行数の適用で下に溢れた分を追加する。
                while (end > timeList.Count)
                {
                    DateTime time_tail = timeList.Keys[timeList.Count - 1].AddHours(1);
                    timeList.Add(time_tail, new List<ProgramViewItem>());
                }

                //必要時間リストと時間と番組の関連づけ。
                while (index < end)
                {
                    timeList.Values[index++].Add(item);
                }
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
                    var sw = System.Diagnostics.Stopwatch.StartNew();
                    notifyTimer.Tick += (sender, e) =>
                    {
                        if (sw.ElapsedMilliseconds > Settings.Instance.DisplayNotifyJumpTime * 1000)
                        {
                            notifyTimer.Stop();
                            target_item.NowJumpingTable = 0;
                            listBox.SelectedItem = target_item;
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

        public DependencyObject SearchParentWpfTree(DependencyObject obj, Type t_trg, Type t_cut = null)
        {
            Func<string, DependencyObject> GetParentFromProperty = name =>
            {
                PropertyInfo p = obj.GetType().GetProperty(name);
                return p == null ? null : p.GetValue(obj, null) as DependencyObject;
            };
            while (true)
            {
                if (obj == null || obj.GetType() == t_trg) return obj;
                if (obj.GetType() == t_cut) return null;

                DependencyObject trg = GetParentFromProperty("TemplatedParent");
                if (trg == null) trg = GetParentFromProperty("Parent");//次の行と同じ？
                if (trg == null) trg = LogicalTreeHelper.GetParent(obj);
                obj = trg;
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
