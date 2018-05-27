using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.ComponentModel;
using System.Collections.ObjectModel;
using System.Collections;


namespace EpgTimer
{
    /// <summary>
    /// ReserveView.xaml の相互作用ロジック
    /// </summary>
    public partial class ReserveView : UserControl
    {
        private bool RedrawReserve = true;
        private Dictionary<string, GridViewColumn> columnList;
        string _lastHeaderClicked2 = "";
        ListSortDirection _lastDirection2 = ListSortDirection.Ascending;

        public ReserveView()
        {
            InitializeComponent();

            columnList = gridView_reserve.Columns.ToDictionary(info => (string)((GridViewColumnHeader)info.Header).Tag);
            gridView_reserve.Columns.Clear();
            foreach (ListColumnInfo info in Settings.Instance.ReserveListColumn)
            {
                if (columnList.ContainsKey(info.Tag))
                {
                    columnList[info.Tag].Width = info.Width;
                    gridView_reserve.Columns.Add(columnList[info.Tag]);
                }
            }
            if (Settings.Instance.ResHideButton)
            {
                stackPanel_button.Visibility = Visibility.Collapsed;
            }
        }


        public void SaveSize()
        {
            Settings.Instance.ReserveListColumn.Clear();
            Settings.Instance.ReserveListColumn.AddRange(
                gridView_reserve.Columns.Select(info => new ListColumnInfo((string)((GridViewColumnHeader)info.Header).Tag, info.Width)));
        }

        /// <summary>
        /// 予約情報の更新通知
        /// </summary>
        public void Refresh()
        {
            RedrawReserve = true;
            if (this.IsVisible == true)
            {
                ReDrawReserveData();
                RedrawReserve = false;
            }
        }

        private void ReDrawReserveData()
        {
            listView_reserve.DataContext = CommonManager.Instance.DB.ReserveList.Values.Select(info => new ReserveItem(info)).ToList();

            if (columnList.ContainsKey(Settings.Instance.ResColumnHead) == false || Settings.Instance.ResColumnHead == "RecFileName")
            {
                Settings.Instance.ResColumnHead = "StartTime";
            }
            Sort();
        }

        public void RefreshEpgData()
        {
            // 枠線表示用
            if (listView_reserve.DataContext != null)
            {
                CollectionViewSource.GetDefaultView(listView_reserve.DataContext).Refresh();
            }
        }

        private void Sort()
        {
            if (listView_reserve.DataContext == null)
            {
                return;
            }
            ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_reserve.DataContext);

            using (dataView.DeferRefresh())
            {
                dataView.SortDescriptions.Clear();

                dataView.SortDescriptions.Add(new SortDescription(Settings.Instance.ResColumnHead, Settings.Instance.ResSortDirection));
                if (columnList.ContainsKey(_lastHeaderClicked2) && _lastHeaderClicked2 != "RecFileName")
                {
                    dataView.SortDescriptions.Add(new SortDescription(_lastHeaderClicked2, _lastDirection2));
                }
            }
        }

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked = e.OriginalSource as GridViewColumnHeader;

