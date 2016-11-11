using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

using EpgTimer.TunerReserveViewCtrl;
using EpgTimer.UserCtrlView;

namespace EpgTimer
{
    /// <summary>
    /// TunerReserveMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class TunerReserveMainView : DataViewBase
    {
        private List<ReserveViewItem> reserveList = new List<ReserveViewItem>();
        private Point clickPos;

        private CmdExeReserve mc; //予約系コマンド集
        private ContextMenu cmdMenu = new ContextMenu();//tunerReserveView.Contextmenu使うとフォーカスおかしくなる‥。

        public TunerReserveMainView()
        {
            InitializeComponent();

            tunerReserveView.ScrollChanged += new ScrollChangedEventHandler(tunerReserveView_ScrollChanged);
            tunerReserveView.LeftDoubleClick += new TunerReserveView.PanelViewClickHandler(tunerReserveView_LeftDoubleClick);
            tunerReserveView.RightClick += new TunerReserveView.PanelViewClickHandler(tunerReserveView_RightClick);

            //ビューコードの登録
            mBinds.View = CtxmCode.TunerReserveView;

            //最初にコマンド集の初期化
            mc = new CmdExeReserve(this);
            mc.SetFuncGetDataList(isAll => isAll == true ? reserveList.GetDataList() : reserveList.GetHitDataList(clickPos));

            //コマンド集からコマンドを登録
            mc.ResetCommandBindings(this, cmdMenu);

            //メニューの作成、ショートカットの登録
            //RefreshMenu();
        }
        public void RefreshMenu()
        {
            mBinds.ResetInputBindings(this);
            mm.CtxmGenerateContextMenu(cmdMenu, CtxmCode.TunerReserveView, false);
        }

        /// <summary>表示スクロールイベント呼び出し</summary>
        void tunerReserveView_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            tunerReserveView.view_ScrollChanged(tunerReserveView.scrollViewer, tunerReserveTimeView.scrollViewer, tunerReserveNameView.scrollViewer);
        }

        /// <summary>左ボタンダブルクリックイベント呼び出し/summary>
        void tunerReserveView_LeftDoubleClick(object sender, Point cursorPos)
        {
            clickPos = cursorPos;
            EpgCmds.ShowDialog.Execute(null, cmdMenu);
        }

        /// <summary>右ボタンクリック</summary>
        protected void sub_erea_MouseRightButtonUp(object sender, MouseButtonEventArgs e)
        {
            tunerReserveView_RightClick(sender, new Point(-1, -1));
        }
        void tunerReserveView_RightClick(object sender, Point cursorPos)
        {
            //右クリック表示メニューの作成
            clickPos = cursorPos;
            mc.SupportContextMenuLoading(cmdMenu, null);
        }

