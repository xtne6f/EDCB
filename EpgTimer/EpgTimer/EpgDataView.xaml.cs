using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

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
            if (RedrawEpg == true && this.IsVisible == true)
            {
                RedrawEpg = !ReDrawEpgData();
            }
        }

        private List<EpgDataViewItem> Views
        {
            get { return tabControl.Items.Cast<TabItem>().Select(item => item.Content).OfType<EpgDataViewItem>().ToList(); }
        }

        /// <summary>
        /// EPGデータの更新通知
        /// </summary>
        public void UpdateEpgData()
        {
            RedrawEpg = true;
            if (this.IsVisible == true)
            {
                RedrawEpg = !ReDrawEpgData();
            }
        }

        /// <summary>
        /// 予約情報の更新通知
        /// </summary>
        public void UpdateReserveData()
        {
            try
            {
                this.Views.ForEach(view => view.UpdateReserveData());
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
                //一度全部削除して作り直す。
                this.Views.ForEach(view => view.ClearInfo());
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
                    if (CommonManager.Instance.VUtil.EpgTimerNWNotConnect() == true) return false;

                    ErrCode err = CommonManager.Instance.DB.ReloadEpgData();
                    if (CommonManager.CmdErrMsgTypical(err, "EPGデータの取得", this) == false)
                    {
                        return false;
                    }

                    CommonManager.Instance.DB.ReloadReserveInfo();

                    var findService = new List<bool>();//その他、地デジ、BS、CS
                    var setInfo = new List<CustomEpgTabInfo>();
                    for (int i = 0; i < 4; i++)
                    {
                        findService.Add(false);
                        var info = new CustomEpgTabInfo();
                        info.ViewMode = 0;
                        info.NeedTimeOnlyBasic = false;
                        setInfo.Add(info);
                    }

                    setInfo[0].TabName = "その他";
                    setInfo[1].TabName = "地デジ";
                    setInfo[2].TabName = "BS";
                    setInfo[3].TabName = "CS";

                    //デフォルト表示
                    foreach (EpgServiceEventInfo info in CommonManager.Instance.DB.ServiceEventList.Values)
                    {
                        int i = 0;//その他
                        if (info.serviceInfo.ONID == 0x0004)
                        {
                            i = 2;//BS
                        }
                        else if (info.serviceInfo.ONID == 0x0006 || info.serviceInfo.ONID == 0x0007)
                        {
                            i = 3;//CS
                        }
                        else if (0x7880 <= info.serviceInfo.ONID && info.serviceInfo.ONID <= 0x7FE8)
                        {
                            i = 1;//地デジ
                        }

                        UInt64 id = info.serviceInfo.Create64Key();
                        setInfo[i].ViewServiceList.Add(id);
                        findService[i] = true;
                    }

                    for (int i = 0; i < 4; i++)
                    {
                        if(findService[i] ==true)
                        {
                            SetTabs(setInfo[i]);
                        }
                    }
                }
                else
                {
                    //カスタム表示
                    Settings.Instance.CustomEpgTabList.ForEach(info => SetTabs(info));
                }
                if (tabControl.Items.Count > 0)
                {
                    tabControl.SelectedIndex = 0;
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

        private void SetTabs(CustomEpgTabInfo info)
        {
            EpgDataViewItem epgView = new EpgDataViewItem();
            epgView.SetViewMode(info);
            epgView.ViewSettingClick += new ViewSettingClickHandler(epgView_ViewSettingClick);

            TabItem tabItem = new TabItem();
            tabItem.Header = info.TabName;
            tabItem.Content = epgView;
            tabControl.Items.Add(tabItem);
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
                    if (sender is EpgDataViewItem)
                    {
                        EpgDataViewItem item = sender as EpgDataViewItem;
                        if (param == null)
                        {
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
                //タブが無ければ生成、あれば更新
                if (tabControl.Items.Count == 0)
                {
                    return CreateTabItem();
                }
                else
                {
                    this.Views.ForEach(view => view.UpdateEpgData());
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

        //メニューの更新
        public void RefreshMenu()
        {
            try
            {
                this.Views.ForEach(view => view.RefreshMenu());
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            try
            {
                if (RedrawEpg == true && this.IsVisible == true)
                {
                    RedrawEpg = !ReDrawEpgData();
                }
                if (this.IsVisible)
                {
                    this.searchJumpTargetProgram();//EPG更新後に探す
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
            UInt64 serviceKey_Target1 = BlackoutWindow.Create64Key();
            if (serviceKey_Target1 == 0) return;

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
