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


namespace EpgTimer
{
    /// <summary>
    /// ManualAutoAddView.xaml の相互作用ロジック
    /// </summary>
    public partial class ManualAutoAddView : UserControl
    {
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private List<ManualAutoAddDataItem> resultList = new List<ManualAutoAddDataItem>();
        private bool ReloadInfo = true;

        private Dictionary<String, GridViewColumn> columnList = new Dictionary<String, GridViewColumn>();

        public ManualAutoAddView()
        {
            InitializeComponent();
            try
            {
                foreach (GridViewColumn info in gridView_key.Columns)
                {
                    GridViewColumnHeader header = info.Header as GridViewColumnHeader;
                    columnList.Add((string)header.Tag, info);
                }
                gridView_key.Columns.Clear();

                foreach (ListColumnInfo info in Settings.Instance.AutoAddManualColumn)
                {
                    columnList[info.Tag].Width = info.Width;
                    gridView_key.Columns.Add(columnList[info.Tag]);
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
                Settings.Instance.AutoAddManualColumn.Clear();
                foreach (GridViewColumn info in gridView_key.Columns)
                {
                    GridViewColumnHeader header = info.Header as GridViewColumnHeader;

                    Settings.Instance.AutoAddManualColumn.Add(new ListColumnInfo((String)header.Tag, info.Width));
                }

            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// リストの更新通知
        /// </summary>
        public void UpdateInfo()
        {
            if (this.IsVisible == true)
            {
                ReloadInfoData();
                ReloadInfo = false;
            }
            else
            {
                ReloadInfo = true;
            }
        }
        
        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (ReloadInfo == true && this.IsVisible == true)
            {
                ReloadInfoData();
                ReloadInfo = false;
            }
        }

        private bool ReloadInfoData()
        {
            try
            {
                listView_key.DataContext = null;
                resultList.Clear();

                ErrCode err = CommonManager.Instance.DB.ReloadManualAutoAddInfo();
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

                foreach (ManualAutoAddData info in CommonManager.Instance.DB.ManualAutoAddList.Values)
                {
                    ManualAutoAddDataItem item = new ManualAutoAddDataItem(info);
                    resultList.Add(item);
                }

                listView_key.DataContext = resultList;
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

        private void button_add_Click(object sender, RoutedEventArgs e)
        {
            AddManualAutoAddWindow dlg = new AddManualAutoAddWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            dlg.ShowDialog();
        }

        private void button_del_Click(object sender, RoutedEventArgs e)
        {
            if (listView_key.SelectedItems.Count > 0)
            {
                List<UInt32> dataIDList = new List<uint>();
                foreach (ManualAutoAddDataItem info in listView_key.SelectedItems)
                {
                    dataIDList.Add(info.ManualAutoAddInfo.dataID);
                }
                cmd.SendDelManualAdd(dataIDList);
            }
        }

        private void button_change_Click(object sender, RoutedEventArgs e)
        {
            if (listView_key.SelectedItem != null)
            {
                ManualAutoAddDataItem info = listView_key.SelectedItem as ManualAutoAddDataItem;
                AddManualAutoAddWindow dlg = new AddManualAutoAddWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetChangeMode(true);
                dlg.SetDefaultSetting(info.ManualAutoAddInfo);
                dlg.ShowDialog();
            }
        }

        private void listView_key_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            if (listView_key.SelectedItem != null)
            {
                ManualAutoAddDataItem info = listView_key.SelectedItem as ManualAutoAddDataItem;
                AddManualAutoAddWindow dlg = new AddManualAutoAddWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetChangeMode(true);
                dlg.SetDefaultSetting(info.ManualAutoAddInfo);
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

        private void ContextMenu_Header_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            try
            {
                foreach (MenuItem item in listView_key.ContextMenu.Items)
                {
                    item.IsChecked = false;
                    foreach (ListColumnInfo info in Settings.Instance.AutoAddManualColumn)
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

                    Settings.Instance.AutoAddManualColumn.Add(new ListColumnInfo(menuItem.Name, Double.NaN));
                    gridView_key.Columns.Add(columnList[menuItem.Name]);
                }
                else
                {
                    foreach (ListColumnInfo info in Settings.Instance.AutoAddManualColumn)
                    {
                        if (info.Tag.CompareTo(menuItem.Name) == 0)
                        {
                            Settings.Instance.AutoAddManualColumn.Remove(info);
                            gridView_key.Columns.Remove(columnList[menuItem.Name]);
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
