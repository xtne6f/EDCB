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
            public abstract void SetID(object data, uint ID);
            public virtual bool SaveChange() { return true; }
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
                dataList = list;
                hlp = helper;
                MenuBinds mBinds = (mbinds != null ? mbinds : new MenuBinds());

                //this.listBox.PreviewMouseLeftButtonUp += new MouseButtonEventHandler(listBox_PreviewMouseLeftButtonUp);

                this.Owner.CommandBindings.Add(new CommandBinding(EpgCmds.UpItem, (sender, e) => MoveItem(-1)));
                this.Owner.CommandBindings.Add(new CommandBinding(EpgCmds.DownItem, (sender, e) => MoveItem(1)));
                this.Owner.CommandBindings.Add(new CommandBinding(EpgCmds.SaveOrder, SaveOrder_handler, (sender, e) => e.CanExecute = NotSaved == true));
                this.Owner.CommandBindings.Add(new CommandBinding(EpgCmds.RestoreOrder, (sender, e) => hlp.RestoreOrder(), (sender, e) => e.CanExecute = NotSaved == true));
                this.Owner.CommandBindings.Add(new CommandBinding(EpgCmds.DragCancel, (sender, e) => DragRelease()));

                mBinds.SetCommandToButton(button_up, EpgCmds.UpItem);
                mBinds.SetCommandToButton(button_down, EpgCmds.DownItem);
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
                                    e.Handled = true;
                                }
                                break;
                        }
                    }
                });

            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
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

        public void MoveItem(int direction)
        {
            try
            {
                if (this.listBox.SelectedItem == null) { return; }

                //選択状態+順序のペアを作る
                var list = this.dataList.OfType<object>().Select((item, index) => 
                    new KeyValuePair<int, bool>(index, this.listBox.SelectedItems.Contains(item))).ToList();

                //逆方向の時はリストひっくり返す
                if (direction >= 0) list.Reverse();

                //移動対象でないアイテムが上下ループを超えないよう細工
                //超えているように見えるときでも良く見ると超えていない
                list.Insert(0, new KeyValuePair<int, bool>(-1, false));
                int end = list[1].Value == true ? 2 : list.Count;
                for (int i = 1; i < end; i++)
                {
                    //選択状態のものだけ移動
                    if (list[i].Value == true)
                    {
                        var tmp = list[i - 1];
                        list.RemoveAt(i - 1);
                        list.Insert(i, tmp);
                    }
                }
                //ループしたものを下へ持って行く。ループしてなければダミーがコピーされるだけ
                list.Add(list[0]);

                //リンク張り替えながらダミー以外(Key!=-1)をコピー
                var chglist = list.Skip(1).Where(item => item.Key != -1).Select(item => this.dataList[item.Key]).ToList();
                if (direction >= 0) chglist.Reverse();

                this.dataList.Clear();
                chglist.ForEach(item => this.dataList.Add(item));

                this.listBox.Items.Refresh();
                this.NotSaved = true;
                hlp.ItemMoved();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
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

                List<uint> dataIdList = this.dataList.OfType<object>().Select(item => hlp.GetID(item)).ToList();
                dataIdList.Sort();//存在する(再利用可能な)IDリストを若い順(元の順)に並べたもの。
                //
                for (int i = 0; i < this.dataList.Count; i++)
                {
                    hlp.SetID(this.dataList[i], dataIdList[i]);
                }

                if (hlp.SaveChange() == true)
                {
                    this.NotSaved = false;
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        //移動関連
        object cursorObj = null;
        object dragItem = null;
        List<object> dragItems = new List<object>();
        DispatcherTimer notifyTimer = new DispatcherTimer();

        public void listBoxItem_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            try
            {
                this.dragItem = GetDragItemData(sender);
                if (this.dragItem == null) return;

                this.cursorObj = sender;

                if (this.listBox.SelectedItems.Contains(this.dragItem))
                {
                    this.dragItems = this.listBox.SelectedItems.OfType<object>().ToList();
                    this.dragItems.Sort((i1, i2) => this.dataList.IndexOf(i1) - this.dataList.IndexOf(i2));
                }
                else
                {
                    this.dragItems = new List<object> { this.dragItem };
                }

                // 一定時間押下で、ドラッグ中と判定をする。
                notifyTimer.Interval = TimeSpan.FromSeconds(0.5);
                notifyTimer.Tick += (sender_t, e_t) =>
                {
                    if (this._onDrag == false)
                    {
                        if (Mouse.LeftButton == MouseButtonState.Pressed)
                        {
                            DragStart();
                        }
                        else
                        {
                            //通常このパスは通らないはずだが、一応クリアしておく。
                            DragRelease();
                        }
                    }
                    notifyTimer.Stop();
                };
                notifyTimer.Start();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public void listBox_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            try
            {
                object dropTo = DragItemTest(this.cursorObj);
                if (dropTo != null)
                {
                    this.MoveItem(dropTo);
                }
                e.Handled = _onDrag == true;
                DragRelease();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public void listBoxItem_MouseEnter(object sender, MouseEventArgs e)
        {
            try
            {
                if (Mouse.LeftButton == MouseButtonState.Released || DragItemTest(sender) == null)
                {
                    DragRelease();
                    return;
                }
                this.cursorObj = sender;
                DragStart();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        private object DragItemTest(object obj)
        {
            object item = GetDragItemData(obj);
            if (this.dragItem == null || (this.dragItem == item && this._onDrag == false)
                || Keyboard.Modifiers == ModifierKeys.Shift || Keyboard.Modifiers == ModifierKeys.Control)
            { return null; }

            return item;
        }
        private object GetDragItemData(object obj)
        {
            //ListBoxItemじゃないものが来ることもあるのでちゃんとチェックする。
            var lvItem = obj as ListBoxItem;
            return lvItem == null ? null : lvItem.Content;
        }
        private void DragItemsSelect()
        {
            if (dragItem != null)
            {
                this.listBox.SelectedItem = dragItem;
                dragItems.ForEach(item => this.listBox.SelectedItems.Add(item));
            }
        }
        private void DragStart()
        {
            this.Owner.Cursor = OnDragCursor; //Cursors.ScrollS;Cursors.SizeNS;
            this.OnDrag = true;
            DragItemsSelect();
            DrawDropLine();
        }
        public void DragRelease()
        {
            try
            {
                //タイマーが走ってる場合がある。
                if (notifyTimer.IsEnabled == true ||  this._onDrag == true)
                {
                    notifyTimer.Stop();
                    this.Owner.Cursor = null;
                    this.OnDrag = false;
                    this.cursorObj = null;
                    this.dragItem = null;
                    this.dragItems = new List<object>();
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
            border.Top = back_ListItem >= this.dataList.IndexOf(this.dragItem) ? 0 : 3;
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
        public void MoveItem(object dropTo)
        {
            try
            {
                var oldList = this.dataList.OfType<object>().ToList();
                int idx_dropItems = oldList.IndexOf(this.dragItem);
                int idx_dropTo = oldList.IndexOf(dropTo);

                //一番上と一番下を選択できるように調整
                idx_dropTo += (idx_dropTo >= idx_dropItems ? 1 : 0);

                //挿入位置で分割→バラのも含め選択アイテムを除去→分割前部+選択アイテム+分割後部で連結
                var work1 = oldList.Take(idx_dropTo).Where(item => !dragItems.Contains(item)).ToList();
                var work2 = oldList.Skip(idx_dropTo).Where(item => !dragItems.Contains(item)).ToList();

                var newList = work1.Concat(dragItems).Concat(work2).ToList();

                this.dataList.Clear();
                newList.ForEach(item => this.dataList.Add(item));
                DragItemsSelect();

                this.listBox.Items.Refresh();

                if (CheckOrderChanged(oldList, newList) == true)
                {
                    this.NotSaved = true;
                    hlp.ItemMoved();
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        private bool CheckOrderChanged(IList oldList, IList newList)
        {
            for (int i = 0; i < newList.Count; i++)
            {
                if (oldList[i] != newList[i]) return true;
            }
            return false;
        }
    }
}
