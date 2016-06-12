using System;
using System.Collections.Generic;
using System.Linq;
using System.Collections;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;

namespace EpgTimer.BoxExchangeEdit
{
    using BoxExchangeEditAux;

    class BoxExchangeEditor
    {
        //ListBox同士でアイテムを交換する。
        //・ItemsSource使用中のTargetBoxでも動かせるが、ItemsSourceのメリットは全く享受出来ない。
        //・重複処理が必要な場合はそれなりに準備が必要。文字列の重複処理がうまく出来ないのはListBoxと同様。

        public BoxExchangeEditor() { }
        public BoxExchangeEditor(ListBox src, ListBox trg, bool allowCancelAction = false, bool allowDragDrop = false, bool allowKeyAction = false, bool allowDoubleClickMove = false)
        {
            SourceBox = src;
            TargetBox = trg;
            if (allowCancelAction == true) this.AllowCancelAction();
            if (allowDragDrop == true) this.AllowDragDrop();
            if (allowKeyAction == true) this.AllowKeyAction();
            if (allowDoubleClickMove == true) this.AllowDoubleClickMove();
        }

        public ListBox SourceBox { set; get; }
        public ListBox TargetBox { set; get; }
        public IList TargetItemsSource { set; get; }

        public bool DuplicationAllAllowed { private set; get; }//項目の重複を全て許可
        public IEnumerable<object> DuplicationSpecific { private set; get; }//特定の項目のみ重複を許可
        public Func<object, object> ItemDuplicator { private set; get; }//重複を許可する場合必要
        public IEqualityComparer<object> ItemComparer { private set; get; }//重複を許可する場合必要

        /// <summary>重複を許可。supecific未指定なら全て許可。きちんと動かすなら、duplicator、comparerが必要。</summary>
        public void AllowDuplication(IEnumerable<object> supecific = null, Func<object, object> duplicator = null, IEqualityComparer<object> comparer = null)
        {
            DuplicationAllAllowed = supecific == null;
            DuplicationSpecific = supecific;
            ItemDuplicator = duplicator;
            ItemComparer = comparer;
        }

        /// <summary>ソース側のEnterによる追加、ターゲット側のDeleteによる削除を有効にする</summary>
        public void AllowKeyAction()
        {
            sourceBoxAllowKeyAction(SourceBox);
            targetBoxAllowKeyAction(TargetBox);
        }
        public void sourceBoxAllowKeyAction(ListBox box, KeyEventHandler add_handler = null)
        {
            if (box == null) return;
            //
            box.PreviewKeyDown += new KeyEventHandler((sender, e) =>
            {
                if (Keyboard.Modifiers != ModifierKeys.None) return;
                //
                switch (e.Key)
                {
                    case Key.Enter:
                        (add_handler ?? button_Add_Click)(sender, e);
                        //一つ下へ移動する。ただし、カーソル位置は正しく動かない。
                        int pos = box.SelectedIndex + 1;
                        box.SelectedIndex = Math.Max(0, Math.Min(pos, box.Items.Count - 1));
                        box.ScrollIntoViewIndex(box.SelectedIndex);
                        e.Handled = true;
                        break;
                }
            });
        }
        public void targetBoxAllowKeyAction(ListBox box, KeyEventHandler delete_handler = null)
        {
            if (box == null) return;
            //
            box.PreviewKeyDown += new KeyEventHandler((sender, e) =>
            {
                if (Keyboard.Modifiers != ModifierKeys.None) return;
                //
                switch (e.Key)
                {
                    case Key.Delete:
                        (delete_handler ?? button_Delete_Click)(sender, e);
                        e.Handled = true;
                        break;
                }
            });
        }

        /// <summary>エスケープキー及びアイテムの無い場所のクリックで選択を解除する</summary>
        public void AllowCancelAction()
        {
            sourceBoxAllowCancelAction(SourceBox);
            targetBoxAllowCancelAction(TargetBox);
        }
        public void sourceBoxAllowCancelAction(ListBox box) { allowCancelAction(box); }
        public void targetBoxAllowCancelAction(ListBox box) { allowCancelAction(box); }
        private void allowCancelAction(ListBox box)
        {
            if (box == null) return;
            //
            box.MouseLeftButtonUp += new MouseButtonEventHandler((sender, e) =>
            {
                if (box.GetPlacementItem() != null) return;
                //
                box.UnselectAll();
            });

            box.KeyDown += new KeyEventHandler((sender, e) =>
            {
                if (Keyboard.Modifiers != ModifierKeys.None) return;
                //
                switch (e.Key)
                {
                    case Key.Escape:
                        if (box.SelectedIndex >= 0)
                        {
                            box.UnselectAll();
                            e.Handled = true;
                        }
                        break;
                }
            });
        }

