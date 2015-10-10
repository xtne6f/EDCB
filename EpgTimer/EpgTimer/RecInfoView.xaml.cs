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

        private bool ReloadInfo = true;

        public RecInfoView()
        {
            InitializeComponent();

            try
            {
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
                /*
                if (Settings.Instance.RecInfoColumnWidth0 != 0)
                {
                    gridView_recinfo.Columns[1].Width = Settings.Instance.RecInfoColumnWidth0;
                }
                if (Settings.Instance.RecInfoColumnWidth1 != 0)
                {
                    gridView_recinfo.Columns[2].Width = Settings.Instance.RecInfoColumnWidth1;
                }
                if (Settings.Instance.RecInfoColumnWidth2 != 0)
                {
                    gridView_recinfo.Columns[3].Width = Settings.Instance.RecInfoColumnWidth2;
                }
                if (Settings.Instance.RecInfoColumnWidth3 != 0)
                {
                    gridView_recinfo.Columns[4].Width = Settings.Instance.RecInfoColumnWidth3;
                }
                if (Settings.Instance.RecInfoColumnWidth4 != 0)
                {
                    gridView_recinfo.Columns[5].Width = Settings.Instance.RecInfoColumnWidth4;
                }
                if (Settings.Instance.RecInfoColumnWidth5 != 0)
                {
                    gridView_recinfo.Columns[6].Width = Settings.Instance.RecInfoColumnWidth5;
                }
                if (Settings.Instance.RecInfoColumnWidth6 != 0)
                {
                    gridView_recinfo.Columns[7].Width = Settings.Instance.RecInfoColumnWidth6;
                }*/
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
                /*
                Settings.Instance.RecInfoColumnWidth0 = gridView_recinfo.Columns[1].Width;
                Settings.Instance.RecInfoColumnWidth1 = gridView_recinfo.Columns[2].Width;
                Settings.Instance.RecInfoColumnWidth2 = gridView_recinfo.Columns[3].Width;
                Settings.Instance.RecInfoColumnWidth3 = gridView_recinfo.Columns[4].Width;
                Settings.Instance.RecInfoColumnWidth4 = gridView_recinfo.Columns[5].Width;
                Settings.Instance.RecInfoColumnWidth5 = gridView_recinfo.Columns[6].Width;
                Settings.Instance.RecInfoColumnWidth6 = gridView_recinfo.Columns[7].Width;
                */
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
                    foreach (RecInfoItem info in listView_recinfo.SelectedItems)
                    {
                        IDList.Add(info.RecInfo.ID);
                    }
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
                ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_recinfo.DataContext);
                if (dataView != null)
                {
                    dataView.SortDescriptions.Clear();
                    dataView.Refresh();
                }
                listView_recinfo.DataContext = null;
                resultList.Clear();

                ErrCode err = CommonManager.Instance.DB.ReloadrecFileInfo();
                if (err == ErrCode.CMD_ERR_CONNECT)
                {
                    this.Dispatcher.BeginInvoke(new Action(() =>
                    {
                        MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                    }), null);
                    return false;
                }
                if (err == ErrCode.CMD_ERR_TIMEOUT)
                {
                    this.Dispatcher.BeginInvoke(new Action(() =>
                    {
                        MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                    }), null);
                    return false;
                }
                if (err != ErrCode.CMD_SUCCESS)
                {
                    this.Dispatcher.BeginInvoke(new Action(() =>
                    {
                        MessageBox.Show("情報の取得でエラーが発生しました。");
                    }), null);
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

        private void listView_recinfo_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null)
            {
                if (Settings.Instance.PlayDClick == false)
                {
                    RecInfoItem info = listView_recinfo.SelectedItem as RecInfoItem;
                    RecInfoDescWindow dlg = new RecInfoDescWindow();
                    dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                    RecFileInfo extraRecInfo = new RecFileInfo();
                    if (cmd.SendGetRecInfo(info.RecInfo.ID, ref extraRecInfo) == ErrCode.CMD_SUCCESS)
                    {
                        info.RecInfo.ProgramInfo = extraRecInfo.ProgramInfo;
                        info.RecInfo.ErrInfo = extraRecInfo.ErrInfo;
                    }
                    dlg.SetRecInfo(info.RecInfo);
                    dlg.ShowDialog();
                }
                else
                {
                    RecInfoItem info = listView_recinfo.SelectedItem as RecInfoItem;
                    if (info.RecInfo.RecFilePath.Length > 0)
                    {
                        try
                        {
                            CommonManager.Instance.FilePlay(info.RecInfo.RecFilePath);
                        }
                        catch (Exception ex)
                        {
                            MessageBox.Show(ex.Message);
                        }
                    }
                }
            }
        }

        private void button_play_Click(object sender, RoutedEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null)
            {
                RecInfoItem info = listView_recinfo.SelectedItem as RecInfoItem;
                if (info.RecInfo.RecFilePath.Length > 0)
                {
                    try
                    {
                        CommonManager.Instance.FilePlay(info.RecInfo.RecFilePath);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.Message);
                    }
                }
            }
        }

        private void autoadd_Click(object sender, RoutedEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null)
            {
                SearchWindow dlg = new SearchWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetViewMode(1);

                EpgSearchKeyInfo key = new EpgSearchKeyInfo();

                RecInfoItem item = listView_recinfo.SelectedItem as RecInfoItem;

                key.andKey = item.RecInfo.Title;
                Int64 sidKey = ((Int64)item.RecInfo.OriginalNetworkID) << 32 | ((Int64)item.RecInfo.TransportStreamID) << 16 | ((Int64)item.RecInfo.ServiceID);
                key.serviceList.Add(sidKey);

                dlg.SetSearchDefKey(key);
                dlg.ShowDialog();
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
                RecInfoItem info = listView_recinfo.SelectedItem as RecInfoItem;
                RecInfoDescWindow dlg = new RecInfoDescWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                RecFileInfo extraRecInfo = new RecFileInfo();
                if (cmd.SendGetRecInfo(info.RecInfo.ID, ref extraRecInfo) == ErrCode.CMD_SUCCESS)
                {
                    info.RecInfo.ProgramInfo = extraRecInfo.ProgramInfo;
                    info.RecInfo.ErrInfo = extraRecInfo.ErrInfo;
                }
                dlg.SetRecInfo(info.RecInfo);
                dlg.ShowDialog();
            }
        }

        private void openFolder_Click(object sender, RoutedEventArgs e)
        {
            if (listView_recinfo.SelectedItem != null && CommonManager.Instance.NWMode == false)
            {
                RecInfoItem info = listView_recinfo.SelectedItem as RecInfoItem;
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
    }
}
