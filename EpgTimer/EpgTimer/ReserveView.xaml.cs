using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.ComponentModel;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    /// <summary>
    /// ReserveView.xaml の相互作用ロジック
    /// </summary>
    public partial class ReserveView : UserControl
    {
        private bool RedrawReserve = true;
        private List<ReserveItem> reserveList = new List<ReserveItem>();
        private Dictionary<String, GridViewColumn> columnList = new Dictionary<String, GridViewColumn>();

        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private MenuUtil mutil = CommonManager.Instance.MUtil;

        MainWindow _mainWindow;

        public ReserveView()
        {
            InitializeComponent();

            try
            {
                if (Settings.Instance.NoStyle == 1)
                {
                    button_del.Style = null;
                    button_change.Style = null;
                    button_on_off.Style = null;
                    button_add_manual.Style = null;
                    button_timeShiftPlay.Style = null;
                }

                foreach (GridViewColumn info in gridView_reserve.Columns)
                {
                    GridViewColumnHeader header = info.Header as GridViewColumnHeader;
                    columnList.Add((string)header.Tag, info);
                }
                gridView_reserve.Columns.Clear();

                foreach (ListColumnInfo info in Settings.Instance.ReserveListColumn)
                {
                    columnList[info.Tag].Width = info.Width;
                    gridView_reserve.Columns.Add(columnList[info.Tag]);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }


        public void SaveSize()
        {
            try
            {
                Settings.Instance.ReserveListColumn.Clear();
                foreach (GridViewColumn info in gridView_reserve.Columns)
                {
                    GridViewColumnHeader header = info.Header as GridViewColumnHeader;

                    Settings.Instance.ReserveListColumn.Add(new ListColumnInfo((String)header.Tag, info.Width));
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (RedrawReserve == true && this.IsVisible == true)
            {
                if (ReDrawReserveData() == true)
                {
                    RedrawReserve = false;
                }
            }
            this._mainWindow = (MainWindow)Window.GetWindow(this);
        }

        /// <summary>
        /// 予約情報の更新通知
        /// </summary>
        public void UpdateReserveData()
        {
            RedrawReserve = true;
            if (this.IsVisible == true)
            {
                if (ReDrawReserveData() == true)
                {
                    RedrawReserve = false;
                }
            }
        }

        private bool ReDrawReserveData()
        {
            try
            {
                if (CommonManager.Instance.NWMode == true)
                {
                    if (CommonManager.Instance.NW.IsConnected == false)
                    {
                        return false;
                    }
                }
                ErrCode err = CommonManager.Instance.DB.ReloadReserveInfo();
                if (CommonManager.CmdErrMsgTypical(err, "予約情報の取得", this) == false)
                {
                    return false;
                }

                //更新前の選択情報の保存
                var oldItems = new ListViewSelectedKeeper<ReserveItem>(listView_reserve, true);
                reserveList.Clear();

                foreach (ReserveData info in CommonManager.Instance.DB.ReserveList.Values)
                {
                    ReserveItem item = new ReserveItem(info);
                    reserveList.Add(item);
                }

                listView_reserve.DataContext = reserveList;

                if (this.gridViewSorter.IsExistSortParams)
                {
                    this.gridViewSorter.SortByMultiHeader(this.reserveList);
                }
                else
                {
                    this.gridViewSorter.ResetSortParams();
                    this.gridViewSorter.SortByMultiHeaderWithKey(this.reserveList, gridView_reserve.Columns,
                        Settings.Instance.ResColumnHead, true, Settings.Instance.ResSortDirection);
                }
                this.listView_reserve.Items.Refresh();

                //選択情報の復元
                oldItems.RestoreListViewSelected();
            }
            catch (Exception ex)
            {
                this.Dispatcher.BeginInvoke(new Action(() =>
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }), null);
                return false;
            }

            // 枠線表示用
            CommonManager.Instance.DB.ReloadEpgData();

            return true;
        }

        GridViewSorter<ReserveItem> gridViewSorter = 
            new GridViewSorter<ReserveItem>(new string[] { "RecFileName", "RecFolder" });


        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked = e.OriginalSource as GridViewColumnHeader;
            if (headerClicked != null)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {
                    this.gridViewSorter.SortByMultiHeader(this.reserveList, headerClicked);
                    listView_reserve.Items.Refresh();
                    Settings.Instance.ResColumnHead = this.gridViewSorter.LastHeader;
                    Settings.Instance.ResSortDirection = this.gridViewSorter.LastDirection;
                }
            }
        }

        private void listView_reserve_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            ChangeReserve();
        }

        private void button_change_Click(object sender, RoutedEventArgs e)
        {
            ChangeReserve();
        }

        private void ChangeReserve()
        {
            if (listView_reserve.SelectedItem != null)
            {
                ReserveItem item = SelectSingleItem();
                mutil.OpenChangeReserveWindow(item.ReserveInfo, this);
            }
        }

        private ReserveItem SelectSingleItem()
        {
            return mutil.SelectSingleItem<ReserveItem>(listView_reserve);
        }

        private void recmode_Click(object sender, RoutedEventArgs e)
        {
            mutil.ReserveChangeRecmode(GetSelectedReserveDataList(), sender);
        }

        private void button_on_off_Click(object sender, RoutedEventArgs e)
        {
            mutil.ReserveChangeOnOff(GetSelectedReserveDataList());
        }

        private void priority_Click(object sender, RoutedEventArgs e)
        {
            mutil.ReserveChangePriority(GetSelectedReserveDataList(), sender);
        }

        //冗長な処理にはなるが、リスト化した方が扱いやすいので
        private List<ReserveData> GetSelectedReserveDataList()
        {
            return listView_reserve.SelectedItems.Cast<ReserveItem>().ToList().ReserveInfoList();
        }

        private void autoadd_Click(object sender, RoutedEventArgs e)
        {
            if (listView_reserve.SelectedItem != null)
            {
                ReserveItem item = SelectSingleItem();
                mutil.SendAutoAdd(item.ReserveInfo, this);
            }
        }

        private void timeShiftPlay_Click(object sender, RoutedEventArgs e)
        {
            if (listView_reserve.SelectedItem != null)
            {
                ReserveItem item = SelectSingleItem();
                CommonManager.Instance.TVTestCtrl.StartTimeShift(item.ReserveInfo.ReserveID);
            }
        }

        private void button_del_Click(object sender, RoutedEventArgs e)
        {
            mutil.ReserveDelete(GetSelectedReserveDataList());
        }

        private void button_add_manual_Click(object sender, RoutedEventArgs e)
        {
            listView_reserve.UnselectAll();
            mutil.OpenManualReserveWindow(this);
        }

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (RedrawReserve == true && this.IsVisible == true)
            {
                if (ReDrawReserveData() == true)
                {
                    RedrawReserve = false;
                }
            }
        }

        private void ContextMenu_Header_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            try
            {
                foreach (MenuItem item in listView_reserve.ContextMenu.Items)
                {
                    item.IsChecked = false;
                    foreach (ListColumnInfo info in Settings.Instance.ReserveListColumn)
                    {
                        if (info.Tag.CompareTo(item.Name) == 0)
                        {
                            item.IsChecked = true;
                            break;
                        }
                    }
                }


            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void headerSelect_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                MenuItem menuItem = sender as MenuItem;
                if (menuItem.IsChecked == true)
                {

                    Settings.Instance.ReserveListColumn.Add(new ListColumnInfo(menuItem.Name, Double.NaN));
                    gridView_reserve.Columns.Add(columnList[menuItem.Name]);
                }
                else
                {
                    foreach (ListColumnInfo info in Settings.Instance.ReserveListColumn)
                    {
                        if (info.Tag.CompareTo(menuItem.Name) == 0)
                        {
                            Settings.Instance.ReserveListColumn.Remove(info);
                            gridView_reserve.Columns.Remove(columnList[menuItem.Name]);
                            break;
                        }
                    }
                }

            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        void listView_reserve_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.P:
                        this.timeShiftPlay_Click(this.listView_reserve.SelectedItem, new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.D:
                        this.deleteItem();
                        break;
                    case Key.S:
                        this.button_on_off_Click(this, new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.C:
                        this.CopyTitle2Clipboard();
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.F3:
                        this.MenuItem_Click_ProgramTable(this, new RoutedEventArgs(Button.ClickEvent));
                        e.Handled = true;
                        break;
                    case Key.Enter:
                        this.ChangeReserve();
                        e.Handled = true;
                        break;
                    case Key.Delete:
                        this.deleteItem();
                        e.Handled = true;
                        break;
                }
            }
        }

        void deleteItem()
        {
            if (listView_reserve.SelectedItems.Count == 0) { return; }
            //
            string text1 = "削除しますか?　[削除アイテム数: " + listView_reserve.SelectedItems.Count + "]" + "\r\n\r\n";
            GetSelectedReserveDataList().ForEach(info => text1 += " ・ " + info.Title + "\r\n");

            string caption1 = "登録項目削除の確認";
            if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel, MessageBoxImage.Exclamation, MessageBoxResult.OK) == MessageBoxResult.OK)
            {
                this.button_del_Click(this.listView_reserve.SelectedItem, new RoutedEventArgs(Button.ClickEvent));
            }
        }

        private void MenuItem_Click_ProgramTable(object sender, RoutedEventArgs e)
        {
            if (listView_reserve.SelectedItem != null)
            {
                ReserveItem item = SelectSingleItem();
                BlackoutWindow.selectedReserveItem = item;
                this._mainWindow.moveTo_tabItem_epg();
            }
        }

        private void MenuItem_Click_CopyTitle(object sender, RoutedEventArgs e)
        {
            CopyTitle2Clipboard();
        }

        private void CopyTitle2Clipboard()
        {
            if (listView_reserve.SelectedItem != null)
            {
                ReserveItem item = SelectSingleItem();
                mutil.CopyTitle2Clipboard(item.EventName);
            }
        }

        private void MenuItem_Click_CopyContent(object sender, RoutedEventArgs e)
        {
            if (listView_reserve.SelectedItem != null)
            {
                ReserveItem item = SelectSingleItem();
                mutil.CopyContent2Clipboard(item.ReserveInfo);
            }
        }
    
        private void MenuItem_Click_SearchTitle(object sender, RoutedEventArgs e)
        {
            if (listView_reserve.SelectedItem != null)
            {
                ReserveItem item = SelectSingleItem();
                mutil.SearchText(item.EventName);
            }
        }

        private void cmdMenu_Loaded(object sender, RoutedEventArgs e)
        {
            if (listView_reserve.SelectedItem != null)
            {
                try
                {
                    List<ReserveData> list = GetSelectedReserveDataList();

                    foreach (object item in ((ContextMenu)sender).Items)
                    {
                        if (item is MenuItem && ((string)((MenuItem)item).Header).StartsWith("変更"))
                        {
                            mutil.CheckChgItems((MenuItem)item, list);//現在の状態(録画モード、優先度)にチェックを入れる
                        }
                        else if (mutil.AppendMenuVisibleControl(item)) { }
                        else { }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }
            }
        }

    }
}
