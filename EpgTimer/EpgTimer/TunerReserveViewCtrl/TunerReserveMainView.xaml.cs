using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;
using EpgTimer.TunerReserveViewCtrl;

namespace EpgTimer
{
    /// <summary>
    /// TunerReserveMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class TunerReserveMainView : UserControl
    {
        private List<TunerNameViewItem> tunerList = new List<TunerNameViewItem>();
        private List<ReserveViewItem> reserveList = new List<ReserveViewItem>();
        private Point clickPos;
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private ViewUtil vutil = CommonManager.Instance.VUtil;
        private MenuManager mm = CommonManager.Instance.MM;
        private MenuBinds mBinds = new MenuBinds();

        private bool ReloadInfo = true;

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
            mc.SetFuncGetDataList(isAll => vutil.GetPanelDataList<ReserveData>(GetReserveItem, clickPos));

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

        /// <summary>マウス位置から予約情報を取得する</summary>
        /// <param name="cursorPos">[IN]マウス位置</param>
        /// <param name="reserve">[OUT]予約情報</param>
        /// <returns>falseで存在しない</returns>
        private bool GetReserveItem(Point cursorPos, ref ReserveData reserve)
        {
            return vutil.GetHitItem(cursorPos, ref reserve, reserveList);
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
        
        /// <summary>情報の更新通知</summary>
        public void UpdateInfo()
        {
            ReloadInfo = true;
            if (ReloadInfo == true && this.IsVisible == true) ReloadInfo = !ReloadInfoData();
        }
        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (ReloadInfo == true && this.IsVisible == true) ReloadInfo = !ReloadInfoData();
        }
        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (ReloadInfo == true && this.IsVisible == true) ReloadInfo = !ReloadInfoData();
        }
        private bool ReloadInfoData()
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

                //TODO: ここでデフォルトマージンを確認するがEpgTimerNWでは無意味。根本的にはSendCtrlCmdの拡張が必要
                int defStartMargin = IniFileHandler.GetPrivateProfileInt("SET", "StartMargin", 0, SettingPath.TimerSrvIniPath);
                int defEndMargin = IniFileHandler.GetPrivateProfileInt("SET", "EndMargin", 0, SettingPath.TimerSrvIniPath);

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
                        vutil.ApplyMarginForPanelView(reserveInfo,
                            ref duration, ref startTime, defStartMargin, defEndMargin, true);

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
                            vutil.ApplyMarginForPanelView(addInfo,
                                ref durationAdd, ref startTimeAdd, defStartMargin, defEndMargin, true);
                            
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

    }
}
