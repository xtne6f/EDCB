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
    public static class ViewUtil
    {
        public static MainWindow MainWindow { get { return (MainWindow)Application.Current.MainWindow; } }

        public static Brush EpgDataContentBrush(EpgEventInfo EventInfo)
        {
            if (EventInfo == null) return null;
            if (EventInfo.ContentInfo == null) return CommonManager.Instance.CustContentColorList[0x10];

            return EpgDataContentBrush(EventInfo.ContentInfo.nibbleList);
        }
        public static Brush EpgDataContentBrush(List<EpgContentData> nibbleList)
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

        public static Brush ReserveErrBrush(ReserveData ReserveData)
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

        public static void SetSpecificChgAppearance(Control obj)
        {
            obj.Background = Brushes.LavenderBlush;
            obj.BorderThickness = new Thickness(2);
        }

        public static bool ReloadReserveData(Control Owner = null)
        {
            if (CommonManager.Instance.IsConnected == false) return false;

            ErrCode err = CommonManager.Instance.DB.ReloadReserveInfo();
            if (CommonManager.CmdErrMsgTypical(err, "予約情報の取得") == false) return false;

            return true;
        }
        
        //ジャンル絞り込み
        public static bool ContainsContent(EpgEventInfo info, Dictionary<UInt16, UInt16> ContentKindList)
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
        public static void ApplyMarginForPanelView(ReserveData resInfo, ref DateTime startTime, ref int duration)
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

        public static void ApplyMarginForTunerPanelView(ReserveData resInfo, ref DateTime startTime, ref int duration)
        {
            int StartMargin = resInfo.RecSetting.GetTrueMargin(true);
            int EndMargin = resInfo.RecSetting.GetTrueMargin(false);

            startTime = resInfo.StartTime.AddSeconds(StartMargin * -1);
            duration = (int)resInfo.DurationSecond + StartMargin + EndMargin;
        }

        //最低表示高さ
        public const double PanelMinimumHeight = 2;

        //最低表示行数を適用。また、最低表示高さを確保して、位置も調整する。
        public static void ModifierMinimumLine<T, S>(List<S> list, double minimumLine, double fontHeight) where S : ViewPanelItem<T>
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

        public static void SetTimeList(List<ProgramViewItem> programList, SortedList<DateTime, List<ProgramViewItem>> timeList)
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

        //指定アイテムまでマーキング付で移動する。
        public static void JumpToListItem(object target, ListBox listBox, bool IsMarking)
        {
            if (target is DataListItemBase)
            {
                ulong ID = ((DataListItemBase)target).KeyID;
                target = listBox.Items.OfType<DataListItemBase>().FirstOrDefault(data => data.KeyID == ID);
            }
            ScrollToFindItem(target, listBox, IsMarking);
        }

        public static void ScrollToFindItem(object target, ListBox listBox, bool IsMarking)
        {
            try
            {
                listBox.SelectedItem = target;

                if (target == null) return;

                listBox.ScrollIntoView(target);

                //パネルビューと比較して、こちらでは最後までゆっくり点滅させる。全表示時間は同じ。
                //ただ、結局スクロールさせる位置がうまく調整できてないので効果は限定的。
                if (IsMarking == true && target is DataListItemBase)
                {
                    var target_item = target as DataListItemBase;
                    listBox.SelectedItem = null;

                    var notifyTimer = new DispatcherTimer();
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
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public static void DisableControlChildren(Control ctrl)
        {
            ctrl.Foreground = Brushes.Gray;
            ChangeChildren(ctrl, false);
        }
        public static void ChangeChildren(UIElement ele, bool enabled)
        {
            foreach (var child in LogicalTreeHelper.GetChildren(ele))
            {
                if (child is UIElement)
                {
                    (child as UIElement).IsEnabled = enabled;
                }
            }
        }

        /*/未使用
        public static DependencyObject SearchParentWpfTree(DependencyObject obj, Type t_trg, Type t_cut = null)
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
        /*/
        public static string ConvertSearchItemStatus(IEnumerable<SearchItem> list, string itemText = "番組数")
        {
            return string.Format("{0}:{1}", itemText, list.Count()) + ConvertReserveStatus(list, " 予約");
        }
        public static string ConvertReserveStatus(IEnumerable<SearchItem> list, string itemText = "予約数", int reserveMode = 0)
        {
            if (reserveMode == 0 && list.Count() == 0) return "";
            return ConvertReserveStatus(list.GetReserveList(), itemText, reserveMode);
        }
        public static string ConvertReserveStatus(List<ReserveData> rlist, string itemText = "予約数", int reserveMode = 0)
        {
            var text = string.Format("{0}:{1}", itemText, rlist.Count);
            List<ReserveData> onlist = rlist.FindAll(data => data.IsEnabled == true);
            if (reserveMode == 0 || (reserveMode != 3 && rlist.Count != onlist.Count))
            {
                text += string.Format(" (有効:{0} 無効:{1})", onlist.Count, rlist.Count - onlist.Count);
            }
            if (reserveMode != 0)
            {
                if (reserveMode <= 2)
                {
                    uint sum = (uint)(onlist.Sum(info => info.DurationSecond
                        + info.RecSetting.GetTrueMargin(true) + info.RecSetting.GetTrueMargin(false)));
                    text += (reserveMode == 1 ? " 総録画時間:" : " 録画時間:")
                            + CommonManager.ConvertDurationText(sum, false);
                }
                else
                {
                    long errs = onlist.Count(item => item.OverlapMode == 2);
                    long warns = onlist.Count(item => item.OverlapMode == 1);
                    if (Settings.Instance.TunerDisplayOffReserve == true)
                    {
                        long off = rlist.Count - onlist.Count;
                        text += string.Format(" (チューナー不足:{0} 一部録画:{1} 無効予約:{2})", errs, warns, off);
                    }
                    else
                    {
                        text += string.Format(" (チューナー不足:{0} 一部録画:{1})", errs, warns);
                    }
                }
            }
            return text;
        }
        public static string ConvertRecinfoStatus(IEnumerable<RecInfoItem> list, string itemText = "録画結果")
        {
            var format = "{0}:{1} ({2}:{3} {4}:{5})";
            if (Settings.Instance.RecinfoErrCriticalDrops == true)
            {
                return string.Format(format, itemText, list.Count(),
                    "*Drop", list.Sum(item => item.RecInfo.DropsCritical),
                    "*Scramble", list.Sum(item => item.RecInfo.ScramblesCritical));
            }
            else
            {
                return string.Format(format, itemText, list.Count(),
                    "Drop", list.Sum(item => item.RecInfo.Drops),
                    "Scramble", list.Sum(item => item.RecInfo.Scrambles));
            }
        }
        public static string ConvertAutoAddStatus(IEnumerable<AutoAddDataItem> list, string itemText = "自動予約登録数")
        {
            var onRes = new List<uint>();
            var offRes = new List<uint>();
            foreach (var rlist in list.Select(data => data.Data.GetReserveList()))
            {
                onRes.AddRange(rlist.Where(item => item.IsEnabled == true).Select(res => res.ReserveID));
                offRes.AddRange(rlist.Where(item => item.IsEnabled == false).Select(res => res.ReserveID));
            }
            return string.Format("{0}:{1} (有効予約数:{2} 無効予約数:{3})",
                itemText, list.Count(), onRes.Distinct().Count(), offRes.Distinct().Count());
        }
        public static string ConvertInfoSearchItemStatus(IEnumerable<InfoSearchItem> list, string itemText)
        {
            string det = "";
            foreach (var key in InfoSearchItem.ViewTypeNameList())
            {
                int num = list.Count(item => item.ViewItemName == key);
                if (num != 0)
                {
                    det += string.Format("{0}:{1} ", key.Substring(0, 2), num);
                }
            }
            return string.Format("{0}:{1}", itemText, list.Count()) + (det == "" ? "" : " (" + det.TrimEnd() + ")");
        }

        public static Type GetListBoxItemType(ListBox lb)
        {
            if (lb == null) return null;
            //場合分けしないで取得する方法があるはず
            return lb is ListView ? typeof(ListViewItem) : lb is ListBox ? typeof(ListBoxItem) : typeof(object);
        }
        public static void ResetItemContainerStyle(ListBox lb)
        {
            try
            {
                if (lb == null) return;
                if (lb.ItemContainerStyle != null && lb.ItemContainerStyle.IsSealed == false) return;

                //IsSealedが設定されているので、作り直す。
                var newStyle = new Style();

                //baseStyleに元のItemContainerStyle放り込むと一見簡単だが、ここはちゃんと内容を移す。
                if (lb.ItemContainerStyle != null)
                {
                    newStyle.TargetType = lb.ItemContainerStyle.TargetType;//余り意味は無いはずだが一応
                    newStyle.BasedOn = lb.ItemContainerStyle.BasedOn;//nullならそのままnullにしておく
                    newStyle.Resources = lb.ItemContainerStyle.Resources;
                    foreach (var item in lb.ItemContainerStyle.Setters) newStyle.Setters.Add(item);
                    foreach (var item in lb.ItemContainerStyle.Triggers) newStyle.Triggers.Add(item);
                }
                else
                {
                    newStyle.TargetType = GetListBoxItemType(lb);
                    //現在のContainerTypeを引っ張る。ただし、アプリケーションリソースでListViewItemが定義されている前提。
                    try { newStyle.BasedOn = (Style)lb.FindResource(newStyle.TargetType); }
                    catch { }
                }
                lb.ItemContainerStyle = newStyle;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public static SelectionChangedEventHandler ListBox_TextBoxSyncSelectionChanged(ListBox lstBox, TextBox txtBox)
        {
            return new SelectionChangedEventHandler((sender, e) =>
            {
                if (lstBox == null || lstBox.SelectedItem == null || txtBox == null) return;
                //
                txtBox.Text = lstBox.SelectedItem.ToString();
            });
        }

        public static RoutedEventHandler ListBox_TextCheckAdd(ListBox lstBox, TextBox txtBox)
        {
            return new RoutedEventHandler((sender, e) => ListBox_TextCheckAdd(lstBox, txtBox == null ? null : txtBox.Text));
        }
        public static void ListBox_TextCheckAdd(ListBox lstBox, string text)
        {
            if (lstBox == null || String.IsNullOrEmpty(text) == true) return;
            //
            if (lstBox.Items.OfType<object>().Any(s => String.Compare(text, s.ToString(), true) == 0) == true)
            {
                MessageBox.Show("すでに追加されています");
                return;
            }
            lstBox.Items.Add(text);
        }

        public static KeyEventHandler KeyDown_Escape_Close()
        {
            return new KeyEventHandler((sender, e) =>
            {
                if (e.Handled == false && Keyboard.Modifiers == ModifierKeys.None && e.Key == Key.Escape)
                {
                    e.Handled = true;
                    var win = CommonUtil.GetTopWindow(sender as Visual);
                    if (win != null) win.Close();
                }
            });
        }

        public static KeyEventHandler KeyDown_Enter(Button btn)
        {
            return new KeyEventHandler((sender, e) =>
            {
                if (e.Handled == false && Keyboard.Modifiers == ModifierKeys.None && e.Key == Key.Enter)
                {
                    e.Handled = true;
                    if (btn != null) btn.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                }
            });
        }

        ///<summary>同じアイテムがあってもスクロールするようにしたもの(ItemSource使用時無効)</summary>
        //ScrollIntoView()は同じアイテムが複数あると上手く動作しないので、ダミーを使って無理矢理移動させる。
        //同じ理由でSelectedItemも正しく動作しないので、スクロール位置はindexで取るようにする。
        public static void ScrollIntoViewIndex(this ListBox box, int index)
        {
            try
            {
                if (box == null || box.Items.Count == 0) return;

                index = Math.Min(Math.Max(0, index), box.Items.Count - 1);
                object item = box.Items[index];

                //リストに追加・削除をするので、ItemsSourceなどあるときは動作しない
                if (box.ItemsSource == null)
                {
                    if (box.Items.IndexOf(item) != index)
                    {
                        item = new ListBoxItem { Visibility = Visibility.Collapsed };
                        box.Items.Insert(index == 0 ? 0 : index + 1, item);

                        //ScrollIntoView()は遅延して実行されるので、実行後にダミーを削除する。
                        Dispatcher.CurrentDispatcher.BeginInvoke(new Action(() => box.Items.Remove(item)), DispatcherPriority.Loaded);
                    }
                }

                box.ScrollIntoView(item);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public static DependencyObject GetPlacementItem(this ItemsControl lb, Point? pt = null)
        {
            if (lb == null) return null;
            var element = lb.InputHitTest((Point)(pt ?? Mouse.GetPosition(lb))) as DependencyObject;
            return element == null ? null : lb.ContainerFromElement(element);
        }

        public static int SingleWindowCheck(Type t, bool closeWindow = false)
        {
            var wList = Application.Current.Windows.OfType<Window>().Where(w => w.GetType() == t);
            foreach (var w in wList)
            {
                if (closeWindow == true)
                {
                    w.Close();
                }
                else
                {
                    if (w.WindowState == WindowState.Minimized)
                    {
                        w.WindowState = WindowState.Normal;
                    }
                    w.Visibility = Visibility.Visible;
                    w.Activate();
                }
            }
            return wList.Count();
        }

        public static TextBlock GetTooltipBlockStandard(string text)
        {
            var block = new TextBlock();
            block.Text = text;
            block.MaxWidth = 400;
            block.TextWrapping = TextWrapping.Wrap;
            return block;
        }

        public static void RenameHeader(this IEnumerable<GridViewColumn> list, string uid, object title, string tag = null)
        {
            foreach(var item in list)
            {
                if(item.Header is GridViewColumnHeader)
                {
                    var header = item.Header as GridViewColumnHeader;
                    if (header.Uid == uid)
                    {
                        header.Content = title;
                        if (tag != null) header.Tag = tag;
                    }
                }
            }
        }
    }
}
