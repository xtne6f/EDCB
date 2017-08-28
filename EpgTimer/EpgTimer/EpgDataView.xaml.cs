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
    /// EpgView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgDataView : UserControl
    {
        private bool RedrawEpg = true;

        public EpgDataView()
        {
            InitializeComponent();
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            try
            {
                if (RedrawEpg == true && this.IsVisible == true)
                {
                    if (ReDrawEpgData() == true)
                    {
                        RedrawEpg = false;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// EPGデータの更新通知
        /// </summary>
        public void UpdateEpgData()
        {
            try
            {
                if (this.IsVisible == true || CommonManager.Instance.NWMode == false)
                {
                    if (ReDrawEpgData() == true)
                    {
                        RedrawEpg = false;
                    }
                }
                else
                {
                    RedrawEpg = true;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 予約情報の更新通知
        /// </summary>
        public void UpdateReserveData()
        {
            try
            {
                foreach (TabItem item in tabControl.Items)
                {
                    if (item.Content.GetType() == typeof(EpgDataViewItem))
                    {
                        EpgDataViewItem view = item.Content as EpgDataViewItem;
                        view.UpdateReserveData();
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 設定の更新通知
        /// </summary>
        public void UpdateSetting()
        {
            try
            {
                //まず表示中のタブのデータをクリア
                foreach (TabItem item in tabControl.Items)
                {
                    if (item.Content.GetType() == typeof(EpgDataViewItem))
                    {
                        EpgDataViewItem view = item.Content as EpgDataViewItem;
                        view.ClearInfo();
                    }
                }
                //タブの削除
                tabControl.Items.Clear();

                ReDrawEpgData();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// タブ生成
        /// </summary>
        private bool CreateTabItem()
        {
            try
            {
                if (Settings.Instance.UseCustomEpgView == false)
                {
                    if (CommonManager.Instance.NWMode == true)
                    {
                        if (CommonManager.Instance.NW.IsConnected == false)
                        {
                            return false;
                        }
                    }
                    ErrCode err = CommonManager.Instance.DB.ReloadEpgData();
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        this.Dispatcher.BeginInvoke(new Action(() =>
                        {
                            MessageBox.Show(CommonManager.GetErrCodeText(err) ??
                                (err == ErrCode.CMD_ERR_BUSY ? "EPGデータの読み込みを行える状態ではありません。\r\n（EPGデータ読み込み中。など）" :
                                                               "EPGデータの取得でエラーが発生しました。EPGデータが読み込まれていない可能性があります。"));
                        }), null);
                        return false;
                    }
                    CommonManager.Instance.DB.ReloadReserveInfo();

                    //デフォルト表示
                    for (int i = 0; i < 4; i++)
                    {
                        CustomEpgTabInfo setInfo = null;
                        bool ignoreEpgCap = Settings.Instance.ShowEpgCapServiceOnly == false;
                        //リモコンキー優先のID順ソート
                        foreach (EpgServiceAllEventInfo info in CommonManager.Instance.DB.ServiceEventList.Values.Where(item => {
                            ulong key = CommonManager.Create64Key(item.serviceInfo.ONID, item.serviceInfo.TSID, item.serviceInfo.SID);
                            return ignoreEpgCap || ChSet5.Instance.ChList.ContainsKey(key) && ChSet5.Instance.ChList[key].EpgCapFlag; }).OrderBy(item => (
                            (ulong)(ChSet5.IsDttv(item.serviceInfo.ONID) ? (item.serviceInfo.remote_control_key_id + 255) % 256 : 0) << 48 |
                            CommonManager.Create64Key(item.serviceInfo.ONID, item.serviceInfo.TSID, item.serviceInfo.SID))))
                        {
                            string tabName = null;
                            if (i == 0 && ChSet5.IsDttv(info.serviceInfo.ONID))
                            {
                                tabName = "地デジ";
                            }
                            else if (i == 1 && ChSet5.IsBS(info.serviceInfo.ONID))
                            {
                                tabName = "BS";
                            }
                            else if (i == 2 && ChSet5.IsCS(info.serviceInfo.ONID))
                            {
                                tabName = "CS";
                            }
                            else if (i == 3 && ChSet5.IsOther(info.serviceInfo.ONID))
                            {
                                tabName = "その他";
                            }
                            if (tabName != null)
                            {
                                if (setInfo == null)
                                {
                                    setInfo = new CustomEpgTabInfo();
                                    setInfo.TabName = tabName;
                                }
                                setInfo.ViewServiceList.Add(CommonManager.Create64Key(info.serviceInfo.ONID, info.serviceInfo.TSID, info.serviceInfo.SID));
                            }
                        }
                        if (setInfo != null)
                        {
                            EpgDataViewItem epgView = new EpgDataViewItem();
                            epgView.SetViewMode(setInfo);
                            epgView.ViewSettingClick += new ViewSettingClickHandler(epgView_ViewSettingClick);
                            TabItem tabItem = new TabItem();
                            tabItem.Header = setInfo.TabName;
                            tabItem.Content = epgView;
                            tabControl.Items.Add(tabItem);
                        }
                    }
                    if (tabControl.Items.Count > 0)
                    {
                        tabControl.SelectedIndex = 0;
                    }
                }
                else
                {
                    //カスタム表示
                    foreach (CustomEpgTabInfo info in Settings.Instance.CustomEpgTabList)
                    {
                        EpgDataViewItem epgView = new EpgDataViewItem();
                        epgView.SetViewMode(info);
                        epgView.ViewSettingClick += new ViewSettingClickHandler(epgView_ViewSettingClick);

                        TabItem tabItem = new TabItem();
                        tabItem.Header = info.TabName;
                        tabItem.Content = epgView;
                        tabControl.Items.Add(tabItem);
                    }
                    if (tabControl.Items.Count > 0)
                    {
                        tabControl.SelectedIndex = 0;
                    }
                }
            }
            catch (Exception ex)
            {
                this.Dispatcher.BeginInvoke(new Action(() =>
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }), null);

            }
            return true;
        }

        void epgView_ViewSettingClick(object sender, object param)
        {
            try
            {
                if (Settings.Instance.UseCustomEpgView == false)
                {
                    MessageBox.Show("デフォルト表示では設定を変更することはできません。");
                }
                else
                {
                    if (sender.GetType() == typeof(EpgDataViewItem))
                    {
                        if (param == null)
                        {
                            EpgDataViewItem item = sender as EpgDataViewItem;
                            CustomEpgTabInfo setInfo = new CustomEpgTabInfo();
                            item.GetViewMode(ref setInfo);

                            EpgDataViewSettingWindow dlg = new EpgDataViewSettingWindow();
                            PresentationSource topWindow = PresentationSource.FromVisual(this);
                            if (topWindow != null)
                            {
                                dlg.Owner = (Window)topWindow.RootVisual;
                            }
                            dlg.SetDefSetting(setInfo);
                            if (dlg.ShowDialog() == true)
                            {
                                dlg.GetSetting(ref setInfo);
                                item.SetViewMode(setInfo);
                            }
                        }
                        else
                        {
                            EpgDataViewItem item = sender as EpgDataViewItem;
                            CustomEpgTabInfo setInfo = param as CustomEpgTabInfo;
                            item.SetViewMode(setInfo);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// EPGデータの再描画
        /// </summary>
        private bool ReDrawEpgData()
        {
            bool ret = true;
            try
            {
                if (tabControl.Items.Count == 0)
                {
                    //タブの生成
                    ret = CreateTabItem();
                }
                else
                {
                    //まず表示中のタブのデータをクリア
                    foreach (TabItem item in tabControl.Items)
                    {
                        if (item.Content.GetType() == typeof(EpgDataViewItem))
                        {
                            EpgDataViewItem view = item.Content as EpgDataViewItem;
                            view.UpdateEpgData();
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                this.Dispatcher.BeginInvoke(new Action(() =>
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }), null);
            }
            return ret;
        }

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            try
            {
                if (RedrawEpg == true && this.IsVisible == true)
                {
                    if (ReDrawEpgData() == true)
                    {
                        RedrawEpg = false;
                    }
                }
                if (this.IsVisible)
                {
                    this.searchJumpTargetProgram();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 予約一覧からのジャンプ先を番組表タブから探す
        /// </summary>
        void searchJumpTargetProgram()
        {
            UInt64 serviceKey_Target1 = 0;
            if (BlackoutWindow.selectedReserveItem != null)
            {
                ReserveData reserveData1 = BlackoutWindow.selectedReserveItem.ReserveInfo;
                serviceKey_Target1 = CommonManager.Create64Key(reserveData1.OriginalNetworkID, reserveData1.TransportStreamID, reserveData1.ServiceID);
            }
            else if (BlackoutWindow.selectedSearchItem != null)
            {
                EpgEventInfo eventInfo1 = BlackoutWindow.selectedSearchItem.EventInfo;
                serviceKey_Target1 = CommonManager.Create64Key(eventInfo1.original_network_id, eventInfo1.transport_stream_id, eventInfo1.service_id);
            }
            foreach (TabItem tabItem1 in this.tabControl.Items)
            {
                EpgDataViewItem epgView1 = tabItem1.Content as EpgDataViewItem;
                foreach (UInt64 serviceKey_OnTab1 in epgView1.ViewInfo.ViewServiceList)
                {
                    if (serviceKey_Target1 == serviceKey_OnTab1)
                    {
                        tabItem1.IsSelected = true;
                        return;
                    }
                }
            }
        }

    }
}