        protected override void UpdateStatusData(int mode = 0)
        {
            this.status[1] = ViewUtil.ConvertReserveStatus(reserveList.GetDataList(), "予約数", 3);
        }
        protected override bool ReloadInfoData()
        {
            if (ViewUtil.ReloadReserveData(this) == false) return false;

            ReloadReserveViewItem();
            return true;
        }
        /// <summary>
        /// 予約情報の再描画
        /// </summary>
        private void ReloadReserveViewItem()
        {
            try
            {
                tunerReserveView.ClearInfo();
                tunerReserveTimeView.ClearInfo();
                tunerReserveNameView.ClearInfo();
                reserveList.Clear();

                var tunerList = new List<PanelItem<TunerReserveInfo>>();
                var timeList = new List<DateTime>();

                List<TunerReserveInfo> tunerReserveList = CommonManager.Instance.DB.TunerReserveList.Values
                    .OrderBy(info => info.tunerID).ToList();//多分大丈夫だけど一応ソートしておく
                if (Settings.Instance.TunerDisplayOffReserve == true)
                {
                    var tuner_off = new TunerReserveInfo();
                    tuner_off.tunerID = 0xFFFFFFFF;//IDの表示判定に使っている
                    tuner_off.tunerName = "無効予約";
                    tuner_off.reserveList = CommonManager.Instance.DB.ReserveList.Values
                        .Where(info => info.IsEnabled == false).Select(info => info.ReserveID).ToList();
                    tunerReserveList.Add(tuner_off);
                }

                //チューナ不足と無効予約はアイテムがなければ非表示
                tunerReserveList.RemoveAll(item => item.tunerID == 0xFFFFFFFF && item.reserveList.Count == 0);

                double tunerWidthSingle = Settings.Instance.TunerWidth;
                double leftPos = 0;
                var resDic = CommonManager.Instance.DB.ReserveList;
                tunerReserveList.ForEach(info =>
                {
                    double tunerWidth = tunerWidthSingle;
                    
                    var tunerAddList = new List<ReserveViewItem>();

                    foreach (ReserveData resInfo in info.reserveList.Where(id => resDic.ContainsKey(id) == true).Select(id => resDic[id]))
                    {
                        var newItem = new ReserveViewItem(resInfo);
                        reserveList.Add(newItem);

                        //横位置の設定
                        newItem.Width = tunerWidthSingle;
                        newItem.LeftPos = leftPos;

                        //列を拡げて表示する処置
                        foreach (ReserveViewItem addedItem in tunerAddList)
                        {
                            ReserveData addedInfo = addedItem.ReserveInfo;

                            if (MenuUtil.CulcOverlapLength(resInfo.StartTimeActual, resInfo.DurationActual, addedInfo.StartTimeActual, addedInfo.DurationActual) > 0)
                            {
                                if (newItem.LeftPos <= addedItem.LeftPos)
                                {
                                    if (resInfo.Create64Key() == addedInfo.Create64Key())
                                    {
                                        newItem.LeftPos = addedItem.LeftPos;
                                    }
                                    else
                                    {
                                        newItem.LeftPos = addedItem.LeftPos + tunerWidthSingle;
                                        if (newItem.LeftPos - leftPos >= tunerWidth)
                                        {
                                            tunerWidth += tunerWidthSingle;
                                        }
                                    }
                                }
                            }
                        }
                        tunerAddList.Add(newItem);

                        //マージン込みの時間でリストを構築
                        ViewUtil.AddTimeList(timeList, resInfo.StartTimeActual, resInfo.DurationActual);
                    }

                    tunerList.Add(new PanelItem<TunerReserveInfo>(info) { Width = tunerWidth });
                    leftPos += tunerWidth;
                });

                //縦位置の設定
                timeList = timeList.Distinct().OrderBy(time => time).ToList();
                reserveList.ForEach(item =>
                {
                    ViewUtil.SetItemVerticalPos(timeList, item, item.ReserveInfo.StartTimeActual, item.ReserveInfo.DurationActual, Settings.Instance.TunerMinHeight, true);

                    //ごく小さいマージンの表示を抑制する。
                    item.TopPos = Math.Round(item.TopPos);
                    item.Height = Math.Round(item.Height);
                });

                //最低表示行数を適用。また、最低表示高さを確保して、位置も調整する。
                ViewUtil.ModifierMinimumLine(reserveList, Settings.Instance.TunerMinimumLine, Settings.Instance.TunerFontSizeService);

                //必要時間リストの修正。最低表示行数の適用で下に溢れた分を追加する。
                ViewUtil.AdjustTimeList(reserveList, timeList, Settings.Instance.TunerMinHeight);

                tunerReserveTimeView.SetTime(timeList, false, true);
                tunerReserveNameView.SetTunerInfo(tunerList);
                tunerReserveView.SetReserveList(reserveList,
                    leftPos,
                    timeList.Count * 60 * Settings.Instance.TunerMinHeight);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        protected override void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            base.UserControl_IsVisibleChanged(sender, e);

            if (this.IsVisible == false) return;

            if (BlackoutWindow.HasReserveData == true)
            {
                MoveToReserveItem(BlackoutWindow.SelectedItem.ReserveInfo, BlackoutWindow.NowJumpTable);
            }

            BlackoutWindow.Clear();
        }

        protected void MoveToReserveItem(ReserveData target, bool IsMarking)
        {
            uint ID = target.ReserveID;
            ReserveViewItem target_item = this.reserveList.Find(item => item.ReserveInfo.ReserveID == ID);
            this.tunerReserveView.ScrollToFindItem(target_item, IsMarking);
        }

    }
}
