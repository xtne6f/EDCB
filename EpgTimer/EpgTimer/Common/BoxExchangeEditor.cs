using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Collections;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    class BoxExchangeEditor
    {
        //複数選択可能なListBox同士でアイテムを交換する。
        //・複数選択設定が必要
        //・編集側(TargetBox)はアイテムソースを使えない

        public ListBox SourceBox { set; get; }
        public ListBox TargetBox { set; get; }

        public bool DuplicationAllowed { set; get; }//項目の重複を全て許可
        public IList DuplicationSpecific { set; get; }//特定の項目のみ重複を許可

        /// <summary>ソース側のEnter、ターゲット側のDelete、Escによる選択解除を有効にする</summary>
        public void KeyActionAllow()
        {
            if (SourceBox != null) sourceBoxKeyEnable(SourceBox, button_add_Click);
            if (TargetBox != null) targetBoxKeyEnable(TargetBox, button_del_Click);
        }
        public void sourceBoxKeyEnable(ListBox box, KeyEventHandler add_handler)
        {
            if (box == null) return;
            //
            box.PreviewKeyDown += getBoxKeyEnableHandler(box, add_handler, true);
        }
        public void targetBoxKeyEnable(ListBox box, KeyEventHandler delete_handler)
        {
            if (box == null) return;
            //
            box.PreviewKeyDown += getBoxKeyEnableHandler(box, delete_handler, false);
        }
        private KeyEventHandler getBoxKeyEnableHandler(ListBox box, KeyEventHandler handler, bool src)
        {
            return new KeyEventHandler((sender, e) =>
            {
                if (Keyboard.Modifiers == ModifierKeys.None)
                {
                    switch (e.Key)
                    {
                        case Key.Escape:
                            if (box.SelectedItem != null)
                            {
                                box.UnselectAll();
                                e.Handled = true;
                            }
                            break;
                        case Key.Enter:
                            if (src == true)
                            {
                                handler(sender, e);
                                //一つ下へ移動する。ただし、カーソル位置は正しく動かない。
                                int pos = box.SelectedIndex + 1;
                                box.SelectedIndex = Math.Max(0, Math.Min(pos, box.Items.Count - 1));
                                box.ScrollIntoViewFix(box.SelectedIndex);
                                e.Handled = true;
                            }
                            break;
                        case Key.Delete:
                            if (src == false)
                            {
                                handler(sender, e);
                                e.Handled = true;
                            }
                            break;
                    }
                }
            });
        }

        /// <summary>ダブルクリックでの移動を行うよう設定する</summary>
        public void DoubleClickMoveAllow()
        {
            if (SourceBox != null) doubleClickSetter(SourceBox, sourceBox_MouseDoubleClick);
            if (TargetBox != null) doubleClickSetter(TargetBox, targetBox_MouseDoubleClick);
        }
        public void doubleClickSetter(ListBox box, MouseButtonEventHandler handler)
        {
            if (box == null) return;
            //
            if (box.ItemContainerStyle == null)
            {
                box.ItemContainerStyle = (Style)new Style(typeof(ListBoxItem));
            }
            box.ItemContainerStyle.Setters.Add(new EventSetter(Button.MouseDoubleClickEvent, new MouseButtonEventHandler(handler)));
        }
        public void sourceBox_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            addItems(SourceBox, TargetBox);
        }
        public void targetBox_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            deleteItems(TargetBox);
        }

        /// <summary>全アイテム追加</summary>
        public void button_addAll_Click(object sender, RoutedEventArgs e)
        {
            addAllItems(SourceBox, TargetBox);
        }

        /// <summary>全アイテムリセット</summary>
        public void button_reset_Click(object sender, RoutedEventArgs e)
        {
            addAllItems(SourceBox, TargetBox, true);
        }

        /// <summary>選択アイテム追加</summary>
        public void button_add_Click(object sender, RoutedEventArgs e)
        {
            addItems(SourceBox, TargetBox);
        }

        /// <summary>選択アイテム挿入</summary>
        public void button_insert_Click(object sender, RoutedEventArgs e)
        {
            addItems(SourceBox, TargetBox, true);
        }

        /// <summary>選択アイテム削除</summary>
        public void button_del_Click(object sender, RoutedEventArgs e)
        {
            deleteItems(TargetBox);
        }

        /// <summary>アイテム全削除</summary>
        public void button_delAll_Click(object sender, RoutedEventArgs e)
        {
            if (TargetBox != null) TargetBox.Items.Clear();
        }

        /// <summary>1つ上に移動</summary>
        public void button_up_Click(object sender, RoutedEventArgs e)
        {
            move_item(TargetBox, -1);
        }

        /// <summary>1つ下に移動</summary>
        public void button_down_Click(object sender, RoutedEventArgs e)
        {
            move_item(TargetBox, 1);
        }

        /// <summary>一番上に移動</summary>
        public void button_top_Click(object sender, RoutedEventArgs e)
        {
            move_item(TargetBox, -1 * TargetBox.SelectedIndex);
        }

        /// <summary>一番下に移動</summary>
        public void button_bottom_Click(object sender, RoutedEventArgs e)
        {
            move_item(TargetBox, TargetBox.Items.Count - 1 - TargetBox.SelectedIndex);
        }

        public void addAllItems(ListBox src, ListBox target, bool IsReset = false)
        {
            try
            {
                if (src == null || target == null) return;

                if (IsReset == true)
                {
                    target.Items.Clear();
                }

                src.UnselectAll();
                src.SelectAll();
                addItems(src, target);

                if (IsReset == true)
                {
                    src.UnselectAll();
                    target.UnselectAll();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void addItems(ListBox src, ListBox target, bool IsInsert = false)
        {
            try
            {
                if (src == null || target == null) return;

                int LastIndex = -1;
                foreach (object info in src.SelectedItems)
                {
                    if (IsEnableAdd(target, info) == true)
                    {
                        if (IsInsert == true && target.SelectedItem != null)
                        {
                            target.Items.Insert(target.SelectedIndex + 1, info);
                            target.SelectedIndex += 1;
                            LastIndex = target.SelectedIndex;
                        }
                        else
                        {
                            target.Items.Add(info);
                            LastIndex = target.Items.Count - 1;
                        }
                    }
                }
                if (target.Items.Count != 0 && LastIndex >=0)
                {
                    target.SelectedIndex = LastIndex;
                    target.ScrollIntoViewFix(LastIndex);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private bool IsEnableAdd(ListBox target, object item)
        {
            if (DuplicationAllowed == true) return true;
            if (DuplicationSpecific != null && DuplicationSpecific.Contains(item) == true) return true;
            if (target.Items.Contains(item) == false) return true;
            return false;
        }

        public void deleteItems(ListBox target)
        {
            try
            {
                if (target == null) return;

                int newSelectedIndex = -1;
                while (target.SelectedIndex >= 0)
                {
                    newSelectedIndex = target.SelectedIndex;
                    target.Items.RemoveAt(newSelectedIndex);
                }

                if (target.Items.Count != 0)
                {
                    target.Items.Refresh();
                    newSelectedIndex = (newSelectedIndex == target.Items.Count ? newSelectedIndex - 1 : newSelectedIndex);
                    target.SelectedIndex = newSelectedIndex;
                    target.ScrollIntoViewFix(newSelectedIndex);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>アイテムを移動</summary>
        public void move_item(ListBox target, int direction)
        {
            try
            {
                if (target == null || target.SelectedItem == null) return;

                object temp = target.SelectedItem;
                int newIndex = ((target.SelectedIndex + direction) % target.Items.Count + target.Items.Count) % target.Items.Count;
                target.Items.RemoveAt(target.SelectedIndex);
                target.Items.Insert(newIndex, temp);
                target.SelectedIndex = newIndex;
                target.ScrollIntoViewFix(newIndex);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

    }
}
