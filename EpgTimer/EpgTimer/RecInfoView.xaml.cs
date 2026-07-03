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
    /// RecInfoView.xaml の相互作用ロジック
    /// </summary>
    public partial class RecInfoView : UserControl
    {
        private Dictionary<string, GridViewColumn> columnList;
        private string _lastHeaderClicked2 = "";
        private ListSortDirection _lastDirection2 = ListSortDirection.Ascending;
        private ListViewHorizontalMouseScroller horizontalScroller = new ListViewHorizontalMouseScroller();
        private bool ReloadInfo = true;

        public RecInfoView()
        {
            InitializeComponent();

            columnList = gridView_recinfo.Columns.ToDictionary(info => (string)((GridViewColumnHeader)info.Header).Tag);
            gridView_recinfo.Columns.Clear();
            foreach (ListColumnInfo info in Settings.Instance.RecInfoListColumn)
            {
                if (columnList.ContainsKey(info.Tag))
                {
                    columnList[info.Tag].Width = info.Width;
                    gridView_recinfo.Columns.Add(columnList[info.Tag]);
                }
            }
            if (Settings.Instance.RecInfoHideButton)
            {
                stackPanel_button.Visibility = Visibility.Collapsed;
            }
            listView_recinfo.AlternationCount = Settings.Instance.RecEndAlternationCount;
        }

        public void SaveSize()
        {
            Settings.Instance.RecInfoListColumn.Clear();
            Settings.Instance.RecInfoListColumn.AddRange(
                gridView_recinfo.Columns.Select(info => new ListColumnInfo((string)((GridViewColumnHeader)info.Header).Tag, info.Width)));
        }

        private void button_del_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView_recinfo.SelectedItems.Count > 0)
                {
                    if ((sender is ListViewItem) || Settings.Instance.ConfirmDelRecInfo)
                    {
                        bool hasPath = listView_recinfo.SelectedItems.Cast<RecInfoItem>().Any(info => info.RecFilePath.Length > 0);
                        if ((hasPath || (sender is ListViewItem) || Settings.Instance.ConfirmDelRecInfoAlways) &&
                            MessageBox.Show(listView_recinfo.SelectedItems.Count + "項目を削除してよろしいですか?" +
                                            (hasPath ? "\r\n\r\n「録画ファイルも削除する」設定が有効な場合、ファイルも削除されます。" : ""), "確認",
                                            MessageBoxButton.OKCancel, MessageBoxImage.Question, MessageBoxResult.OK) != MessageBoxResult.OK)
                        {
                            return;
                        }
                    }
                    List<uint> IDList = new List<uint>();
                    foreach (RecInfoItem info in listView_recinfo.SelectedItems)
                    {
                        IDList.Add(info.RecInfo.ID);
                    }
                    CommonManager.CreateSrvCtrl().SendDelRecInfo(IDList);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void Sort()
        {
            if (listView_recinfo.ItemsSource == null)
            {
                return;
            }
            ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_recinfo.ItemsSource);

            using (dataView.DeferRefresh())
            {
                dataView.SortDescriptions.Clear();

                dataView.SortDescriptions.Add(new SortDescription(Settings.Instance.RecInfoColumnHead, Settings.Instance.RecInfoSortDirection));
                if (columnList.ContainsKey(_lastHeaderClicked2))
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
                    if (header != Settings.Instance.RecInfoColumnHead)
                    {
                        _lastHeaderClicked2 = Settings.Instance.RecInfoColumnHead;
                        _lastDirection2 = Settings.Instance.RecInfoSortDirection;
                        Settings.Instance.RecInfoColumnHead = header;
                        Settings.Instance.RecInfoSortDirection = ListSortDirection.Ascending;
                    }
                    else if (Settings.Instance.RecInfoSortDirection == ListSortDirection.Ascending)
                    {
                        Settings.Instance.RecInfoSortDirection = ListSortDirection.Descending;
                    }
                    else
                    {
                        Settings.Instance.RecInfoSortDirection = ListSortDirection.Ascending;
                    }
                    Sort();
                }
            }
        }

        public bool ReloadInfoData()
        {
            if (CommonManager.Instance.NWMode && CommonManager.Instance.NWConnectedIP == null)
            {
                listView_recinfo.ItemsSource = null;
                return false;
            }
            ErrCode err = CommonManager.Instance.DB.ReloadrecFileInfo();
            if (err != ErrCode.CMD_SUCCESS)
            {
                Dispatcher.BeginInvoke(new Action(() => MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "情報の取得でエラーが発生しました。")));
                listView_recinfo.ItemsSource = null;
                return false;
            }
            listView_recinfo.ItemsSource = CommonManager.Instance.DB.RecFileInfo.Values.Select(info => new RecInfoItem(info)).ToList();

            if (columnList.ContainsKey(Settings.Instance.RecInfoColumnHead) == false)
            {
                Settings.Instance.RecInfoColumnHead = "StartTime";
            }
            Sort();
            return true;
        }

        /// <summary>
        /// リストの更新通知
        /// </summary>
        public void UpdateInfo()
        {
            ReloadInfo = true;
            if (this.IsVisible == true)
            {
                if (ReloadInfoData() == true)
                {
                    ReloadInfo = false;
                }
            }
        }

        private void listView_recinfo_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            if (Settings.Instance.PlayDClick)
            {
                button_play_Click(sender, e);
            }
            else
            {
                button_recInfo_Click(sender, e);
            }
            e.Handled = true;
        }

        private void listView_recinfo_KeyDown(object sender, KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.P:
                        if (e.IsRepeat == false)
                        {
                            button_play_Click(sender, e);
                        }
                        e.Handled = true;
                        break;
                    case Key.O:
                        if (e.IsRepeat == false)
                        {
                            openFolder_Click(sender, e);
                        }
                        e.Handled = true;
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.Enter:
                        button_recInfo_Click(sender, e);
                        e.Handled = true;
                        break;
                    case Key.Delete:
                        button_del_Click(sender, e);
                        e.Handled = true;
                        break;
                }
            }
        }

        private void button_play_Click(object sender, RoutedEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null)
            {
                RecFileInfo info = ((RecInfoItem)listView_recinfo.SelectedItem).RecInfo;
                if (info.RecFilePath.Length > 0)
                {
                    CommonManager.Instance.FilePlay(info.RecFilePath);
                }
            }
        }

        private void autoadd_Click(object sender, RoutedEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null)
            {
                RecInfoItem item = listView_recinfo.SelectedItem as RecInfoItem;

                SearchWindow search = ((MainWindow)Application.Current.MainWindow).CreateSearchWindow();

                var key = new EpgSearchKeyInfo();
                key.andKey = item.RecInfo.Title;
                key.serviceList.Add((long)CommonManager.Create64Key(item.RecInfo.OriginalNetworkID, item.RecInfo.TransportStreamID, item.RecInfo.ServiceID));

                search.SetSearchDefKey(key);
                search.Show();
            }
        }

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (ReloadInfo == true && this.IsVisible == true)
            {
                if (ReloadInfoData() == true)
                {
                    ReloadInfo = false;
                }
            }
        }

        private void button_recInfo_Click(object sender, RoutedEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null)
            {
                RecFileInfo info = ((RecInfoItem)listView_recinfo.SelectedItem).RecInfo;
                var win = new RecInfoDescWindow();
                ((MainWindow)Application.Current.MainWindow).SwapOwnedReserveWindow(win);
                RecFileInfo extraRecInfo = new RecFileInfo();
                if (CommonManager.CreateSrvCtrl().SendGetRecInfo(info.ID, ref extraRecInfo) == ErrCode.CMD_SUCCESS)
                {
                    info.ProgramInfo = extraRecInfo.ProgramInfo;
                    info.ErrInfo = extraRecInfo.ErrInfo;
                }
                win.SetRecInfo(info);
                win.Show();
            }
        }

        private void openFolder_Click(object sender, RoutedEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null)
            {
                string folderPath = "";
                RecInfoItem info = listView_recinfo.SelectedItem as RecInfoItem;
                if (info.RecFilePath.Length > 0)
                {
                    string filePath =
                        CommonManager.ReplaceText(info.RecFilePath, CommonManager.CreateReplaceDictionary(Settings.Instance.FilePathReplacePattern));
                    try
                    {
                        if (System.IO.File.Exists(filePath))
                        {
                            using (System.Diagnostics.Process.Start("EXPLORER.EXE", "/select,\"" + filePath + "\"")) { }
                            return;
                        }
                        try
                        {
                            folderPath = System.IO.Path.GetDirectoryName(filePath);
                        }
                        catch { }
                        if (System.IO.Directory.Exists(folderPath))
                        {
                            using (System.Diagnostics.Process.Start("EXPLORER.EXE", "\"" + folderPath + "\"")) { }
                            return;
                        }
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.ToString());
                        return;
                    }
                }
                MessageBox.Show("録画フォルダ" + (folderPath.Length > 0 ? " \"" + folderPath + "\" " : "") + "が存在しません");
            }
        }


        private void ContextMenu_Header_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            foreach (object item in listView_recinfo.ContextMenu.Items)
            {
                MenuItem menuItem = item as MenuItem;
                if (menuItem != null)
                {
                    if (menuItem.Name == "HideButton")
                    {
                        menuItem.IsChecked = Settings.Instance.RecInfoHideButton;
                    }
                    else
                    {
                        menuItem.IsChecked = Settings.Instance.RecInfoListColumn.Any(info => info.Tag == menuItem.Name);
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

                    Settings.Instance.RecInfoListColumn.Add(new ListColumnInfo(menuItem.Name, double.NaN));
                    gridView_recinfo.Columns.Add(columnList[menuItem.Name]);
                }
                else
                {
                    foreach (ListColumnInfo info in Settings.Instance.RecInfoListColumn)
                    {
                        if (info.Tag == menuItem.Name)
                        {
                            Settings.Instance.RecInfoListColumn.Remove(info);
                            gridView_recinfo.Columns.Remove(columnList[menuItem.Name]);
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
            Settings.Instance.RecInfoHideButton = ((MenuItem)sender).IsChecked;
            stackPanel_button.Visibility = Settings.Instance.RecInfoHideButton ? Visibility.Collapsed : Visibility.Visible;
        }

        private void listView_recinfo_MouseEnter(object sender, MouseEventArgs e)
        {
            horizontalScroller.OnMouseEnter(listView_recinfo, Settings.Instance.EpgSettingList[0].MouseHorizontalScrollAuto,
                                            Settings.Instance.EpgSettingList[0].HorizontalScrollSize);
        }

        private void listView_recinfo_MouseLeave(object sender, MouseEventArgs e)
        {
            horizontalScroller.OnMouseLeave();
        }
    }
}
