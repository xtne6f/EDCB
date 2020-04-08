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
        private bool ReloadInfo = true;

        private Dictionary<string, GridViewColumn> columnList;

        public ManualAutoAddView()
        {
            InitializeComponent();
            columnList = gridView_key.Columns.ToDictionary(info => (string)((GridViewColumnHeader)info.Header).Tag);
            gridView_key.Columns.Clear();
            foreach (ListColumnInfo info in Settings.Instance.AutoAddManualColumn)
            {
                if (columnList.ContainsKey(info.Tag))
                {
                    columnList[info.Tag].Width = info.Width;
                    gridView_key.Columns.Add(columnList[info.Tag]);
                }
            }
        }

        public void SaveSize()
        {
            Settings.Instance.AutoAddManualColumn.Clear();
            Settings.Instance.AutoAddManualColumn.AddRange(
                gridView_key.Columns.Select(info => new ListColumnInfo((string)((GridViewColumnHeader)info.Header).Tag, info.Width)));
        }

        /// <summary>
        /// リストの更新通知
        /// </summary>
        public void UpdateInfo()
        {
            ReloadInfo = true;
            if (IsVisible && ReloadInfoData())
            {
                ReloadInfo = false;
            }
        }

        private bool ReloadInfoData()
        {
            if (CommonManager.Instance.NWMode && CommonManager.Instance.NWConnectedIP == null)
            {
                listView_key.ItemsSource = null;
                return false;
            }
            ErrCode err = CommonManager.Instance.DB.ReloadManualAutoAddInfo();
            if (err != ErrCode.CMD_SUCCESS)
            {
                Dispatcher.BeginInvoke(new Action(() => MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "情報の取得でエラーが発生しました。")));
                listView_key.ItemsSource = null;
                return false;
            }
            listView_key.ItemsSource = CommonManager.Instance.DB.ManualAutoAddList.Values.Select(info => new ManualAutoAddDataItem(info)).ToList();
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
                CommonManager.CreateSrvCtrl().SendDelManualAdd(dataIDList);
            }
        }

        private void button_change_Click(object sender, RoutedEventArgs e)
        {
            if (listView_key.SelectedItem != null)
            {
                ManualAutoAddDataItem info = listView_key.SelectedItem as ManualAutoAddDataItem;
                AddManualAutoAddWindow dlg = new AddManualAutoAddWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetChangeModeData(info.ManualAutoAddInfo);
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
                dlg.SetChangeModeData(info.ManualAutoAddInfo);
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
                    item.IsChecked = Settings.Instance.AutoAddManualColumn.Any(info => info.Tag == item.Name);
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
                        if (info.Tag == menuItem.Name)
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