        /// <summary>ダブルクリックでの移動を行うよう設定する</summary>
        public void AllowDoubleClickMove()
        {
            sourceBoxAllowDoubleClick(SourceBox);
            targetBoxAllowDoubleClick(TargetBox);
        }
        public void sourceBoxAllowDoubleClick(ListBox box, MouseButtonEventHandler handler = null)
        {
            mouseDoubleClickSetter(box, handler ?? button_Add_Click);
        }
        public void targetBoxAllowDoubleClick(ListBox box, MouseButtonEventHandler handler = null)
        {
            mouseDoubleClickSetter(box, handler ?? button_Delete_Click);
        }
        private void mouseDoubleClickSetter(ListBox box, MouseButtonEventHandler handler)
        {
            if (box == null) return;
            //
            box.MouseDoubleClick += new MouseButtonEventHandler((sender, e) =>
            {
                var hitItem = box.GetPlacementItem();
                if (hitItem != null) handler(hitItem, e);
            });
        }

        /// <summary>全アイテム追加</summary>
        public void button_AddAll_Click(object sender, RoutedEventArgs e)
        {
            bxAddItemsAll(SourceBox, TargetBox, TargetItemsSource);
        }
        /// <summary>全アイテムリセット</summary>
        public void button_Reset_Click(object sender, RoutedEventArgs e)
        {
            bxResetItems(SourceBox, TargetBox, TargetItemsSource);
        }

        /// <summary>選択アイテム追加</summary>
        public void button_Add_Click(object sender, RoutedEventArgs e)
        {
            bxAddItems(SourceBox, TargetBox, false, TargetItemsSource);
        }
        /// <summary>選択アイテム挿入</summary>
        public void button_Insert_Click(object sender, RoutedEventArgs e)
        {
            bxAddItems(SourceBox, TargetBox, true, TargetItemsSource);
        }

        /// <summary>選択アイテム削除</summary>
        public void button_Delete_Click(object sender, RoutedEventArgs e)
        {
            bxDeleteItems(TargetBox, TargetItemsSource);
        }
        /// <summary>アイテム全削除</summary>
        public void button_DeleteAll_Click(object sender, RoutedEventArgs e)
        {
            bxDeleteItemsAll(TargetBox, TargetItemsSource);
        }

        /// <summary>1つ上に移動</summary>
        public void button_Up_Click(object sender, RoutedEventArgs e)
        {
            bxMoveItems(TargetBox, -1, TargetItemsSource);
        }
        /// <summary>1つ下に移動</summary>
        public void button_Down_Click(object sender, RoutedEventArgs e)
        {
            bxMoveItems(TargetBox, 1, TargetItemsSource);
        }

        /// <summary>一番上に移動</summary>
        public void button_Top_Click(object sender, RoutedEventArgs e)
        {
            bxMoveItemsTopBottom(TargetBox, -1, TargetItemsSource);
        }
        /// <summary>一番下に移動</summary>
        public void button_Bottom_Click(object sender, RoutedEventArgs e)
        {
            bxMoveItemsTopBottom(TargetBox, 1, TargetItemsSource);
        }
        /// <summary>一番上または下に移動</summary>
        public bool bxMoveItemsTopBottom(ListBox target, int direction, IList trgItemsSource = null)
        {
            if (target == null) return false;
            //
            return bxMoveItemsDrop(target, direction < 0 ? target.Items.Cast<object>().FirstOrDefault() : null, trgItemsSource);
        }

