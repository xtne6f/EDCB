using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    /// <summary>
    /// ManualAutoAddView.xaml の相互作用ロジック
    /// </summary>
    public partial class ManualAutoAddView : UserControl
    {
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private List<ManualAutoAddDataItem> resultList = new List<ManualAutoAddDataItem>();
        private bool ReloadInfo = true;

        private GridViewSelector gridViewSelector = null;
        private Action<object, RoutedEventArgs> headerSelect_Click = null;

        public ManualAutoAddView()
        {
            InitializeComponent();
            try
            {
                if (Settings.Instance.NoStyle == 1)
                {
                    button_add.Style = null;
                    button_del.Style = null;
                    button_del2.Style = null;
                    button_change.Style = null;
                }

                gridViewSelector = new GridViewSelector(gridView_key, Settings.Instance.AutoAddManualColumn);
                headerSelect_Click = gridViewSelector.HeaderSelectClick;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void ContextMenu_Header_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            gridViewSelector.ContextMenuOpening(listView_key.ContextMenu);
        }

        public void SaveSize()
        {
            gridViewSelector.SaveSize();
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
                //更新前の選択情報の保存
                var oldItems = new ListViewSelectedKeeper<ManualAutoAddDataItem>(listView_key, true);

                listView_key.DataContext = null;
                resultList.Clear();

                if (CommonManager.Instance.NWMode == true)
                {
                    if (CommonManager.Instance.NW.IsConnected == false)
                    {
                        return false;
                    }
                }
                ErrCode err = CommonManager.Instance.DB.ReloadManualAutoAddInfo();
                if (CommonManager.CmdErrMsgTypical(err, "情報の取得", this) == false) return false;

                foreach (ManualAutoAddData info in CommonManager.Instance.DB.ManualAutoAddList.Values)
                {
                    ManualAutoAddDataItem item = new ManualAutoAddDataItem(info);
                    resultList.Add(item);
                }

                listView_key.DataContext = resultList;

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

        private void button_add_Click(object sender, RoutedEventArgs e)
        {
            AddManualAutoAddWindow dlg = new AddManualAutoAddWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            dlg.ShowDialog();
        }

        private void button_del_Click(object sender, RoutedEventArgs e)
        {
            if (listView_key.SelectedItems.Count == 0) return;

            var dataIDList = GetSelectedItemsList().Select(info => info.ManualAutoAddInfo.dataID).ToList();
            cmd.SendDelManualAdd(dataIDList);
        }

        private void button_del2_Click(object sender, RoutedEventArgs e)
        {
            if (listView_key.SelectedItems.Count == 0) return;

            string text1 = "予約項目ごと削除してよろしいですか?　[削除アイテム数: " + listView_key.SelectedItems.Count + "]\r\n"
                            + "(サービス、時間の一致した予約が削除されます)\r\n\r\n";
            GetSelectedItemsList().ForEach(info => text1 += " ・ " + info.Title + "\r\n");

            string caption1 = "[予約ごと削除]の確認";
            if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel,
                MessageBoxImage.Exclamation, MessageBoxResult.OK) != MessageBoxResult.OK)
            {
                return;
            }

            //EpgTimerSrvでの自動予約登録の実行タイミングに左右されず確実に予約を削除するため、
            //先に自動予約登録項目を削除する。

            //自動予約登録項目のリストを保持
            List<ManualAutoAddDataItem> autoaddlist = GetSelectedItemsList();

            button_del_Click(sender, e);

            try
            {
                //配下の予約の削除
                var dellist = new List<ReserveData>();
                autoaddlist.ForEach(info => dellist.AddRange(info.ManualAutoAddInfo.GetReserveList()));
                dellist = dellist.Distinct().ToList();

                mutil.ReserveDelete(dellist);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }

        }

        private List<ManualAutoAddDataItem> GetSelectedItemsList()
        {
            return listView_key.SelectedItems.Cast<ManualAutoAddDataItem>().ToList();
        }

        private void button_change_Click(object sender, RoutedEventArgs e)
        {
            if (listView_key.SelectedItem != null)
            {
                ManualAutoAddDataItem info = SelectSingleItem();
                AddManualAutoAddWindow dlg = new AddManualAutoAddWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetChangeMode(true);
                dlg.SetDefaultSetting(info.ManualAutoAddInfo);
                dlg.ShowDialog();
            }
        }

        private ManualAutoAddDataItem SelectSingleItem()
        {
            return mutil.SelectSingleItem<ManualAutoAddDataItem>(listView_key);
        }

        private void listView_key_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            this.button_change.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
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

    }
}
