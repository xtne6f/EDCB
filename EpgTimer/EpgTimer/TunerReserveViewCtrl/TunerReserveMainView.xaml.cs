using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
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
        private List<TunerNameViewItem> tunerList = new List<TunerNameViewItem>();
        private List<ReserveViewItem> reserveList = new List<ReserveViewItem>();
        private Point clickPos;

        private CmdExeReserve mc; //予約系コマンド集
        private ContextMenu cmdMenu = new ContextMenu();//tunerReserveView.Contextmenu使うとフォーカスおかしくなる‥。

        public TunerReserveMainView()
        {
            InitializeComponent();

            tunerReserveView.PreviewMouseWheel += new MouseWheelEventHandler(tunerReserveView_PreviewMouseWheel);
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
            RefreshMenu();
        }
        public void RefreshMenu()
        {
            mBinds.ResetInputBindings(this);
            mm.CtxmGenerateContextMenu(cmdMenu, CtxmCode.TunerReserveView, false);
        }

        /// <summary>保持情報のクリア</summary>
        public bool ClearInfo()
        {
            tunerReserveView.ClearInfo();
            tunerReserveTimeView.ClearInfo();
            tunerReserveNameView.ClearInfo();
            tunerList = new List<TunerNameViewItem>();
            reserveList = new List<ReserveViewItem>();

            return true;
        }

        /// <summary>表示スクロールイベント呼び出し</summary>
        void tunerReserveView_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            vutil.view_ScrollChanged<TunerReserveView>(sender, e,
                tunerReserveView.scrollViewer, tunerReserveTimeView.scrollViewer, tunerReserveNameView.scrollViewer);
        }

        /// <summary>マウスホイールイベント呼び出し</summary>
        void tunerReserveView_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            vutil.view_PreviewMouseWheel<TunerReserveView>(sender, e, tunerReserveView.scrollViewer, Settings.Instance.TunerMouseScrollAuto, Settings.Instance.TunerScrollSize);
        }

        /// <summary>左ボタンダブルクリックイベント呼び出し/summary>
        void tunerReserveView_LeftDoubleClick(object sender, Point cursorPos)
        {
            clickPos = cursorPos;
            EpgCmds.ShowDialog.Execute(sender, cmdMenu);
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

        protected override bool ReloadInfoData()
        {
            if (vutil.ReloadReserveData(this) == false) return false;

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
                List<DateTime> timeList = new List<DateTime>();
                tunerList.Clear();
                reserveList.Clear();

                List<TunerReserveInfo> tunerReserveList = CommonManager.Instance.DB.TunerReserveList.Values
                    .OrderBy(info => info.tunerID).ToList();//多分大丈夫だけど一応ソートしておく
                if (Settings.Instance.TunerDisplayOffReserve == true)
                {
                    var tuner_off = new TunerReserveInfo();
                    tuner_off.tunerID = 0xFFFFFFFF;//IDの表示判定に使っている
                    tuner_off.tunerName = "無効予約";
                    tuner_off.reserveList = CommonManager.Instance.DB.ReserveList.Values
                        .Where(info => info.RecSetting.RecMode == 5).Select(info => info.ReserveID).ToList();
                    tunerReserveList.Add(tuner_off);
                }

                //チューナ不足と無効予約はアイテムがなければ非表示
                var delList = tunerReserveList.Where(item => item.tunerID == 0xFFFFFFFF && item.reserveList.Count == 0).ToList();
                delList.ForEach(item => tunerReserveList.Remove(item));

                double tunerWidthSingle = Settings.Instance.TunerWidth;
                double leftPos = 0;
                tunerReserveList.ForEach(info =>
                {
                    double tunerWidth = tunerWidthSingle;

                    var tunerAddList = new List<ReserveViewItem>();
                    foreach(UInt32 reserveID in info.reserveList)
                    {
                        ReserveData reserveInfo;
                        if (CommonManager.Instance.DB.ReserveList.TryGetValue(reserveID,out reserveInfo) == false)
                        {
                            continue;
                        }
                        var viewItem = new ReserveViewItem(reserveInfo);

                        //マージンを適用
                        DateTime startTime = reserveInfo.StartTime;
                        Int32 duration = (Int32)reserveInfo.DurationSecond;
                        vutil.ApplyMarginForPanelView(reserveInfo, ref startTime, ref duration);

                        DateTime EndTime = startTime.AddSeconds(duration);

                        viewItem.Height = duration * Settings.Instance.TunerMinHeight / 60;
                        viewItem.Width = tunerWidthSingle;
                        viewItem.LeftPos = leftPos;

                        foreach (ReserveViewItem addItem in tunerAddList)
                        {
                            ReserveData addInfo = addItem.ReserveInfo;

                            //マージンを適用
                            DateTime startTimeAdd = addInfo.StartTime;
                            Int32 durationAdd = (Int32)addInfo.DurationSecond;
                            vutil.ApplyMarginForPanelView(addInfo, ref startTimeAdd, ref durationAdd);

                            DateTime endTimeAdd = startTimeAdd.AddSeconds(durationAdd);

                            if ((startTimeAdd <= startTime && startTime < endTimeAdd) ||
                                (startTimeAdd < EndTime && EndTime <= endTimeAdd) ||
                                (startTime <= startTimeAdd && startTimeAdd < EndTime) ||
                                (startTime < endTimeAdd && endTimeAdd <= EndTime)
                                )
                            {
                                if (addItem.LeftPos >= viewItem.LeftPos)
                                {
                                    viewItem.LeftPos += tunerWidthSingle;
                                    if (viewItem.LeftPos - leftPos >= tunerWidth)
                                    {
                                        tunerWidth += tunerWidthSingle;
                                    }
                                }
                            }
                        }

                        reserveList.Add(viewItem);
                        tunerAddList.Add(viewItem);

                        //必要時間リストの構築
                        var chkStartTime = new DateTime(startTime.Year, startTime.Month, startTime.Day, startTime.Hour, 0, 0);
                        while (chkStartTime <= EndTime)
                        {
                            int index = timeList.BinarySearch(chkStartTime);
                            if (index < 0)
                            {
                                timeList.Insert(~index, chkStartTime);
                            }
                            chkStartTime = chkStartTime.AddHours(1);
                        }

                    }

                    tunerList.Add(new TunerNameViewItem(info, tunerWidth));
                    leftPos += tunerWidth;
                });

                //表示位置設定
                foreach (ReserveViewItem item in reserveList)
                {
                    DateTime startTime = item.ReserveInfo.StartTime;
                    if (item.ReserveInfo.RecSetting.UseMargineFlag == 1)
                    {
                        if (item.ReserveInfo.RecSetting.StartMargine < 0)
                        {
                            startTime = item.ReserveInfo.StartTime.AddSeconds(item.ReserveInfo.RecSetting.StartMargine * -1);
                        }
                    }

                    var chkStartTime = new DateTime(startTime.Year,
                        startTime.Month,
                        startTime.Day,
                        startTime.Hour,
                        0,
                        0);
                    int index = timeList.BinarySearch(chkStartTime);
                    if (index >= 0)
                    {
                        item.TopPos = (index * 60 + (startTime - chkStartTime).TotalMinutes) * Settings.Instance.TunerMinHeight;
                    }
                }

                //最低表示行数を適用。また、最低表示高さを確保して、位置も調整する。
                vutil.ModifierMinimumLine<ReserveData, ReserveViewItem>(reserveList, Settings.Instance.TunerMinimumLine, Settings.Instance.TunerFontSizeService);

                tunerReserveTimeView.SetTime(timeList, true);
                tunerReserveNameView.SetTunerInfo(tunerList);
                tunerReserveView.SetReserveList(reserveList,
                    leftPos,
                    timeList.Count * 60 * Settings.Instance.TunerMinHeight);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
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
