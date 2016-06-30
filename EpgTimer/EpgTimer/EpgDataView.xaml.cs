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
                    if (err == ErrCode.CMD_ERR_CONNECT)
                    {
                        this.Dispatcher.BeginInvoke(new Action(() =>
                        {
                            MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                        }), null);
                        return false;
                    }
                    if (err == ErrCode.CMD_ERR_BUSY)
                    {
                        this.Dispatcher.BeginInvoke(new Action(() =>
                        {
                            MessageBox.Show("EPGデータの読み込みを行える状態ではありません。\r\n（EPGデータ読み込み中。など）");
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
                            MessageBox.Show("EPGデータの取得でエラーが発生しました。EPGデータが読み込まれていない可能性があります。");
                        }), null);
                        return false;
                    }
                    CommonManager.Instance.DB.ReloadReserveInfo();

                    bool findTere = false;
                    bool findBS = false;
                    bool findCS = false;
                    bool findOther = false;

                    CustomEpgTabInfo setInfoBS = new CustomEpgTabInfo();
                    setInfoBS.ViewMode = 0;
                    setInfoBS.TabName = "BS";
                    setInfoBS.NeedTimeOnlyBasic = false;
                    CustomEpgTabInfo setInfoCS = new CustomEpgTabInfo();
                    setInfoCS.ViewMode = 0;
                    setInfoCS.TabName = "CS";
                    setInfoCS.NeedTimeOnlyBasic = false;
                    CustomEpgTabInfo setInfoTere = new CustomEpgTabInfo();
                    setInfoTere.ViewMode = 0;
                    setInfoTere.TabName = "地デジ";
                    setInfoTere.NeedTimeOnlyBasic = false;
                    CustomEpgTabInfo setInfoOther = new CustomEpgTabInfo();
                    setInfoOther.ViewMode = 0;
                    setInfoOther.TabName = "その他";
                    setInfoOther.NeedTimeOnlyBasic = false;


                    //デフォルト表示
                    foreach (EpgServiceEventInfo info in CommonManager.Instance.DB.ServiceEventList.Values)
                    {
                        if (info.serviceInfo.ONID == 0x0004)
                        {
                            findBS = true;
                            UInt64 id = CommonManager.Create64Key(info.serviceInfo.ONID, info.serviceInfo.TSID, info.serviceInfo.SID);
                            setInfoBS.ViewServiceList.Add(id);
                        }
                        else if (info.serviceInfo.ONID == 0x0006 || info.serviceInfo.ONID == 0x0007)
                        {
                            findCS = true;
                            UInt64 id = CommonManager.Create64Key(info.serviceInfo.ONID, info.serviceInfo.TSID, info.serviceInfo.SID);
                            setInfoCS.ViewServiceList.Add(id);
                        }
                        else if (0x7880 <= info.serviceInfo.ONID && info.serviceInfo.ONID <= 0x7FE8)
                        {
                            findTere = true;
                            UInt64 id = CommonManager.Create64Key(info.serviceInfo.ONID, info.serviceInfo.TSID, info.serviceInfo.SID);
                            setInfoTere.ViewServiceList.Add(id);
                        }
                        else
                        {
                            findOther = true;
                            UInt64 id = CommonManager.Create64Key(info.serviceInfo.ONID, info.serviceInfo.TSID, info.serviceInfo.SID);
                            setInfoOther.ViewServiceList.Add(id);
                        }
                    }
                    if (findBS == true)
                    {
                        EpgDataViewItem epgView = new EpgDataViewItem();
                        epgView.SetViewMode(setInfoBS);
                        epgView.ViewSettingClick += new ViewSettingClickHandler(epgView_ViewSettingClick);


                        TabItem tabItem = new TabItem();
                        tabItem.Header = setInfoBS.TabName;
                        tabItem.Content = epgView;
                        tabControl.Items.Add(tabItem);
                    }
                    if (findCS == true)
                    {
                        EpgDataViewItem epgView = new EpgDataViewItem();
                        epgView.SetViewMode(setInfoCS);
                        epgView.ViewSettingClick += new ViewSettingClickHandler(epgView_ViewSettingClick);


                        TabItem tabItem = new TabItem();
                        tabItem.Header = setInfoCS.TabName;
                        tabItem.Content = epgView;
                        tabControl.Items.Add(tabItem);
                    }
                    if (findTere == true)
                    {
                        EpgDataViewItem epgView = new EpgDataViewItem();
                        epgView.SetViewMode(setInfoTere);
                        epgView.ViewSettingClick += new ViewSettingClickHandler(epgView_ViewSettingClick);


                        TabItem tabItem = new TabItem();
                        tabItem.Header = setInfoTere.TabName;
                        tabItem.Content = epgView;
                        tabControl.Items.Add(tabItem);

                    }
                    if (findOther == true)
                    {
                        EpgDataViewItem epgView = new EpgDataViewItem();
                        epgView.SetViewMode(setInfoOther);
                        epgView.ViewSettingClick += new ViewSettingClickHandler(epgView_ViewSettingClick);


                        TabItem tabItem = new TabItem();
                        tabItem.Header = setInfoOther.TabName;
                        tabItem.Content = epgView;
                        tabControl.Items.Add(tabItem);
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