        /// <summary>全アイテム追加</summary>
        public bool bxAddItemsAll(ListBox src, ListBox target, IList trgItemsSource = null)
        {
            if (src == null || target == null) return false;

            if (src.SelectionMode != SelectionMode.Single) src.SelectAll();
            return bxAddItems(src.Items, target, false, trgItemsSource);
        }
        /// <summary>全アイテムリセット</summary>
        public bool bxResetItems(ListBox src, ListBox target, IList trgItemsSource = null)
        {
            if (src == null || target == null) return false;

            var trgItems = trgItemsSource ?? target.Items;
            SourceBox.UnselectAll();
            trgItems.Clear();
            trgItems.AddItemsAx(SourceBox.Items);
            TargetBoxItemsRefresh(target, trgItemsSource);

            return true;
        }

        /// <summary>選択アイテム追加・挿入</summary>
        public bool bxAddItems(ListBox src, ListBox target, bool IsInsert = false, IList trgItemsSource = null)
        {
            if (src == null) return false;
            //
            return bxAddItems(src.SelectedItemsList(true), target, IsInsert, trgItemsSource);
        }
        /// <summary>選択アイテム追加・挿入</summary>
        public bool bxAddItems(IEnumerable srcList, ListBox target, bool IsInsert = false, IList trgItemsSource = null)
        {
            try
            {
                if (srcList == null || srcList.Cast<object>().Count() == 0 || target == null) return false;

                var trgItems = trgItemsSource ?? target.Items;
                var addList = srcList.Cast<object>()
                    .Where(item => IsEnableDuplicate(item) == true || bxContains(target.Items, item) == false)
                    .Select(item => IsEnableDuplicate(item) == true ? CloneObj(item) : item).ToList();

                int scrollTo = target.SelectedIndex;
                if (IsInsert == true && target.SelectedIndex >= 0)
                {
                    trgItems.InsertItemsAx(target.SelectedIndex, addList);
                }
                else
                {
                    scrollTo = trgItems.AddItemsAx(addList);
                }

                target.UnselectAll();
                TargetBoxItemsRefresh(target, trgItemsSource);
                target.SelectedItemsAdd(addList);
                if (target.SelectedIndex >= 0) target.ScrollIntoViewIndex(scrollTo);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return true;
        }

        private bool IsEnableDuplicate(object item)
        {
            return DuplicationAllAllowed == true ||
                DuplicationSpecific != null && bxContains(DuplicationSpecific, item) == true;
        }
        private bool bxContains(IEnumerable lst, object obj)
        {
            return ItemComparer != null ? lst.Cast<object>().Contains(obj, ItemComparer) : lst.Cast<object>().Contains(obj);
        }

        private object CloneObj(object obj)
        {
            return ItemDuplicator != null ? ItemDuplicator(obj) : obj;
        }

        /// <summary>選択アイテム削除</summary>
        public bool bxDeleteItems(ListBox box, IList boxItemsSource = null)
        {
            try
            {
                if (box == null || box.SelectedIndex < 0) return false;

                var boxItems = boxItemsSource ?? box.Items;
                int newSelectedIndex = -1;
                while (box.SelectedIndex >= 0)
                {
                    newSelectedIndex = box.SelectedIndex;
                    boxItems.RemoveAt(newSelectedIndex);
                    TargetBoxItemsRefresh(box, boxItemsSource);
                }

                if (box.Items.Count != 0)
                {
                    box.SelectedIndex = Math.Min(newSelectedIndex, box.Items.Count - 1);
                    box.ScrollIntoViewIndex(box.SelectedIndex);
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return true;
        }
        /// <summary>アイテム全削除</summary>
        public bool bxDeleteItemsAll(ListBox box, IList boxItemsSource = null)
        {
            if (box == null) return false;
            //
            var boxItems = boxItemsSource ?? box.Items;
            boxItems.Clear();
            TargetBoxItemsRefresh(box, boxItemsSource);

            return true;
        }

        /// <summary>アイテムを上下に一つ移動</summary>
        public bool bxMoveItems(ListBox box, int direction, IList boxItemsSource = null)
        {
            try
            {
                if (box == null || box.SelectedIndex < 0) return false;

                var boxItems = boxItemsSource ?? box.Items;
                var selected = box.SelectedItemsList();//連続移動の視点固定のため順番保持
                int iCount = boxItems.Count;//固定
                var r = direction >= 0 ? (Func<int, int>)(i => iCount - 1 - i) : (i => i);

                for (int i = 0; i < boxItems.Count; i++)
                {
                    var item = boxItems[r(i)];
                    if (box.SelectedItems.Contains(item) == true)
                    {
                        boxItems.RemoveAt(r(i));
                        boxItems.Insert(r((i + iCount - 1) % iCount), item);
                        if (i == 0) break;
                    }
                }

                box.UnselectAll();
                TargetBoxItemsRefresh(box, boxItemsSource);
                box.SelectedItemsAdd(selected);
                box.ScrollIntoView(direction < 0 ? selected[0] : selected.Last());
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return true;
        }

        /// <summary>アイテムをボックス内にドロップ</summary>
        public void targetBox_PreviewDrop_fromSelf(object sender, DragEventArgs e)
        {
            var hitItem = GetDragHitItem(sender, e);
            bxMoveItemsDrop(TargetBox, hitItem == null ? null : hitItem.Content, TargetItemsSource);
        }
        /// <summary>アイテムをボックス内にドロップ</summary>
        public bool bxMoveItemsDrop(ListBox box, object dropTo, IList boxItemsSource = null)
        {
            try
            {
                if (box == null || box.SelectedIndex < 0) return false;

                var boxItems = boxItemsSource ?? box.Items;

                //選択の上と下でドロップ位置を調整する。
                int idx_dropTo = boxItems.IndexOf(dropTo);
                idx_dropTo = idx_dropTo < 0 ? boxItems.Count : idx_dropTo;
                idx_dropTo += (idx_dropTo >= box.SelectedIndex ? 1 : 0);

                var selected = box.SelectedItemsList(true);

                var insertItem = boxItems.Cast<object>().Skip(idx_dropTo).FirstOrDefault(item => !selected.Contains(item));
                boxItems.RemoveItemsAx(selected);//削除はこのタイミング
                int insertIdx = insertItem == null ? boxItems.Count : boxItems.IndexOf(insertItem);
                boxItems.InsertItemsAx(insertIdx, selected);
                box.SelectedItemsAdd(selected);

                TargetBoxItemsRefresh(box, boxItemsSource);
                box.ScrollIntoViewIndex(insertItem == null ? int.MaxValue : box.SelectedIndex);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return true;
        }

        private void TargetBoxItemsRefresh(ListBox box, object itmsSource = null)
        {
            if (back_font != null || itmsSource != null)
            {
                box.Items.Refresh();
                back_font = null;
            }
        }

        //ドラッグ関連
        fromBox dragStartBox;
        List<object> dragStartItems = null;
        bool dragStartCancel = false;
        DispatcherTimer notifyTimer = new DispatcherTimer();
        private void clearDragStartData()
        {
            notifyTimer.Stop();
            dragStartItems = null;
            dragStartCancel = false;
        }
        public void AllowDragDrop()
        {
            sourceBoxAllowDragDrop(SourceBox);
            targetBoxAllowDragDrop(TargetBox);
        }
        public void sourceBoxAllowDragDrop(ListBox srcBox, DragEventHandler delete_handler = null)
        {
            if (srcBox != null)
            {
                boxAllowDragDrop_common(srcBox);
                srcBox.PreviewDrop += getDragDropHandler(fromBox.TrgBox, delete_handler ?? ((sender, e) => bxDeleteItems(TargetBox)));
            }
        }
        public void targetBoxAllowDragDrop(ListBox trgBox, DragEventHandler add_handler = null)
        {
            if (trgBox != null)
            {
                boxAllowDragDrop_common(trgBox);
                trgBox.ItemContainerStyle.Setters.Add(new EventSetter(DragDrop.PreviewDragOverEvent, new DragEventHandler(targetBoxItem_PreviewDragOver)));
                trgBox.ItemContainerStyle.Setters.Add(new EventSetter(DragDrop.PreviewDragLeaveEvent, new DragEventHandler(targetBoxItem_PreviewDragLeave)));
                trgBox.PreviewDrop += getDragDropHandler(fromBox.SrcBox, add_handler ?? targetBox_PreviewDrop_fromSourceBox);
                trgBox.PreviewDrop += getDragDropHandler(fromBox.TrgBox, targetBox_PreviewDrop_fromSelf);
            }
        }
        private void boxAllowDragDrop_common(ListBox box)
        {
            box.AllowDrop = true;
            ViewUtil.ResetItemContainerStyle(box);

            //PreviewMouseLeftButtonDownより先に走るので、ダブルクリックが入る場合はキャンセルしておく。
            box.PreviewMouseDoubleClick += (sender, e) => dragStartCancel = true;
            box.PreviewMouseLeftButtonDown += box_PreviewMouseLeftButtonDown;
            box.ItemContainerStyle.Setters.Add(new EventSetter(Mouse.MouseLeaveEvent, new MouseEventHandler(boxItem_MouseLeave)));
            box.PreviewMouseLeftButtonUp += (sender, e) => clearDragStartData();
        }

        private void box_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            var box = sender as ListBox;
            var item = box.GetPlacementItem() as ListBoxItem;
            if (item == null || item.Content == null) return;

            //ダブルクリックが入る場合もキャンセルする
            if (dragStartCancel == true || IsMouseDragCondition() == false)
            {
                clearDragStartData();
                return;
            }

            dragStartBox = box == SourceBox ? fromBox.SrcBox : fromBox.TrgBox;

            //ドロップ位置の上下判定に使用するので、掴んだアイテムを先頭にする。
            dragStartItems = new List<object> { item.Content };
            if (box.SelectedItems.Contains(item.Content) == true)
            {
                dragStartItems.AddRange(box.SelectedItemsList());//重複するが特に問題ない。
            }

            // 一定時間押下で、ドラッグ中と判定をする。
            if (notifyTimer.Tag == null)
            {
                notifyTimer.Tag = "Initialized";
                notifyTimer.Interval = TimeSpan.FromSeconds(0.5);
                notifyTimer.Tick += (sender_t, e_t) =>
                {
                    notifyTimer.Stop();
                    boxItem_MouseLeave(null, null);
                };
            }
            notifyTimer.Start();
        }
        private void boxItem_MouseLeave(object sender, MouseEventArgs e)
        {
            if (dragStartItems == null) return;
            if (IsMouseDragCondition() == true)
            {
                notifyTimer.Stop();
                ListBox box = dragStartBox == fromBox.SrcBox ? SourceBox : TargetBox;
                var selList = dragStartItems;

                //MouseEnterを回避
                Dispatcher.CurrentDispatcher.BeginInvoke(new Action(() =>
                {
                    box.UnselectAll();
                    box.SelectedItemsAdd(selList);

                    //タイマーからの実行の際、先に画面を更新する
                    Dispatcher.CurrentDispatcher.BeginInvoke(new Action(() =>
                        DragDrop.DoDragDrop(box, box, box == SourceBox ? DragDropEffects.Copy : DragDropEffects.Move)),
                        DispatcherPriority.Render);

                }), DispatcherPriority.Render);
            }
            clearDragStartData();
        }
        private static bool IsMouseDragCondition()
        {
            return Mouse.LeftButton == MouseButtonState.Pressed && Keyboard.Modifiers == ModifierKeys.None && Keyboard.Modifiers == ModifierKeys.None;
        }
        private ListBox dStartBox(DragEventArgs e) 
        {
            //GetListBoxType()は正しいタイプを与えないとListBox(ListView)を返してくれない。
            return e.Data.GetData((SourceBox ?? TargetBox).GetType()) as ListBox;
        }
        private void targetBoxItem_PreviewDragOver(object sender, DragEventArgs e)
        {
            try
            {
                var item = sender as ListBoxItem;
                if (item == null) return;

                if (dStartBox(e) == SourceBox)
                {
                    TargetBox.UnselectAll();
                    item.IsSelected = true;
                }
                else if (dStartBox(e) == TargetBox)
                {
                    if (back_font == null) back_font = item.FontWeight;
                    item.FontWeight = FontWeights.Bold;
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        //移動ターゲットになるアイテムのフォント変更のバックアップ
        FontWeight? back_font = null;

        private void targetBoxItem_PreviewDragLeave(object sender, DragEventArgs e)
        {
            try
            {
                var item = sender as ListBoxItem;
                if (item == null) return;

                if (dStartBox(e) == TargetBox)
                {
                    if (back_font != null) item.FontWeight = (FontWeight)back_font;
                    back_font = null;
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public void targetBox_PreviewDrop_fromSourceBox(object sender, DragEventArgs e)
        {
            bxAddItems(SourceBox, TargetBox, GetDragHitItem(sender, e) != null, TargetItemsSource);
        }
        public static ListBoxItem GetDragHitItem(object sender, DragEventArgs e)
        {
            return (sender as ListBox).GetPlacementItem(e.GetPosition(sender as ListBox)) as ListBoxItem;
        }

        enum fromBox { SrcBox, TrgBox };
        private DragEventHandler getDragDropHandler(fromBox box, DragEventHandler proc)
        {
            return new DragEventHandler((sender, e) =>
            {
                try
                {
                    if (dStartBox(e) == (box == fromBox.SrcBox ? SourceBox : TargetBox)) proc(sender, e);
                }
                catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            });
        }
    }

    static class BoxEx
    {
        //c#的でないので元々定義されてないが、ここではよく使ってるので‥
        /// <summary>指定したコレクションの要素を追加する</summary>
        public static int AddItems(this ItemCollection itemColloction, IEnumerable collection)
        {
            return itemColloction.AddItemsAx(collection);
        }
        /// <summary>指定したコレクションの要素を挿入する</summary>
        public static void InsertItems(this ItemCollection itemColloction, int insertIndex, IEnumerable collection)
        {
            itemColloction.InsertItemsAx(insertIndex, collection);
        }
        /// <summary>指定したコレクションの要素を削除する</summary>
        public static void RemoveItems(this ItemCollection itemColloction, IEnumerable collection)
        {
            itemColloction.RemoveItemsAx(collection);
        }
        /// <summary>指定したコレクションの要素を選択項目に追加する。SelectionMode.Singleでもcollectionの先頭の要素を選択要素に設定する。</summary>
        public static int SelectedItemsAdd(this ListBox box, IEnumerable collection)
        {
            if (collection == null) return -1;

            var addList = collection.ToListAx();
            if (addList.Count == 0) return -1;

            //下の暫定処置で順番壊れるが、先頭だけは保持できる。(SelectAllは設定されているSelectedIndexを動かさないため)
            box.SelectedItem = addList[0];
            if (addList.Count == 1) return 0;//SingleMode対応

            //暫定処置。早くする方法は？
            if (box.Items.Count * 0.7 <= addList.Count)
            {
                addList = addList.FindAll(item => box.Items.Contains(item)).Distinct().ToList();
                if (box.Items.Count * 0.7 <= addList.Count)
                {
                    box.SelectAll();
                    box.SelectedItems.RemoveItemsAx(box.Items.Cast<object>().Except(addList));
                    return box.Items.Count - 1;
                }
            }

            return box.SelectedItems.AddItemsAx(addList);
        }
        /// <summary>選択アイテムをリスト化する。SelectionMode.Singleでも選択アイテムがあれば単一要素のリストを返す。</summary>
        public static List<object> SelectedItemsList(this ListBox box, bool isItemsOrdered = false)
        {
            if (isItemsOrdered == true && box.SelectedItems.Count > 1)
            {
                return box.Items.ToListAx().FindAll(item => box.SelectedItems.Contains(item));
            }
            else
            {
                return box.SelectedItems.ToListAx();
            }
        }
    }

    namespace BoxExchangeEditAux
    {
        static class BoxExAux
        {
            //コンパイル時の型チェックが効かないので、一応このファイル内だけ
            public static int AddItemsAx(this IList list, IEnumerable collection)
            {
                if (list == null || collection == null) return -1;
                int addres = -1;//追加されない場合
                foreach (var item in collection) addres = list.Add(item);
                return addres;
            }
            public static void InsertItemsAx(this IList list, int insertIndex, IEnumerable collection)
            {
                if (list == null || collection == null) return;
                int i = 0;
                foreach (var item in collection) list.Insert(insertIndex + (i++), item);
            }
            public static void RemoveItemsAx(this IList list, IEnumerable collection)
            {
                if (list == null || collection == null) return;
                foreach (var item in collection) list.Remove(item);
            }
            public static List<object> ToListAx(this IEnumerable collection)
            {
                if (collection == null) return null;
                return collection.Cast<object>().ToList();
            }
        }
    }
}
