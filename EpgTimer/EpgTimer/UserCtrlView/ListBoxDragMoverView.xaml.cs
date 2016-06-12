using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;
using System.IO;

namespace EpgTimer.UserCtrlView
{
    using EpgTimer.BoxExchangeEdit;

    /// <summary>
    /// ListBoxDragMover.xaml の相互作用ロジック
    /// </summary>
    // 他でも使うなら、UP,Downのハンドラとドラッグ系の機能だけ別クラスに分離する
    public partial class ListBoxDragMoverView : UserControl
    {
        private static Cursor OnDragCursor;
        private Control Owner;
        private ListBox listBox;
        private IList dataList;
        private BoxExchangeEditor bx;

        public ListBoxDragMoverView()
        {
            InitializeComponent();

            try
            {
                if (OnDragCursor == null)
                {
                    Stream CusorStream = Application.GetResourceStream(new Uri("pack://application:,,,/Resources/drag.cur")).Stream;
                    OnDragCursor = new Cursor(CusorStream);
                    //OnDragCursor = ((UserControl)this.Resources["DragCursor"]).Cursor;
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public abstract class LVDMHelper
        {
            public abstract uint GetID(object data);
            public virtual bool SaveChange(Dictionary<uint, uint> changeIDTable) { return true; }
            public virtual bool RestoreOrder() { return true; }
            public virtual void StatusChanged() { }
            public virtual void ItemMoved() { }//アイテム動かなくてもステータス変更の場合がある
        }
        private LVDMHelper hlp;
        public void SetData(Control ow, ListBox listbox, IList list, LVDMHelper helper, MenuBinds mbinds = null)
        {
            try
            {
                Owner = ow;
                listBox = listbox;
                dataList = list ?? listbox.Items;
                hlp = helper;
                MenuBinds mBinds = mbinds ?? new MenuBinds();

                //マウスイベント関係
                this.listBox.PreviewMouseLeftButtonUp += new MouseButtonEventHandler(listBox_PreviewMouseLeftButtonUp);
                this.listBox.PreviewMouseLeftButtonDown += new MouseButtonEventHandler(listBox_PreviewMouseLeftButtonDown);
                ViewUtil.ResetItemContainerStyle(listbox);
                listbox.ItemContainerStyle.Setters.Add(new EventSetter(Mouse.MouseEnterEvent, new MouseEventHandler(listBoxItem_MouseEnter)));

                //移動などのアクションはBoxExchangeEditorのものをそのまま使用する。
                bx = new BoxExchangeEditor { TargetBox = listBox, TargetItemsSource = dataList };

                this.Owner.CommandBindings.Add(new CommandBinding(EpgCmds.TopItem, (sender, e) => ItemsAction(() => bx.button_Top_Click(null, null))));
                this.Owner.CommandBindings.Add(new CommandBinding(EpgCmds.UpItem, (sender, e) => ItemsAction(() => bx.button_Up_Click(null, null))));
                this.Owner.CommandBindings.Add(new CommandBinding(EpgCmds.DownItem, (sender, e) => ItemsAction(() => bx.button_Down_Click(null, null))));
                this.Owner.CommandBindings.Add(new CommandBinding(EpgCmds.BottomItem, (sender, e) => ItemsAction(() => bx.button_Bottom_Click(null, null))));
                this.Owner.CommandBindings.Add(new CommandBinding(EpgCmds.SaveOrder, SaveOrder_handler, (sender, e) => e.CanExecute = NotSaved == true));
                this.Owner.CommandBindings.Add(new CommandBinding(EpgCmds.RestoreOrder, (sender, e) => hlp.RestoreOrder(), (sender, e) => e.CanExecute = NotSaved == true));
                this.Owner.CommandBindings.Add(new CommandBinding(EpgCmds.DragCancel, (sender, e) => DragRelease()));

                mBinds.SetCommandToButton(button_top, EpgCmds.TopItem);
                mBinds.SetCommandToButton(button_up, EpgCmds.UpItem);
                mBinds.SetCommandToButton(button_down, EpgCmds.DownItem);
                mBinds.SetCommandToButton(button_bottom, EpgCmds.BottomItem);
                mBinds.SetCommandToButton(button_saveItemOrder, EpgCmds.SaveOrder);
                mBinds.SetCommandToButton(button_reloadItem, EpgCmds.RestoreOrder);
                mBinds.AddInputCommand(EpgCmds.DragCancel);//アイテムのドラッグキャンセル
                if (mbinds == null)
                {
                    mBinds.ResetInputBindings(this.Owner, this.listBox);
                }

                //コマンドだと、DragCancelしない場合に、CanExecuteを割り当てても、Handled=falseにしても、
                //Keygestureが先へ伝搬してくれないので、対処療法的だけど先に処理してしまうことにする。コマンドの意味無い‥。
                listBox.PreviewKeyDown += new KeyEventHandler((sender, e) =>
                {
                    if (Keyboard.Modifiers == ModifierKeys.None)
                    {
                        switch (e.Key)
                        {
                            case Key.Escape:
                                if (_onDrag == true)
                                {
                                    EpgCmds.DragCancel.Execute(null, null);
                                    //this.listBox.Items.Refresh();//壊したバインディングを修復する。モタツキ感があるので放置する。
                                    e.Handled = true;
                                }
                                break;
                        }
                    }
                });
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public void ItemsAction(Action func)
        {
            var oldList = this.dataList.Cast<object>().ToList();//ここはリストの複製が必要

            func();

            if (oldList.SequenceEqual(this.dataList.Cast<object>()) == false)
            {
                this.NotSaved = true;
                hlp.ItemMoved();
            }
        }

        private bool _notSaved = false;
        public bool NotSaved
        {
            get { return this._notSaved; }
            set
            {
                this._notSaved = value;
                StatusChanged();
            }
        }
        private bool _onDrag = false;
        public bool OnDrag
        {
            get { return this._onDrag; }
            set
            {
                this._onDrag = value;
                StatusChanged();
            }
        }
        private void StatusChanged()
        {
            this.textBox_Status.Text = (_onDrag == true ? "ドラッグ中..." : _notSaved == true ? "*未保存" : "");
            hlp.StatusChanged();
        }

        public void SaveOrder_handler(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                if (CmdExeUtil.IsDisplayKgMessage(e) == true)
                {
                    if (MessageBox.Show("並びの変更を保存します。\r\nよろしいですか？", "保存の確認",
                        MessageBoxButton.OKCancel) != MessageBoxResult.OK) return;
                }

                //並び替えテーブル
                var changeIDTable = GetChangeIDTable();

                if (hlp.SaveChange(changeIDTable) == true)
                {
                    this.NotSaved = false;

                    //結果を保存する
                    if (e.Parameter is EpgCmdParam)
                    {
                        (e.Parameter as EpgCmdParam).Data = changeIDTable;
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        public Dictionary<uint, uint> GetChangeIDTable()
        {
            List<uint> dataIdList = this.dataList.OfType<object>().Select(item => hlp.GetID(item)).ToList();
            dataIdList.Sort();//存在する(再利用可能な)IDリストを若い順(元の順)に並べたもの。

            //並び替えテーブル
            var changeIDTable = new Dictionary<uint, uint>();
            for (int i = 0; i < this.dataList.Count; i++)
            {
                changeIDTable.Add(hlp.GetID(this.dataList[i]), dataIdList[i]);
            }
            return changeIDTable;
        }

        //移動関連
        object cursorObj = null;
        List<object> dragItems = null;
        DispatcherTimer notifyTimer = new DispatcherTimer();

        public void listBox_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            try
            {
                object item = (sender as ListBox).GetPlacementItem();
                object dragItem = GetDragItemData(item);
                if (dragItem == null) return;

                if (IsMouseDragCondition() == false) return;

                this.cursorObj = item;

                //ドロップ位置の上下判定に使用するので、掴んだアイテムを先頭にする。
                this.dragItems = new List<object> { dragItem }; ;
                if (this.listBox.SelectedItems.Contains(dragItem) == true)
                {
                    this.dragItems.AddRange(this.listBox.SelectedItemsList());
                }

                // 一定時間押下で、ドラッグ中と判定をする。
                if (notifyTimer.Tag == null)
                {
                    notifyTimer.Tag = "Initialized";
                    notifyTimer.Interval = TimeSpan.FromSeconds(0.5);
                    notifyTimer.Tick += (sender_t, e_t) =>
                    {
                        notifyTimer.Stop();
                        listBoxItem_MouseEnter(this.cursorObj, null);
                    };
                }
                notifyTimer.Start();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public void listBox_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            try
            {
                if (this._onDrag == true && this.dragItems != null && IsMouseDragCondition(false) == true)
                {
                    object dropTo = GetDragItemData(this.cursorObj);
                    if (dropTo != null)
                    {
                        ItemsAction(() => bx.bxMoveItemsDrop(bx.TargetBox, dropTo, bx.TargetItemsSource));
                        e.Handled = true;
                    }
                }
                DragRelease();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public void listBoxItem_MouseEnter(object sender, MouseEventArgs e)
        {
            try
            {
                if (this.dragItems == null) return;
                if (IsMouseDragCondition() == false)
                {
                    DragRelease();
                    return;
                }
                if (this._onDrag == false)
                {
                    notifyTimer.Stop();
                    this.Owner.Cursor = OnDragCursor; //Cursors.ScrollS;Cursors.SizeNS;
                    this.OnDrag = true;
                }
                this.cursorObj = sender;
                DragItemsSelect();
                DrawDropLine();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        private object GetDragItemData(object obj)
        {
            //ListBoxItemじゃないものが来ることもあるのでちゃんとチェックする。
            var lvItem = obj as ListBoxItem;
            return lvItem == null ? null : lvItem.Content;
        }
        private bool IsMouseDragCondition(bool isPressedCheck = true)
        {
            return (isPressedCheck == false || Mouse.LeftButton == MouseButtonState.Pressed)
                && Keyboard.Modifiers == ModifierKeys.None && Keyboard.Modifiers == ModifierKeys.None;
        }
        private void DragItemsSelect()
        {
            this.listBox.UnselectAll();
            this.listBox.SelectedItemsAdd(dragItems);
        }
        public void DragRelease()
        {
            try
            {
                //タイマーが走ってる場合がある。
                notifyTimer.Stop();

                if (this.dragItems != null)
                {
                    this.Owner.Cursor = null;
                    this.OnDrag = false;
                    this.cursorObj = null;
                    this.dragItems = null;
                    EraseDropLine();
                    ClearDropLineData();
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        //ドロップライン関係
        int back_ListItem = -1;
        List<Brush> back_Brush = null;
        List<Thickness> back_Thickness = null;

        private void DrawDropLine()
        {
            EraseDropLine();

            var item = this.cursorObj as ListBoxItem;
            if (item == null) return;

            if (back_Brush == null)
            {
                //元の色を押さえておくにはこのタイミングしかない。
                //一時的にバインディングが壊れるが、余り影響無いので気にしない。
                back_Brush = new List<Brush>();
                back_Thickness = new List<Thickness>();
                for (int i = 0; i < this.listBox.Items.Count; i++)
                {
                    //とりあえずインデックスで管理する。
                    var boxItem = this.listBox.ItemContainerGenerator.ContainerFromIndex(i) as ListBoxItem;
                    back_Brush.Add(boxItem != null ? boxItem.BorderBrush : null);
                    back_Thickness.Add(boxItem != null ? boxItem.BorderThickness : new Thickness());
                }
            }

            back_ListItem = this.dataList.IndexOf(item.Content);

            item.BorderBrush = Brushes.OrangeRed;
            Thickness border = item.BorderThickness;
            border.Top = back_ListItem >= this.listBox.SelectedIndex ? 0 : 3;
            border.Bottom = 3 - border.Top;
            item.BorderThickness = border;
        }
        private void EraseDropLine()
        {
            if (back_Brush != null && 0 <= back_ListItem && back_ListItem < back_Brush.Count &&
                back_ListItem < this.listBox.Items.Count)
            {
                var boxItem = this.listBox.ItemContainerGenerator.ContainerFromIndex(back_ListItem) as ListBoxItem;
                if (boxItem == null) return;
                boxItem.BorderBrush = back_Brush[back_ListItem];
                boxItem.BorderThickness = back_Thickness[back_ListItem];
            }
        }
        private void ClearDropLineData()
        {
            back_ListItem = -1;
            back_Brush = null;
            back_Thickness = null;
        }
    }
}
