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
        private ListViewHorizontalMouseScroller horizontalScroller = new ListViewHorizontalMouseScroller();
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
            if (Settings.Instance.AutoAddManualHideButton)
            {
                stackPanel_button.Visibility = Visibility.Collapsed;
            }
            listView_key.AlternationCount = Settings.Instance.ResAlternationCount;
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
            var win = new AddManualAutoAddWindow();
            ((MainWindow)Application.Current.MainWindow).SwapOwnedReserveWindow(win);
            win.Show();
        }

        private void button_del_Click(object sender, RoutedEventArgs e)
        {
            if (listView_key.SelectedItems.Count > 0)
            {
                List<uint> dataIDList = new List<uint>();
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
                var win = new AddManualAutoAddWindow();
                ((MainWindow)Application.Current.MainWindow).SwapOwnedReserveWindow(win);
                win.SetChangeModeData(info.ManualAutoAddInfo);
                win.Show();
            }
        }

        private void listView_key_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            button_change_Click(sender, e);
            e.Handled = true;
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
            foreach (object item in listView_key.ContextMenu.Items)
            {
                MenuItem menuItem = item as MenuItem;
                if (menuItem != null && menuItem.IsCheckable)
                {
                    if (menuItem.Name == "HideButton")
                    {
                        menuItem.IsChecked = Settings.Instance.AutoAddManualHideButton;
                    }
                    else
                    {
                        menuItem.IsChecked = Settings.Instance.AutoAddManualColumn.Any(info => info.Tag == menuItem.Name);
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

                    Settings.Instance.AutoAddManualColumn.Add(new ListColumnInfo(menuItem.Name, double.NaN));
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
                MessageBox.Show(ex.ToString());
            }
        }

        private void hideButton_Click(object sender, RoutedEventArgs e)
        {
            Settings.Instance.AutoAddManualHideButton = ((MenuItem)sender).IsChecked;
            stackPanel_button.Visibility = Settings.Instance.AutoAddManualHideButton ? Visibility.Collapsed : Visibility.Visible;
        }

        private void listView_key_KeyDown(object sender, KeyEventArgs e)
        {
            switch (e.Key)
            {
                case Key.Enter:
                    button_change_Click(sender, e);
                    e.Handled = true;
                    break;
                case Key.Delete:
                    if (listView_key.SelectedItems.Count > 0 &&
                        MessageBox.Show(listView_key.SelectedItems.Count + "項目を削除してよろしいですか?", "確認",
                                        MessageBoxButton.OKCancel, MessageBoxImage.Question, MessageBoxResult.OK) == MessageBoxResult.OK)
                    {
                        button_del_Click(sender, e);
                    }
                    e.Handled = true;
                    break;
            }
        }

        private void listView_key_MouseEnter(object sender, MouseEventArgs e)
        {
            horizontalScroller.OnMouseEnter(listView_key, Settings.Instance.EpgSettingList[0].MouseHorizontalScrollAuto,
                                            Settings.Instance.EpgSettingList[0].HorizontalScrollSize);
        }

        private void listView_key_MouseLeave(object sender, MouseEventArgs e)
        {
            horizontalScroller.OnMouseLeave();
        }
    }
}
