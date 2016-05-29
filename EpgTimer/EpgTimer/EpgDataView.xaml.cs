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
        private bool ReloadInfo = true;

        public EpgDataView()
        {
            InitializeComponent();
        }

        private List<EpgDataViewItem> Views
        {
            get { return tabControl.Items.Cast<TabItem>().Select(item => item.Content).OfType<EpgDataViewItem>().ToList(); }
        }

        public void SaveViewData()
        {
            //存在しないときは、本当に無いか、破棄されて保存済み
            this.Views.ForEach(view => view.SaveViewData());
        }

        /// <summary>
        /// EPGデータの更新通知
        /// </summary>
        public void UpdateInfo(bool reload = true)
        {
            ReloadInfo |= reload;
            if (ReloadInfo == true && this.IsVisible == true)
            {
                ReloadInfo = !ReloadInfoData();
            }
        }

        /// <summary>
        /// 予約情報の更新通知
        /// </summary>
        public void UpdateReserveInfo(bool reload = true)
        {
            this.Views.ForEach(view => view.UpdateReserveInfo(reload));
        }

        /// <summary>
        /// 設定の更新通知
        /// </summary>
        CustomEpgTabInfo oldInfo = null;
        object oldState = null;
        int? oldID = null;
        public void UpdateSetting()
        {
            try
            {
                SaveViewData();

                //表示していた番組表の情報を保存
                var item = tabControl.SelectedItem as TabItem;
                if (item != null)
                {
                    var view = item.Content as EpgDataViewItem;
                    oldInfo = view.GetViewMode();
                    oldState = view.GetViewState();
                    oldID = oldInfo.ID;
                }

                //一度全部削除して作り直す。
                //保存情報は次回のタブ作成時に復元する。
                tabControl.Items.Clear();
                UpdateInfo();
            }
            catch (Exception ex) { CommonUtil.DispatcherMsgBoxShow(ex.Message + "\r\n" + ex.StackTrace); }

            //UpdateInfo()は非表示の時走らない。
            //データはここでクリアしてしまうので、現に表示されているもの以外は表示状態はリセットされる。
            //ただし、番組表(oldID)の選択そのものは保持する。
            oldInfo = null;
            oldState = null;
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

                int selectIndex = 0;
                setInfo.ForEach(info => 
                {
                    var epgView = new EpgDataViewItem();
                    var tabItem = new TabItem();
                    tabItem.Header = info.TabName;
                    tabItem.Content = epgView;
                    int index = tabControl.Items.Add(tabItem);

                    //とりあえず同じIDを探して表示してみる(中身は別物になってるかもしれないが、とりあえず表示を試みる)。
                    //標準・カスタム切り替えの際は、標準番組表が負のIDを与えられているので、このコードは走らない。
                    object state = null;
                    if (oldID == info.ID)
                    {
                        selectIndex = index;
                        if (oldInfo != null)
                        {
                            info = info.Clone();
                            info.ViewMode = oldInfo.ViewMode;
                            state = oldState;
                        }
                    }
                    epgView.SetViewMode(info, state);
                });
                tabControl.SelectedIndex = selectIndex;
                oldID = null;
            }
            catch (Exception ex) { CommonUtil.DispatcherMsgBoxShow(ex.Message + "\r\n" + ex.StackTrace); }
            return true;
        }

        /// <summary>
        /// EPGデータの再描画
        /// </summary>
        private bool ReloadInfoData(bool reload = true)
        {
            //タブが無ければ生成、あれば更新
            if (tabControl.Items.Count == 0)
            {
                return CreateTabItem();
            }
            else
            {
                this.Views.ForEach(view => view.UpdateInfo(reload));
            }
            return true;
        }

        //メニューの更新
        public void RefreshMenu()
        {
            this.Views.ForEach(view => view.RefreshMenu());
        }

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            try
            {
                UpdateInfo(false);
                if (this.IsVisible)
                {
                    this.searchJumpTargetProgram();//EPG更新後に探す
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
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
                var epgView1 = tabItem1.Content as EpgDataViewItem;
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
