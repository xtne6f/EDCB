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
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private List<RecInfoItem> resultList = new List<RecInfoItem>();
        private bool ReloadInfo = true;

        private GridViewSelector gridViewSelector = null;
        private RoutedEventHandler headerSelect_Click = null;

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

                gridViewSelector = new GridViewSelector(gridView_recinfo, Settings.Instance.RecInfoListColumn);
                headerSelect_Click = gridViewSelector.HeaderSelectClick;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void ContextMenu_Header_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            gridViewSelector.ContextMenuOpening(listView_recinfo.ContextMenu);
        }

        public void SaveSize()
        {
            gridViewSelector.SaveSize();
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
        
        GridViewSorter<RecInfoItem> gridViewSorter = new GridViewSorter<RecInfoItem>();

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked = e.OriginalSource as GridViewColumnHeader;
            if (headerClicked != null)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {
                    this.gridViewSorter.SortByMultiHeader(this.resultList, headerClicked);
                    listView_recinfo.Items.Refresh();
                    Settings.Instance.RecInfoColumnHead=this.gridViewSorter.LastHeader;
                    Settings.Instance.RecInfoSortDirection = this.gridViewSorter.LastDirection;
                }
            }
        }

        public bool ReloadInfoData()
        {
            try
            {
                //更新前の選択情報の保存
                var oldItems = new ListViewSelectedKeeper(listView_recinfo, true);

                listView_recinfo.DataContext = null;
                resultList.Clear();

                if (CommonManager.Instance.VUtil.EpgTimerNWNotConnect() == true) return false;

                ErrCode err = CommonManager.Instance.DB.ReloadrecFileInfo();
                if (CommonManager.CmdErrMsgTypical(err, "録画情報の取得", this) == false) return false;

                foreach (RecFileInfo info in CommonManager.Instance.DB.RecFileInfo.Values)
                {
                    resultList.Add(new RecInfoItem(info));
                }

                if (this.gridViewSorter.IsExistSortParams)
                {
                    this.gridViewSorter.SortByMultiHeader(this.resultList);
                }
                else
                {
                    this.gridViewSorter.ResetSortParams();
                    this.gridViewSorter.SortByMultiHeaderWithKey(this.resultList, gridView_recinfo.Columns,
                        Settings.Instance.RecInfoColumnHead, true, Settings.Instance.RecInfoSortDirection);
                }
                listView_recinfo.DataContext = resultList;

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
                ReloadInfo = !ReloadInfoData();
            }
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (ReloadInfo == true && this.IsVisible == true)
            {
                ReloadInfo = !ReloadInfoData();
            }
        }

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (ReloadInfo == true && this.IsVisible == true)
            {
                ReloadInfo = !ReloadInfoData();
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
                mutil.FilePlay(info.RecInfo.RecFilePath);
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
