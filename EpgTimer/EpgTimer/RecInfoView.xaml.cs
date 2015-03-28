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
    /// RecInfoView.xaml の相互作用ロジック
    /// </summary>
    public partial class RecInfoView : UserControl
    {
        private List<RecInfoItem> resultList = new List<RecInfoItem>();
        private Dictionary<String, GridViewColumn> columnList = new Dictionary<String, GridViewColumn>();

        private string _lastHeaderClicked = null;
        private ListSortDirection _lastDirection = ListSortDirection.Ascending;
        private string _lastHeaderClicked2 = null;
        private ListSortDirection _lastDirection2 = ListSortDirection.Ascending;

        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private MenuUtil mutil = CommonManager.Instance.MUtil;

        private bool ReloadInfo = true;

        public RecInfoView()
        {
            InitializeComponent();

            try
            {
                if (Settings.Instance.NoStyle == 1)
                {
                    button_del.Style = null;
                    button_play.Style = null;
                }

                foreach (GridViewColumn info in gridView_recinfo.Columns)
                {
                    GridViewColumnHeader header = info.Header as GridViewColumnHeader;
                    columnList.Add((string)header.Tag, info);
                }
                gridView_recinfo.Columns.Clear();

                foreach (ListColumnInfo info in Settings.Instance.RecInfoListColumn)
                {
                    columnList[info.Tag].Width = info.Width;
                    gridView_recinfo.Columns.Add(columnList[info.Tag]);
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
                Settings.Instance.RecInfoListColumn.Clear();
                foreach (GridViewColumn info in gridView_recinfo.Columns)
                {
                    GridViewColumnHeader header = info.Header as GridViewColumnHeader;

                    Settings.Instance.RecInfoListColumn.Add(new ListColumnInfo((String)header.Tag, info.Width));
                }

            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void ChgProtectRecInfo()
        {
            try
            {
                if (listView_recinfo.SelectedItems.Count > 0)
                {
                    List<RecFileInfo> list = new List<RecFileInfo>();
                    foreach (RecInfoItem info in listView_recinfo.SelectedItems)
                    {
                        info.RecInfo.ProtectFlag = !info.RecInfo.ProtectFlag;
                        list.Add(info.RecInfo);
                    }
                    cmd.SendChgProtectRecInfo(list);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_protect_Click(object sender, RoutedEventArgs e)
        {
            ChgProtectRecInfo();
        }

        private void button_del_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView_recinfo.SelectedItems.Count > 0)
                {
                    if (IniFileHandler.GetPrivateProfileInt("SET", "RecInfoDelFile", 0, SettingPath.CommonIniPath) == 1)
                    {
                        if (MessageBox.Show("録画ファイルが存在する場合は一緒に削除されます。\r\nよろしいですか？", "ファイル削除", MessageBoxButton.OKCancel) != MessageBoxResult.OK)
                        {
                            return;
                        }
                    }
                    List<UInt32> IDList = new List<uint>();
                    listView_recinfo.SelectedItems.Cast<RecInfoItem>().ToList().ForEach(
                        info => IDList.Add(info.RecInfo.ID));
                    cmd.SendDelRecInfo(IDList);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void Sort(string sortBy, ListSortDirection direction)
        {
            try
            {
                ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_recinfo.DataContext);

                dataView.SortDescriptions.Clear();

                SortDescription sd = new SortDescription(sortBy, direction);
                dataView.SortDescriptions.Add(sd);
                if (_lastHeaderClicked2 != null)
                {
                    if (String.Compare(sortBy, _lastHeaderClicked2) != 0)
                    {
                        SortDescription sd2 = new SortDescription(_lastHeaderClicked2, _lastDirection2);
                        dataView.SortDescriptions.Add(sd2);
                    }
                }
                dataView.Refresh();

                Settings.Instance.RecInfoColumnHead = sortBy;
                Settings.Instance.RecInfoSortDirection = direction;

            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked = e.OriginalSource as GridViewColumnHeader;
            ListSortDirection direction;

            if (headerClicked != null)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {
                    string header = headerClicked.Tag as string;
                    if (String.Compare(header, _lastHeaderClicked) != 0)
                    {
                        direction = ListSortDirection.Ascending;
                        _lastHeaderClicked2 = _lastHeaderClicked;
                        _lastDirection2 = _lastDirection;
                    }
                    else
                    {
                        if (_lastDirection == ListSortDirection.Ascending)
                        {
                            direction = ListSortDirection.Descending;
                        }
                        else
                        {
                            direction = ListSortDirection.Ascending;
                        }
                    }

                    Sort(header, direction);

                    _lastHeaderClicked = header;
                    _lastDirection = direction;
                }
            }
        }

        public bool ReloadInfoData()
        {
            try
            {
                //更新前の選択情報の保存
                var oldItems = new ListViewSelectedKeeper<RecInfoItem>(listView_recinfo, true);

                ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_recinfo.DataContext);
                if (dataView != null)
                {
                    dataView.SortDescriptions.Clear();
                    dataView.Refresh();
                }
                listView_recinfo.DataContext = null;
                resultList.Clear();

                if (CommonManager.Instance.NWMode == true)
                {
                    if (CommonManager.Instance.NW.IsConnected == false)
                    {
                        return false;
                    }
                }
                ErrCode err = CommonManager.Instance.DB.ReloadrecFileInfo();
                if (CommonManager.CmdErrMsgTypical(err, "録画情報の取得", this) == false)
                {
                    return false;
                }

                foreach (RecFileInfo info in CommonManager.Instance.DB.RecFileInfo.Values)
                {
                    RecInfoItem item = new RecInfoItem(info);
                    resultList.Add(item);
                }

                listView_recinfo.DataContext = resultList;
                if (_lastHeaderClicked != null)
                {
                    //GridViewColumnHeader columnHeader = _lastHeaderClicked.Header as GridViewColumnHeader;
                    //string header = columnHeader.Tag as string;
                    Sort(_lastHeaderClicked, _lastDirection);
                }
                else
                {
                    bool sort = false;
                    foreach (GridViewColumn info in gridView_recinfo.Columns)
                    {
                        GridViewColumnHeader columnHeader = info.Header as GridViewColumnHeader;
                        string header = columnHeader.Tag as string;
                        if (String.Compare(header, Settings.Instance.RecInfoColumnHead, true) == 0)
                        {
                            Sort(header, Settings.Instance.RecInfoSortDirection);
                            _lastHeaderClicked = header;
                            _lastDirection = Settings.Instance.RecInfoSortDirection;
                            sort = true;
                            break;
                        }
                    }
                    if (gridView_recinfo.Columns.Count > 0 && sort == false)
                    {
                        GridViewColumnHeader columnHeader = gridView_recinfo.Columns[1].Header as GridViewColumnHeader;
                        string header = columnHeader.Tag as string;

                        Sort(header, _lastDirection);
                        _lastHeaderClicked = header;
                    }
                }

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

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (ReloadInfo == true && this.IsVisible == true)
            {
                if (ReloadInfoData() == true)
                {
                    ReloadInfo = false;
                }
            }
        }

        void listView_recinfo_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.P:
                        this.button_play_Click(this.listView_recinfo.SelectedItem, new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.S:
                        this.ChgProtectRecInfo();
                        break;
                    case Key.D:
                        this.deleteItem();
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
                    case Key.Enter:
                        this.button_recInfo_Click(this.listView_recinfo.SelectedItem, new RoutedEventArgs(Button.ClickEvent));
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
            if (listView_recinfo.SelectedItems.Count == 0) { return; }
            //
            string text1 = "削除しますか?　[削除アイテム数: " + listView_recinfo.SelectedItems.Count + "]" + "\r\n\r\n";
            string caption1 = "項目削除の確認";
            if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel, MessageBoxImage.Exclamation, MessageBoxResult.OK) == MessageBoxResult.OK)
            {
                this.button_del_Click(this.listView_recinfo.SelectedItem, new RoutedEventArgs(Button.ClickEvent));
            }
        }

        private void listView_recinfo_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null)
            {
                if (Settings.Instance.PlayDClick == false)
                {
                    RecInfoItem info = (RecInfoItem)listView_recinfo.SelectedItem;
                    RecInfoDescWindow dlg = new RecInfoDescWindow();
                    dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                    dlg.SetRecInfo(info.RecInfo);
                    dlg.ShowDialog();
                }
                else
                {
                    button_play_Click(sender, e);
                }
            }
        }

        private RecInfoItem SelectSingleItem()
        {
            return mutil.SelectSingleItem<RecInfoItem>(listView_recinfo);
        }

        private void button_play_Click(object sender, RoutedEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null)
            {
                RecInfoItem info = SelectSingleItem();
                if (info.RecInfo.RecFilePath.Length > 0)
                {
                    CommonManager.Instance.FilePlay(info.RecInfo.RecFilePath);
                }
            }
        }

        private void autoadd_Click(object sender, RoutedEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null)
            {
                RecInfoItem info = SelectSingleItem();
                mutil.SendAutoAdd(info.RecInfo, this);
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
                RecInfoItem info = SelectSingleItem();
                RecInfoDescWindow dlg = new RecInfoDescWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetRecInfo(info.RecInfo);
                dlg.ShowDialog();
            }
        }

        private void openFolder_Click(object sender, RoutedEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null)
            {
                RecInfoItem info = SelectSingleItem();

                if (CommonManager.Instance.NWMode == false)//一応残す
                {
                    if (info.RecFilePath.Length == 0)
                    {
                        MessageBox.Show("録画ファイルが存在しません");
                    }
                    else
                    {
                        if (System.IO.File.Exists(info.RecFilePath) == true)
                        {
                            String cmd = "/select,";
                            cmd += "\"" + info.RecFilePath + "\"";

                            System.Diagnostics.Process.Start("EXPLORER.EXE", cmd);
                        }
                        else
                        {
                            String folderPath = System.IO.Path.GetDirectoryName(info.RecFilePath);
                            System.Diagnostics.Process.Start("EXPLORER.EXE", folderPath);
                        }
                    }
                }
            }
        }

        private void ContextMenu_Header_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            try
            {
                foreach (MenuItem item in listView_recinfo.ContextMenu.Items)
                {
                    item.IsChecked = false;
                    foreach (ListColumnInfo info in Settings.Instance.RecInfoListColumn)
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

                    Settings.Instance.RecInfoListColumn.Add(new ListColumnInfo(menuItem.Name, Double.NaN));
                    gridView_recinfo.Columns.Add(columnList[menuItem.Name]);
                }
                else
                {
                    foreach (ListColumnInfo info in Settings.Instance.RecInfoListColumn)
                    {
                        if (info.Tag.CompareTo(menuItem.Name) == 0)
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
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void MenuItem_Click_CopyTitle(object sender, RoutedEventArgs e)
        {
            CopyTitle2Clipboard();
        }

        private void CopyTitle2Clipboard()
        {
            if (listView_recinfo.SelectedItem != null)
            {
                RecInfoItem info = SelectSingleItem();
                mutil.CopyTitle2Clipboard(info.EventName);
            }
        }

        private void MenuItem_Click_CopyContent(object sender, RoutedEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null)
            {
                RecInfoItem info = SelectSingleItem();
                mutil.CopyContent2Clipboard(info);
            }
        }

        private void MenuItem_Click_SearchTitle(object sender, RoutedEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null)
            {
                RecInfoItem info = SelectSingleItem();
                mutil.SearchText(info.EventName);
            }
        }

        private void cmdMenu_Loaded(object sender, RoutedEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null)
            {
                try
                {
                    foreach (object item in ((ContextMenu)sender).Items)
                    {
                        if (item is MenuItem && ((((MenuItem)item).Name == "cmdopenFolder")))
                        {
                            if (CommonManager.Instance.NWMode == true)
                            {
                                ((MenuItem)item).Visibility = System.Windows.Visibility.Collapsed;
                            }
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
