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
                    if (Settings.Instance.ConfirmDelRecInfo)
                    {
                        bool hasPath = listView_recinfo.SelectedItems.Cast<RecInfoItem>().Any(info => info.RecFilePath.Length > 0);
                        if ((hasPath || Settings.Instance.ConfirmDelRecInfoAlways) &&
                            MessageBox.Show(listView_recinfo.SelectedItems.Count + "項目を削除してよろしいですか?" +
                                            (hasPath ? "\r\n\r\n「録画ファイルも削除する」設定が有効な場合、ファイルも削除されます。" : ""), "確認",
                                            MessageBoxButton.OKCancel, MessageBoxImage.Question, MessageBoxResult.OK) != MessageBoxResult.OK)
                        {
                            return;
                        }
                    }
                    List<UInt32> IDList = new List<uint>();
                    foreach (RecInfoItem info in listView_recinfo.SelectedItems)
                    {
                        IDList.Add(info.RecInfo.ID);
                    }
                    CommonManager.CreateSrvCtrl().SendDelRecInfo(IDList);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
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
        }

        private void listView_recinfo_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter && e.IsRepeat == false)
            {
                //PlayDClickの切り替えはKeyDownが2回発生する(プロセス起動でフォーカス変化するため?)バギーな挙動があるので行わない
                button_recInfo_Click(sender, e);
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
                RecFileInfo info = ((RecInfoItem)listView_recinfo.SelectedItem).RecInfo;
                RecInfoDescWindow dlg = new RecInfoDescWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                RecFileInfo extraRecInfo = new RecFileInfo();
                if (CommonManager.CreateSrvCtrl().SendGetRecInfo(info.ID, ref extraRecInfo) == ErrCode.CMD_SUCCESS)
                {
                    info.ProgramInfo = extraRecInfo.ProgramInfo;
                    info.ErrInfo = extraRecInfo.ErrInfo;
                    if (info.ProgramInfo.Length == 0 && info.EventID != 0xFFFF)
                    {
                        //過去番組情報を探してみる
                        var arcList = new List<EpgServiceEventInfo>();
                        if (CommonManager.CreateSrvCtrl().SendEnumPgArc(new List<long> {
                                0, (long)CommonManager.Create64Key(info.OriginalNetworkID, info.TransportStreamID, info.ServiceID),
                                info.StartTime.ToFileTimeUtc(), info.StartTime.ToFileTimeUtc() + 1 }, ref arcList) == ErrCode.CMD_SUCCESS &&
                            arcList.Count > 0 && arcList[0].eventList.Count > 0)
                        {
                            info.ProgramInfo = CommonManager.Instance.ConvertProgramText(arcList[0].eventList[0], EventInfoTextMode.All);
                        }
                        else
                        {
                            //番組情報を探してみる
                            EpgEventInfo eventInfo = CommonManager.Instance.DB.GetPgInfo(info.OriginalNetworkID, info.TransportStreamID,
                                                                                         info.ServiceID, info.EventID, false);
                            if (eventInfo != null && eventInfo.StartTimeFlag != 0 && eventInfo.start_time == info.StartTime)
                            {
                                info.ProgramInfo = CommonManager.Instance.ConvertProgramText(eventInfo, EventInfoTextMode.All);
                            }
                        }
                    }
                }
                dlg.SetRecInfo(info);
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
                        menuItem.IsChecked = false;
                        foreach (ListColumnInfo info in Settings.Instance.RecInfoListColumn)
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

                    Settings.Instance.RecInfoListColumn.Add(new ListColumnInfo(menuItem.Name, Double.NaN));
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
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void hideButton_Click(object sender, RoutedEventArgs e)
        {
            Settings.Instance.RecInfoHideButton = ((MenuItem)sender).IsChecked;
            stackPanel_button.Visibility = Settings.Instance.RecInfoHideButton ? Visibility.Collapsed : Visibility.Visible;
        }
    }
}
