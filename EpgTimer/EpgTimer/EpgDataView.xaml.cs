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

        /// <summary>
        /// EPGデータの更新通知
        /// </summary>
        public void UpdateEpgData()
        {
            RedrawEpg = true;
            if (IsVisible || Settings.Instance.NgAutoEpgLoadNW == false)
            {
                if (ReDrawEpgData())
                {
                    RedrawEpg = false;
                }
            }
        }

        /// <summary>
        /// 予約情報の更新通知
        /// </summary>
        public void RefreshReserve()
        {
            foreach (TabItem item in tabControl.Items)
            {
                if (item.Content is EpgListMainView)
                {
                    ((EpgListMainView)item.Content).RefreshReserve();
                }
                else if (item.Content is EpgMainView)
                {
                    ((EpgMainView)item.Content).RefreshReserve();
                }
                else if (item.Content is EpgWeekMainView)
                {
                    ((EpgWeekMainView)item.Content).RefreshReserve();
                }
            }
        }

        /// <summary>
        /// 設定の更新通知
        /// </summary>
        public void UpdateSetting()
        {
            {
                //まず表示中のタブのデータをクリア
                foreach (TabItem item in tabControl.Items)
                {
                    if (item.Content is EpgListMainView)
                    {
                        ((EpgListMainView)item.Content).ClearInfo();
                    }
                    else if (item.Content is EpgMainView)
                    {
                        ((EpgMainView)item.Content).ClearInfo();
                    }
                    else if (item.Content is EpgWeekMainView)
                    {
                        ((EpgWeekMainView)item.Content).ClearInfo();
                    }
                }
                //タブの削除
                tabControl.Items.Clear();

                UpdateEpgData();
            }
        }

        /// <summary>
        /// タブ生成
        /// </summary>
        private bool CreateTabItem()
        {
            {
                List<CustomEpgTabInfo> defaultList = null;
                if (Settings.Instance.UseCustomEpgView == false)
                {
                    defaultList = new List<CustomEpgTabInfo>();
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
                            defaultList.Add(setInfo);
                        }
                    }
                }
                foreach (CustomEpgTabInfo info in defaultList ?? Settings.Instance.CustomEpgTabList)
                {
                    var tabItem = new TabItem();
                    tabItem.Header = info.TabName;
                    if (info.ViewMode == 1)
                    {
                        //1週間表示
                        var epgView = new EpgWeekMainView();
                        epgView.SetViewMode(info);
                        epgView.ViewModeChangeRequested += item_ViewModeChangeRequested;
                        tabItem.Content = epgView;
                    }
                    else if (info.ViewMode == 2)
                    {
                        //リスト表示
                        var epgView = new EpgListMainView();
                        epgView.SetViewMode(info);
                        epgView.ViewModeChangeRequested += item_ViewModeChangeRequested;
                        tabItem.Content = epgView;
                    }
                    else
                    {
                        //標準ラテ欄表示
                        var epgView = new EpgMainView();
                        epgView.SetViewMode(info);
                        epgView.ViewModeChangeRequested += item_ViewModeChangeRequested;
                        tabItem.Content = epgView;
                    }
                    tabControl.Items.Add(tabItem);
                }
                if (tabControl.Items.Count > 0)
                {
                    tabControl.SelectedIndex = 0;
                }
            }
            return true;
        }

        /// <summary>
        /// EPGデータの再描画
        /// </summary>
        private bool ReDrawEpgData()
        {
            bool ret = true;
            {
                if (tabControl.Items.Count == 0)
                {
                    //タブの生成
                    ret = CreateTabItem();
                }
                else
                {
                    foreach (TabItem item in tabControl.Items)
                    {
                        if (item.Content is EpgListMainView)
                        {
                            ((EpgListMainView)item.Content).UpdateEpgData();
                        }
                        else if (item.Content is EpgMainView)
                        {
                            ((EpgMainView)item.Content).UpdateEpgData();
                        }
                        else if (item.Content is EpgWeekMainView)
                        {
                            ((EpgWeekMainView)item.Content).UpdateEpgData();
                        }
                    }
                }
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
            ushort onid = 0;
            ushort tsid = 0;
            ushort sid = 0;
            if (BlackoutWindow.selectedReserve != null)
            {
                onid = BlackoutWindow.selectedReserve.OriginalNetworkID;
                tsid = BlackoutWindow.selectedReserve.TransportStreamID;
                sid = BlackoutWindow.selectedReserve.ServiceID;
            }
            else if (BlackoutWindow.selectedEventInfo != null)
            {
                onid = BlackoutWindow.selectedEventInfo.original_network_id;
                tsid = BlackoutWindow.selectedEventInfo.transport_stream_id;
                sid = BlackoutWindow.selectedEventInfo.service_id;
            }
            foreach (TabItem tabItem1 in this.tabControl.Items)
            {
                if ((tabItem1.Content is EpgListMainView && ((EpgListMainView)tabItem1.Content).HasService(onid, tsid, sid)) ||
                    (tabItem1.Content is EpgMainView && ((EpgMainView)tabItem1.Content).HasService(onid, tsid, sid)) ||
                    (tabItem1.Content is EpgWeekMainView && ((EpgWeekMainView)tabItem1.Content).HasService(onid, tsid, sid)))
                {
                    tabItem1.IsSelected = true;
                    break;
                }
            }
        }

        private void item_ViewModeChangeRequested(object sender, CustomEpgTabInfo info)
        {
            foreach (TabItem tabItem in tabControl.Items)
            {
                if (tabItem.Content == sender)
                {
                    //情報クリア
                    if (tabItem.Content is EpgListMainView)
                    {
                        ((EpgListMainView)tabItem.Content).ClearInfo();
                    }
                    else if (tabItem.Content is EpgMainView)
                    {
                        ((EpgMainView)tabItem.Content).ClearInfo();
                    }
                    else if (tabItem.Content is EpgWeekMainView)
                    {
                        ((EpgWeekMainView)tabItem.Content).ClearInfo();
                    }
                    if (info.ViewMode == 1)
                    {
                        //1週間表示
                        var epgView = new EpgWeekMainView();
                        epgView.SetViewMode(info);
                        epgView.ViewModeChangeRequested += item_ViewModeChangeRequested;
                        tabItem.Content = epgView;
                    }
                    else if (info.ViewMode == 2)
                    {
                        //リスト表示
                        var epgView = new EpgListMainView();
                        epgView.SetViewMode(info);
                        epgView.ViewModeChangeRequested += item_ViewModeChangeRequested;
                        tabItem.Content = epgView;
                    }
                    else
                    {
                        //標準ラテ欄表示
                        var epgView = new EpgMainView();
                        epgView.SetViewMode(info);
                        epgView.ViewModeChangeRequested += item_ViewModeChangeRequested;
                        tabItem.Content = epgView;
                    }
                    break;
                }
            }
        }

    }
}
