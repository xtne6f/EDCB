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
        private ListViewHorizontalMouseScroller horizontalScroller = new ListViewHorizontalMouseScroller();

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
            listView_reserve.AlternationCount = Settings.Instance.ResAlternationCount;
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
            listView_reserve.ItemsSource = CommonManager.Instance.DB.ReserveList.Values.Select(info => new ReserveItem(info)).ToList();

            if (columnList.ContainsKey(Settings.Instance.ResColumnHead) == false || Settings.Instance.ResColumnHead == "RecFileName")
            {
                Settings.Instance.ResColumnHead = "StartTime";
            }
            Sort();
        }

        public void RefreshEpgData()
        {
            // 枠線表示用
            if (listView_reserve.ItemsSource != null)
            {
                CollectionViewSource.GetDefaultView(listView_reserve.ItemsSource).Refresh();
            }
        }

        private void Sort()
        {
            if (listView_reserve.ItemsSource == null)
            {
                return;
            }
            ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_reserve.ItemsSource);

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
            e.Handled = true;
        }

        private void button_change_Click(object sender, RoutedEventArgs e)
        {
            ChangeReserve();
        }

        private void ChangeReserve()
        {
            if (listView_reserve.SelectedItem != null)
            {
                var win = new ChgReserveWindow();
                ((MainWindow)Application.Current.MainWindow).SwapOwnedReserveWindow(win);
                win.SetReserveInfo(((ReserveItem)listView_reserve.SelectedItem).ReserveInfo);
                win.Show();
            }
        }

        private void recmode_Click(object sender, RoutedEventArgs e)
        {
            var list = new List<ReserveData>();
            var originalRecModeList = new List<byte>();
            foreach (ReserveItem item in listView_reserve.SelectedItems)
            {
                originalRecModeList.Add(item.ReserveInfo.RecSetting.RecMode);
                byte recMode = byte.Parse((string)((MenuItem)sender).Tag);
                item.ReserveInfo.RecSetting.RecMode = CommonManager.Instance.DB.CombineRecModeAndNoRec(recMode, item.ReserveInfo.RecSetting.IsNoRec());
                list.Add(item.ReserveInfo);
            }
            if (list.Count > 0)
            {
                string message = null;
                try
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        message = CommonManager.GetErrCodeText(err) ?? "チューナー一覧の取得でエラーが発生しました。";
                    }
                }
                catch (Exception ex)
                {
                    message = ex.ToString();
                }
                for (int i = 0; i < list.Count; i++)
                {
                    list[i].RecSetting.RecMode = originalRecModeList[i];
                }
                if (message != null)
                {
                    MessageBox.Show(message);
                }
            }
        }

        private void button_no_Click(object sender, RoutedEventArgs e)
        {
            var list = new List<ReserveData>();
            var originalRecModeList = new List<byte>();
            foreach (ReserveItem item in listView_reserve.SelectedItems)
            {
                originalRecModeList.Add(item.ReserveInfo.RecSetting.RecMode);
                byte recMode = item.ReserveInfo.RecSetting.GetRecMode();
                item.ReserveInfo.RecSetting.RecMode = CommonManager.Instance.DB.CombineRecModeAndNoRec(recMode, !item.ReserveInfo.RecSetting.IsNoRec());
                list.Add(item.ReserveInfo);
            }
            if (list.Count > 0)
            {
                string message = null;
                try
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        message = CommonManager.GetErrCodeText(err) ?? "チューナー一覧の取得でエラーが発生しました。";
                    }
                }
                catch (Exception ex)
                {
                    message = ex.ToString();
                }
                for (int i = 0; i < list.Count; i++)
                {
                    list[i].RecSetting.RecMode = originalRecModeList[i];
                }
                if (message != null)
                {
                    MessageBox.Show(message);
                }
            }
        }

        private void priority_Click(object sender, RoutedEventArgs e)
        {
            var list = new List<ReserveData>();
            var originalPriorityList = new List<byte>();
            foreach (ReserveItem item in listView_reserve.SelectedItems)
            {
                originalPriorityList.Add(item.ReserveInfo.RecSetting.Priority);
                item.ReserveInfo.RecSetting.Priority = byte.Parse((string)((MenuItem)sender).Tag);
                list.Add(item.ReserveInfo);
            }
            if (list.Count > 0)
            {
                string message = null;
                try
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        message = CommonManager.GetErrCodeText(err) ?? "チューナー一覧の取得でエラーが発生しました。";
                    }
                }
                catch (Exception ex)
                {
                    message = ex.ToString();
                }
                for (int i = 0; i < list.Count; i++)
                {
                    list[i].RecSetting.Priority = originalPriorityList[i];
                }
                if (message != null)
                {
                    MessageBox.Show(message);
                }
            }
        }

        private void autoadd_Click(object sender, RoutedEventArgs e)
        {
            if (listView_reserve.SelectedItem != null)
            {
                ReserveItem item = listView_reserve.SelectedItem as ReserveItem;

                SearchWindow search = ((MainWindow)Application.Current.MainWindow).CreateSearchWindow();

                var key = new EpgSearchKeyInfo();
                key.andKey = item.ReserveInfo.Title;
                key.serviceList.Add((long)CommonManager.Create64Key(item.ReserveInfo.OriginalNetworkID, item.ReserveInfo.TransportStreamID, item.ReserveInfo.ServiceID));

                search.SetSearchDefKey(key);
                search.Show();
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
                List<uint> list = new List<uint>();
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
                MessageBox.Show(ex.ToString());
            }
        }

        private void button_add_manual_Click(object sender, RoutedEventArgs e)
        {
            var win = new ChgReserveWindow();
            ((MainWindow)Application.Current.MainWindow).SwapOwnedReserveWindow(win);
            win.Show();
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
                if (menuItem != null && menuItem.IsCheckable)
                {
                    if (menuItem.Name == "HideButton")
                    {
                        menuItem.IsChecked = Settings.Instance.ResHideButton;
                    }
                    else
                    {
                        menuItem.IsChecked = Settings.Instance.ReserveListColumn.Any(info => info.Tag == menuItem.Name);
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

                    Settings.Instance.ReserveListColumn.Add(new ListColumnInfo(menuItem.Name, double.NaN));
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
                MessageBox.Show(ex.ToString());
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
                        if (e.IsRepeat == false)
                        {
                            timeShiftPlay_Click(sender, e);
                        }
                        e.Handled = true;
                        break;
                    case Key.R:
                        if (e.IsRepeat == false)
                        {
                            button_no_Click(sender, e);
                        }
                        e.Handled = true;
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.F3:
                        jumpToProgramTable_Click(sender, e);
                        e.Handled = true;
                        break;
                    case Key.Enter:
                        ChangeReserve();
                        e.Handled = true;
                        break;
                    case Key.Delete:
                        if (listView_reserve.SelectedItems.Count > 0 &&
                            MessageBox.Show(listView_reserve.SelectedItems.Count + "項目を削除してよろしいですか?", "確認",
                                            MessageBoxButton.OKCancel, MessageBoxImage.Question, MessageBoxResult.OK) == MessageBoxResult.OK)
                        {
                            button_del_Click(sender, e);
                        }
                        e.Handled = true;
                        break;
                }
            }
        }

        private void jumpToProgramTable_Click(object sender, RoutedEventArgs e)
        {
            ReserveItem item1 = this.listView_reserve.SelectedItem as ReserveItem;
            if (item1 != null)
            {
                ((MainWindow)Application.Current.MainWindow).SearchJumpTargetProgram(item1.ReserveInfo);
            }
        }

        private void listView_reserve_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            //選択されているすべての予約が同じ設定の場合だけチェックを表示する
            int noRec = -1;
            int recMode = -1;
            int priority = -1;
            foreach (ReserveItem item in listView_reserve.SelectedItems)
            {
                if (noRec < 0)
                {
                    noRec = item.ReserveInfo.RecSetting.IsNoRec() ? 1 : 0;
                    recMode = item.ReserveInfo.RecSetting.GetRecMode();
                    priority = item.ReserveInfo.RecSetting.Priority;
                }
                else
                {
                    noRec = noRec != (item.ReserveInfo.RecSetting.IsNoRec() ? 1 : 0) ? 2 : noRec;
                    recMode = recMode != item.ReserveInfo.RecSetting.GetRecMode() ? -1 : recMode;
                    priority = priority != item.ReserveInfo.RecSetting.Priority ? -1 : priority;
                }
            }
            var itemChg = (MenuItem)((ListViewItem)sender).ContextMenu.Items.Cast<FrameworkElement>().First(a => a.Name == "cm_chg");
            itemChg.Items.Cast<FrameworkElement>().First(a => a.Name == "cm_no").Visibility =
                (noRec == 0 ? Visibility.Visible : Visibility.Collapsed);
            itemChg.Items.Cast<FrameworkElement>().First(a => a.Name == "cm_no_inv").Visibility =
                (noRec == 1 ? Visibility.Visible : Visibility.Collapsed);
            itemChg.Items.Cast<FrameworkElement>().First(a => a.Name == "cm_no_toggle").Visibility =
                (noRec == 2 ? Visibility.Visible : Visibility.Collapsed);
            int i = itemChg.Items.IndexOf(itemChg.Items.Cast<FrameworkElement>().First(a => a.Name == "recmode_all"));
            for (int j = 0; j <= 4; j++)
            {
                ((MenuItem)itemChg.Items[i + j]).IsChecked = (j == recMode);
            }
            var itemPri = (MenuItem)itemChg.Items.Cast<FrameworkElement>().First(a => a.Name == "cm_pri");
            for (int j = 0; j < itemPri.Items.Count; j++)
            {
                ((MenuItem)itemPri.Items[j]).IsChecked = (j + 1 == priority);
            }
            itemPri.Header = string.Format((string)itemPri.Tag, priority >= 0 ? "" + priority : "*");
        }

        private void listView_reserve_MouseEnter(object sender, MouseEventArgs e)
        {
            horizontalScroller.OnMouseEnter(listView_reserve, Settings.Instance.EpgSettingList[0].MouseHorizontalScrollAuto,
                                            Settings.Instance.EpgSettingList[0].HorizontalScrollSize);
        }

        private void listView_reserve_MouseLeave(object sender, MouseEventArgs e)
        {
            horizontalScroller.OnMouseLeave();
        }
    }
}
