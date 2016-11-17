﻿using System;
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
                if (ReserveData.IsMultiple == true)
                {
                    return CommonManager.Instance.ResMultipleBackColor;
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
        public static bool ContainsContent(EpgEventInfo info, Dictionary<UInt16, UInt16> ContentKindList, bool notContent = false)
        {
            //絞り込み無し
            if (ContentKindList.Count == 0) return true;

            //ジャンルデータ'なし'扱い
            if (info.ContentInfo == null || info.ContentInfo.nibbleList.Count == 0)
            {
                return ContentKindList.ContainsKey(0xFFFF) == !notContent;
            }

            foreach (EpgContentData contentInfo in info.ContentInfo.nibbleList)
            {
                var ID1 = (UInt16)(contentInfo.content_nibble_level_1 << 8 | 0xFF);
                var ID2 = (UInt16)(contentInfo.content_nibble_level_1 << 8 | contentInfo.content_nibble_level_2);
                
                //CS検索の仮対応
                if (ID2 == 0x0E01)
                {
                    ID1 = (UInt16)((contentInfo.user_nibble_1 | 0x70) << 8 | 0xFF);
                    ID2 = (UInt16)((contentInfo.user_nibble_1 | 0x70) << 8 | contentInfo.user_nibble_2);
                }

                if (ContentKindList.ContainsKey(ID1) == true || ContentKindList.ContainsKey(ID2) == true)
                {
                    return !notContent;
                }
            }

            //見つからない
            return notContent;
        }

        public static void AddTimeList(ICollection<DateTime> timeList, DateTime startTime, UInt32 duration)
        {
            AddTimeList(timeList, startTime, startTime.AddSeconds(duration));
        }
        public static void AddTimeList(ICollection<DateTime> timeList, DateTime startTime, DateTime lastTime)
        {
            var chkStartTime = startTime.Date.AddHours(startTime.Hour); ;
            while (chkStartTime <= lastTime)
            {
                timeList.Add(chkStartTime);
                chkStartTime += TimeSpan.FromHours(1);
            }
        }

        public static void SetItemVerticalPos(List<DateTime> timeList, PanelItem item, DateTime startTime, UInt32 duration, double MinutesHeight, bool NeedTimeOnly)
        {
            item.Height = duration * MinutesHeight / 60;
            var chkStartTime = NeedTimeOnly == false ? timeList[0] : startTime.Date.AddHours(startTime.Hour);
            int offset = NeedTimeOnly == false ? 0 : 60 * timeList.BinarySearch(chkStartTime);
            if (offset >= 0)
            {
                item.TopPos = (offset + (startTime - chkStartTime).TotalMinutes) * MinutesHeight;
            }
        }

        //最低表示高さ
        public const double PanelMinimumHeight = 2;
        public static void ModifierMinimumLine(IEnumerable<PanelItem> list, double minimumLine, double fontHeight)
        {
            //list.Sort((x, y) => Math.Sign(x.LeftPos - y.LeftPos) * 2 + Math.Sign(x.TopPos - y.TopPos));
            double minimum = Math.Max((fontHeight + 2) * minimumLine, PanelMinimumHeight);
            double lastLeft = double.MinValue;
            double lastBottom = 0;
            foreach (PanelItem item in list.OrderBy(item => item.LeftPos * 1e6 + item.TopPos))
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

        public static void AdjustTimeList(IEnumerable<PanelItem> list, List<DateTime> timeList, double MinutesHeight)
        {
            if (list.Count() != 0 && timeList.Count > 0)
            {
                double bottom = list.Max(info => info.TopPos + info.Height);
                AddTimeList(timeList, timeList.Last().AddHours(1), timeList.First().AddMinutes(bottom / MinutesHeight));
            }
        }

        //指定アイテムまでマーキング付で移動する。
        public static void JumpToListItem(object target, ListBox listBox, bool IsMarking = false)
        {
            if (target is IGridViewSorterItem)
            {
                JumpToListItem(((IGridViewSorterItem)target).KeyID, listBox, IsMarking);
            }
            ScrollToFindItem(target, listBox, IsMarking);
        }
        public static void JumpToListItem(UInt64 gvSorterID, ListBox listBox, bool IsMarking = false)
        {
            var target = listBox.Items.OfType<IGridViewSorterItem>().FirstOrDefault(data => data.KeyID == gvSorterID);
            ScrollToFindItem(target, listBox, IsMarking);
        }
        public static void ScrollToFindItem(object target, ListBox listBox, bool IsMarking = false)
        {
            try
            {
                listBox.SelectedItem = target;
                if (listBox.SelectedItem == null) return;

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

        /*/未使用
        public static void DisableControlChildren(Control ctrl)
        {
            ctrl.Foreground = SystemColors.GrayTextBrush;
            ChangeChildren(ctrl, false);
        }
        /*/
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
                    uint sum = (uint)(onlist.Sum(info => info.DurationActual));
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

        public static string WindowTitleText(string contentTitle, string baseTitle)
        {
            return (string.IsNullOrEmpty(contentTitle) == true ? "" : contentTitle + " - ") + baseTitle;
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

        public static double GetScreenHeightMax()
        {
            return System.Windows.Forms.Screen.AllScreens.Max(sc => sc.WorkingArea.Height);
        }
        public static double GetScreenWidthMax()
        {
            return System.Windows.Forms.Screen.AllScreens.Max(sc => sc.WorkingArea.Width);
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
            block.MaxWidth = Settings.Instance.ToolTipWidth;
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

        //無効だけどテキストは選択出来るような感じ
        public static void SetReadOnlyWithEffect(this TextBox txtBox, bool val)
        {
            txtBox.IsReadOnly = val;
            txtBox.IsEnabled = true;
            if (val == true)
            {
                txtBox.Background = SystemColors.ControlBrush;//nullではないみたい;
                txtBox.Foreground = SystemColors.GrayTextBrush;
            }
            else
            {
                txtBox.ClearValue(TextBox.BackgroundProperty);
                txtBox.ClearValue(TextBox.ForegroundProperty);
            }
        }

        public static void AdjustWindowPosition(Window win)
        {
            foreach (var sc in System.Windows.Forms.Screen.AllScreens)
            {
                if (sc.Bounds.Contains((int)win.Left, (int)win.Top) == true)
                {
                    if (sc.WorkingArea.Contains((int)(win.Left + 100), (int)(win.Top + 100)) == true)
                    {
                        return;
                    }
                    break;
                }
            }
            win.Left = double.IsNaN(win.Left) == true ? double.NaN : 100;
            win.Top = double.IsNaN(win.Top) == true ? double.NaN : 100;
        }

        public static TextBlock GetPanelTextBlock(string s = null)
        {
            return new TextBlock
            {
                Text = s,
                TextAlignment = TextAlignment.Center,
                FontSize = 12,
            };
        }
    }
}