            if (headerClicked != null)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {
                    string header = headerClicked.Tag as string;
                    if (header == "RecFileName")
                    {
                        return;
                    }

                    if (header != Settings.Instance.ResColumnHead)
                    {
                        _lastHeaderClicked2 = Settings.Instance.ResColumnHead;
                        _lastDirection2 = Settings.Instance.ResSortDirection;
                        Settings.Instance.ResColumnHead = header;
                        Settings.Instance.ResSortDirection = ListSortDirection.Ascending;
                    }
                    else if (Settings.Instance.ResSortDirection == ListSortDirection.Ascending)
                    {
                        Settings.Instance.ResSortDirection = ListSortDirection.Descending;
                    }
                    else
                    {
                        Settings.Instance.ResSortDirection = ListSortDirection.Ascending;
                    }
                    Sort();
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
                ReserveItem item = listView_reserve.SelectedItem as ReserveItem;
                ChgReserveWindow dlg = new ChgReserveWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetReserveInfo(item.ReserveInfo);
                if (dlg.ShowDialog() == true)
                {
                }
            }
        }

        private void recmode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                byte recMode = byte.Parse(((MenuItem)sender).Name.Substring("recmode_".Length));
                List<ReserveData> list = new List<ReserveData>();
                foreach (ReserveItem item in listView_reserve.SelectedItems)
                {
                    ReserveData reserveInfo = item.ReserveInfo;
                    reserveInfo.RecSetting.RecMode = recMode;

                    list.Add(reserveInfo);
                }
                if (list.Count > 0)
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "チューナー一覧の取得でエラーが発生しました。");
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_no_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                List<ReserveData> list = new List<ReserveData>();
                foreach (ReserveItem item in listView_reserve.SelectedItems)
                {
                    ReserveData reserveInfo = item.ReserveInfo;

                    reserveInfo.RecSetting.RecMode = 5;

                    list.Add(reserveInfo);
                }
                if (list.Count > 0)
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "チューナー一覧の取得でエラーが発生しました。");
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void priority_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                byte priority = byte.Parse(((MenuItem)sender).Name.Substring("priority_".Length));
                List<ReserveData> list = new List<ReserveData>();
                foreach (ReserveItem item in listView_reserve.SelectedItems)
                {
                    ReserveData reserveInfo = item.ReserveInfo;
                    reserveInfo.RecSetting.Priority = priority;

                    list.Add(reserveInfo);
                }
                if (list.Count > 0)
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "チューナー一覧の取得でエラーが発生しました。");
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void autoadd_Click(object sender, RoutedEventArgs e)
        {
            if (listView_reserve.SelectedItem != null)
            {
                SearchWindow dlg = new SearchWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetViewMode(1);

                EpgSearchKeyInfo key = new EpgSearchKeyInfo();

                ReserveItem item = listView_reserve.SelectedItem as ReserveItem;

                key.andKey = item.ReserveInfo.Title;
                Int64 sidKey = ((Int64)item.ReserveInfo.OriginalNetworkID) << 32 | ((Int64)item.ReserveInfo.TransportStreamID) << 16 | ((Int64)item.ReserveInfo.ServiceID);
                key.serviceList.Add(sidKey);

                dlg.SetSearchDefKey(key);
                dlg.ShowDialog();
            }
        }

        private void timeShiftPlay_Click(object sender, RoutedEventArgs e)
        {
            if (listView_reserve.SelectedItem != null)
            {
                ReserveItem info = listView_reserve.SelectedItem as ReserveItem;
                CommonManager.Instance.FilePlay(info.ReserveInfo.ReserveID);
            }
        }

        private void button_del_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                List<UInt32> list = new List<UInt32>();
                foreach (ReserveItem item in listView_reserve.SelectedItems)
                {
                    ReserveData reserveInfo = item.ReserveInfo;

                    list.Add(reserveInfo.ReserveID);
                }
                if (list.Count > 0)
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendDelReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "チューナー一覧の取得でエラーが発生しました。");
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_add_manual_Click(object sender, RoutedEventArgs e)
        {
            ChgReserveWindow dlg = new ChgReserveWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            dlg.AddReserveMode(true);
            dlg.ShowDialog();
        }

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (RedrawReserve == true && this.IsVisible == true)
            {
                ReDrawReserveData();
                RedrawReserve = false;
            }
        }

        private void ContextMenu_Header_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            foreach (object item in listView_reserve.ContextMenu.Items)
            {
                MenuItem menuItem = item as MenuItem;
                if (menuItem != null)
                {
                    if (menuItem.Name == "HideButton")
                    {
                        menuItem.IsChecked = Settings.Instance.ResHideButton;
                    }
                    else
                    {
                        menuItem.IsChecked = false;
                        foreach (ListColumnInfo info in Settings.Instance.ReserveListColumn)
                        {
                            if (info.Tag == menuItem.Name)
                            {
                                menuItem.IsChecked = true;
                                break;
                            }
                        }
                    }
                }
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
                        if (info.Tag == menuItem.Name)
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

        private void hideButton_Click(object sender, RoutedEventArgs e)
        {
            Settings.Instance.ResHideButton = ((MenuItem)sender).IsChecked;
            stackPanel_button.Visibility = Settings.Instance.ResHideButton ? Visibility.Collapsed : Visibility.Visible;
        }

        void listView_reserve_KeyDown(object sender, KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.P:
                        this.timeShiftPlay_Click(this.listView_reserve.SelectedItem, new RoutedEventArgs(Button.ClickEvent));
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.F3:
                        this.MenuItem_Click_ProgramTable(this, new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.Enter:
                        this.button_change_Click(this.listView_reserve.SelectedItem, new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.Delete:
                        this.deleteItem();
                        break;
                }
            }
        }

        void deleteItem()
        {
            if (listView_reserve.SelectedItems.Count == 0) { return; }
            //
            try
            {
                string text1 = "削除しますか？" + "　[削除アイテム数: " + listView_reserve.SelectedItems.Count + "]" + "\n\n";
                List<UInt32> dataIDList = new List<uint>();
                foreach (ReserveItem info in listView_reserve.SelectedItems)
                {
                    text1 += " ・ " + info.ReserveInfo.Title + "\n";
                }
                string caption1 = "登録項目削除の確認";
                if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel, MessageBoxImage.Exclamation, MessageBoxResult.OK) == MessageBoxResult.OK)
                {
                    this.button_del_Click(this.listView_reserve.SelectedItem, new RoutedEventArgs(Button.ClickEvent));
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        void setPriority(int priority0)
        {
            try
            {
                List<ReserveData> list = new List<ReserveData>();
                foreach (ReserveItem item in listView_reserve.SelectedItems)
                {
                    ReserveData reserveInfo = item.ReserveInfo;
                    reserveInfo.RecSetting.Priority = BitConverter.GetBytes(priority0)[0]; ;
                    list.Add(reserveInfo);
                }
                if (list.Count > 0)
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "チューナー一覧の取得でエラーが発生しました。");
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        void setRecMode(int redMode0)
        {
            try
            {
                List<ReserveData> list = new List<ReserveData>();
                foreach (ReserveItem item in listView_reserve.SelectedItems)
                {
                    ReserveData reserveInfo = item.ReserveInfo;
                    reserveInfo.RecSetting.RecMode = BitConverter.GetBytes(redMode0)[0];
                    list.Add(reserveInfo);
                }
                if (list.Count > 0)
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "チューナー一覧の取得でエラーが発生しました。");
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void MenuItem_Click_ProgramTable(object sender, RoutedEventArgs e)
        {
            ReserveItem item1 = this.listView_reserve.SelectedItem as ReserveItem;
            if (item1 != null)
            {
                ((MainWindow)Application.Current.MainWindow).SearchJumpTargetProgram(item1.ReserveInfo);
            }
        }

        private void cmdMenu_Loaded(object sender, RoutedEventArgs e)
        {
            //選択されているすべての予約が同じ設定の場合だけチェックを表示する
            byte recMode = 0xFF;
            byte priority = 0xFF;
            foreach (ReserveItem item in listView_reserve.SelectedItems)
            {
                if (recMode == 0xFF)
                {
                    recMode = item.ReserveInfo.RecSetting.RecMode;
                }
                else if (recMode != item.ReserveInfo.RecSetting.RecMode)
                {
                    recMode = 0xFE;
                }
                if (priority == 0xFF)
                {
                    priority = item.ReserveInfo.RecSetting.Priority;
                }
                else if (priority != item.ReserveInfo.RecSetting.Priority)
                {
                    priority = 0xFE;
                }
            }
            foreach (object item in ((ContextMenu)sender).Items)
            {
                if (item is MenuItem && ((string)((MenuItem)item).Header).StartsWith("変更", StringComparison.Ordinal))
                {
                    for (int i = 0; i < ((MenuItem)item).Items.Count; i++)
                    {
                        MenuItem subItem = ((MenuItem)item).Items[i] as MenuItem;
                        if (subItem != null && subItem.Name == "recmode_0")
                        {
                            for (int j = 0; j <= 5; j++)
                            {
                                ((MenuItem)((MenuItem)item).Items[i + j]).IsChecked = (j == recMode);
                            }
                        }
                        if (subItem != null && subItem.Name == "cm_pri")
                        {
                            for (int j = 0; j < subItem.Items.Count; j++)
                            {
                                ((MenuItem)subItem.Items[j]).IsChecked = (j + 1 == priority);
                            }
                            subItem.Header = string.Format((string)subItem.Tag, priority < 0xFE ? "" + priority : "*");
                        }
                    }
                    break;
                }
            }
        }

    }
}
