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
        private object jumpTarget;

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
                    if (CommonManager.Instance.NWMode && CommonManager.Instance.NWConnectedIP == null)
                    {
                        return false;
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
                        var epgView = new EpgWeekMainView(info);
                        epgView.ViewModeChangeRequested += item_ViewModeChangeRequested;
                        tabItem.Content = epgView;
                    }
                    else if (info.ViewMode == 2)
                    {
                        //リスト表示
                        var epgView = new EpgListMainView(info);
                        epgView.ViewModeChangeRequested += item_ViewModeChangeRequested;
                        tabItem.Content = epgView;
                    }
                    else
                    {
                        //標準ラテ欄表示
                        var epgView = new EpgMainView(info);
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
                if (ret)
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
            if (IsVisible)
            {
                if (RedrawEpg)
                {
                    if (ReDrawEpgData() == true)
                    {
                        RedrawEpg = false;
                    }
                }
                if (jumpTarget != null)
                {
                    SearchJumpTargetProgram(jumpTarget);
                }
            }
        }

        /// <summary>
        /// 予約または番組へジャンプする
        /// </summary>
        public void SearchJumpTargetProgram(object target)
        {
            jumpTarget = null;
            if (IsVisible == false)
            {
                //Visibleになるまですこし待つ
                jumpTarget = target;
                Dispatcher.BeginInvoke(System.Windows.Threading.DispatcherPriority.Render, new Action(() => jumpTarget = null));
                return;
            }
            ushort onid = 0;
            ushort tsid = 0;
            ushort sid = 0;
            //TODO: 難易度上がるがEventIDも見るべき
            if (target is ReserveData)
            {
                onid = ((ReserveData)target).OriginalNetworkID;
                tsid = ((ReserveData)target).TransportStreamID;
                sid = ((ReserveData)target).ServiceID;
            }
            else if (target is EpgEventInfo)
            {
                onid = ((EpgEventInfo)target).original_network_id;
                tsid = ((EpgEventInfo)target).transport_stream_id;
                sid = ((EpgEventInfo)target).service_id;
            }
            foreach (TabItem tabItem1 in this.tabControl.Items)
            {
                if (tabItem1.Content is EpgListMainView)
                {
                    var epgView = (EpgListMainView)tabItem1.Content;
                    if (epgView.HasService(onid, tsid, sid))
                    {
                        tabItem1.IsSelected = true;
                        break;
                    }
                }
                else if (tabItem1.Content is EpgMainView)
                {
                    var epgView = (EpgMainView)tabItem1.Content;
                    if (epgView.HasService(onid, tsid, sid))
                    {
                        tabItem1.IsSelected = true;
                        epgView.ScrollTo(target);
                        break;
                    }
                }
                else if (tabItem1.Content is EpgWeekMainView)
                {
                    var epgView = (EpgWeekMainView)tabItem1.Content;
                    if (epgView.HasService(onid, tsid, sid))
                    {
                        tabItem1.IsSelected = true;
                        epgView.ScrollTo(target);
                        break;
                    }
                }
            }
        }

        private void item_ViewModeChangeRequested(object sender, CustomEpgTabInfo info, object scrollToTarget)
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
                        var epgView = new EpgWeekMainView(info);
                        epgView.ViewModeChangeRequested += item_ViewModeChangeRequested;
                        tabItem.Content = epgView;
                        epgView.ScrollTo(scrollToTarget);
                    }
                    else if (info.ViewMode == 2)
                    {
                        //リスト表示
                        var epgView = new EpgListMainView(info);
                        epgView.ViewModeChangeRequested += item_ViewModeChangeRequested;
                        tabItem.Content = epgView;
                    }
                    else
                    {
                        //標準ラテ欄表示
                        var epgView = new EpgMainView(info);
                        epgView.ViewModeChangeRequested += item_ViewModeChangeRequested;
                        tabItem.Content = epgView;
                        epgView.ScrollTo(scrollToTarget);
                    }
                    break;
                }
            }
        }

    }
}
