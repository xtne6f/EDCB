using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

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
                tabControl.Items.Clear();
                UpdateEpgData();
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
                List<CustomEpgTabInfo> setInfo = null;
                if (Settings.Instance.UseCustomEpgView == false)
                {
                    setInfo = CommonManager.Instance.CreateDefaultTabInfo();
                }
                else
                {
                    setInfo = Settings.Instance.CustomEpgTabList;
                }

                setInfo.ForEach(info => 
                {
                    EpgDataViewItem epgView = new EpgDataViewItem();
                    epgView.SetViewMode(info);

                    TabItem tabItem = new TabItem();
                    tabItem.Header = info.TabName;
                    tabItem.Content = epgView;
                    tabControl.Items.Add(tabItem);
                });

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
                foreach (UInt64 serviceKey_OnTab1 in epgView1.GetViewMode().ViewServiceList)
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
