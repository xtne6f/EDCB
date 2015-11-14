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
            tunerReserveView.LeftDoubleClick += new TunerReserveView.ProgramViewClickHandler(tunerReserveView_LeftDoubleClick);
            tunerReserveView.RightClick += new TunerReserveView.ProgramViewClickHandler(tunerReserveView_RightClick);

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
            vutil.view_PreviewMouseWheel<TunerReserveView>(sender, e, tunerReserveView.scrollViewer);
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

                double leftPos = 0;
                for (int i = 0; i < CommonManager.Instance.DB.TunerReserveList.Count; i++)
                {
                    double width = 150;
                    TunerReserveInfo info = CommonManager.Instance.DB.TunerReserveList.Values.ElementAt(i);
                    TunerNameViewItem tunerInfo = new TunerNameViewItem(info, width);
                    tunerList.Add(tunerInfo);

                    List<ReserveViewItem> tunerAddList = new List<ReserveViewItem>();
                    for (int j = 0; j < info.reserveList.Count; j++ )
                    {
                        UInt32 reserveID = (UInt32)info.reserveList[j];
                        if (CommonManager.Instance.DB.ReserveList.ContainsKey(reserveID) == false)
                        {
                            continue;
                        }
                        ReserveData reserveInfo = CommonManager.Instance.DB.ReserveList[reserveID];
                        ReserveViewItem viewItem = new ReserveViewItem(CommonManager.Instance.DB.ReserveList[reserveID]);

                        //マージンを適用
                        Int32 duration = (Int32)reserveInfo.DurationSecond;
                        DateTime startTime = reserveInfo.StartTime;
                        vutil.ApplyMarginForPanelView(reserveInfo, ref duration, ref startTime, true);

                        DateTime EndTime = startTime.AddSeconds(duration);

                        viewItem.Height = Math.Max((duration * Settings.Instance.MinHeight) / 60, Settings.Instance.MinHeight);
                        viewItem.Width = 150;
                        viewItem.LeftPos = leftPos;

                        foreach (ReserveViewItem addItem in tunerAddList)
                        {
                            ReserveData addInfo = addItem.ReserveInfo;

                            //マージンを適用
                            Int32 durationAdd = (Int32)addInfo.DurationSecond;
                            DateTime startTimeAdd = addInfo.StartTime;
                            vutil.ApplyMarginForPanelView(addInfo, ref durationAdd, ref startTimeAdd, true);
                            
                            DateTime endTimeAdd = startTimeAdd.AddSeconds(durationAdd);

                            if ((startTimeAdd <= startTime && startTime < endTimeAdd) ||
                                (startTimeAdd < EndTime && EndTime <= endTimeAdd) || 
                                (startTime <= startTimeAdd && startTimeAdd < EndTime) ||
                                (startTime < endTimeAdd && endTimeAdd <= EndTime)
                                )
                            {
                                if (addItem.LeftPos >= viewItem.LeftPos)
                                {
                                    viewItem.LeftPos += 150;
                                    if (viewItem.LeftPos - leftPos >= width)
                                    {
                                        width += 150;
                                    }
                                }
                            }
                        }

                        reserveList.Add(viewItem);
                        tunerAddList.Add(viewItem);

                        //必要時間リストの構築

                        DateTime chkStartTime = new DateTime(startTime.Year, startTime.Month, startTime.Day, startTime.Hour, 0, 0);
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
                    tunerInfo.Width = width;
                    leftPos += width;
                }

                //表示位置設定
                foreach (ReserveViewItem item in reserveList)
                {
                    DateTime startTime = item.ReserveInfo.StartTime;
                    if (item.ReserveInfo.RecSetting.UseMargineFlag == 1)
                    {
                        if (item.ReserveInfo.RecSetting.StartMargine < 0)
                        {
                            startTime = item.ReserveInfo.StartTime.AddSeconds(item.ReserveInfo.RecSetting.StartMargine*-1);
                        }
                    }

                    DateTime chkStartTime = new DateTime(startTime.Year,
                        startTime.Month,
                        startTime.Day,
                        startTime.Hour,
                        0,
                        0);
                    int index = timeList.BinarySearch(chkStartTime);
                    if (index >= 0)
                    {
                        item.TopPos = (index * 60 + (startTime - chkStartTime).TotalMinutes) * Settings.Instance.MinHeight;
                    }
                }

                tunerReserveTimeView.SetTime(timeList, true);
                tunerReserveNameView.SetTunerInfo(tunerList);
                tunerReserveView.SetReserveList(reserveList,
                    leftPos,
                    timeList.Count * 60 * Settings.Instance.MinHeight);
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

            if (BlackoutWindow.SelectedReserveItem != null)
            {
                MoveToReserveItem(BlackoutWindow.SelectedReserveItem, BlackoutWindow.NowJumpTable);
            }

            BlackoutWindow.Clear();
        }

        protected void MoveToReserveItem(ReserveItem target, bool IsMarking)
        {
            uint ID = target.ReserveInfo.ReserveID;
            ReserveViewItem target_item = this.reserveList.Find(item => item.ReserveInfo.ReserveID == ID);
            this.tunerReserveView.ScrollToFindItem(target_item, IsMarking);
        }

    }
}
