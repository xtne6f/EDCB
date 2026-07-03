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
using System.Windows.Threading;

using EpgTimer.TunerReserveViewCtrl;

namespace EpgTimer
{
    /// <summary>
    /// TunerReserveMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class TunerReserveMainView : UserControl
    {
        private List<ReserveViewItem> reserveList = new List<ReserveViewItem>();
        private bool updateReserveData = true;


        public TunerReserveMainView()
        {
            InitializeComponent();
        }

        /// <summary>
        /// 保持情報のクリア
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public bool ClearInfo()
        {
            tunerReserveView.ClearInfo();
            tunerReserveTimeView.ClearInfo();
            tunerReserveNameView.ClearInfo();
            reserveList.Clear();

            return true;
        }

        /// <summary>
        /// 表示スクロールイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void tunerReserveView_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            {
                {
                    //時間軸の表示もスクロール
                    tunerReserveTimeView.scrollViewer.ScrollToVerticalOffset(tunerReserveView.scrollViewer.VerticalOffset);
                    //サービス名表示もスクロール
                    tunerReserveNameView.scrollViewer.ScrollToHorizontalOffset(tunerReserveView.scrollViewer.HorizontalOffset);
                }
            }
        }

        /// <summary>
        /// マウス位置から予約情報を取得する
        /// </summary>
        /// <param name="cursorPos">[IN]マウス位置</param>
        /// <returns>nullで存在しない</returns>
        private ReserveData GetReserveItem(Point cursorPos)
        {
            foreach (ReserveViewItem resInfo in reserveList)
            {
                if (resInfo.LeftPos <= cursorPos.X && cursorPos.X < resInfo.LeftPos + resInfo.Width &&
                    resInfo.TopPos <= cursorPos.Y && cursorPos.Y < resInfo.TopPos + resInfo.Height)
                {
                    return resInfo.ReserveInfo;
                }
            }
            return null;
        }

        /// <summary>
        /// 左ボタンダブルクリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="cursorPos"></param>
        void tunerReserveView_LeftDoubleClick(object sender, Point cursorPos)
        {
            //まず予約情報あるかチェック
            ReserveData reserve = GetReserveItem(cursorPos);
            if (reserve != null)
            {
                //予約変更ダイアログ表示
                ChangeReserve(reserve);
            }
        }

        /// <summary>
        /// 右クリックメニュー 予約変更クリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_chg_Click(object sender, RoutedEventArgs e)
        {
            var reserve = (ReserveData)((MenuItem)sender).DataContext;
            ChangeReserve(reserve);
        }

        /// <summary>
        /// 右クリックメニュー 予約削除クリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_del_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var reserve = (ReserveData)((MenuItem)sender).DataContext;
                List<uint> list = new List<uint>();
                list.Add(reserve.ReserveID);
                ErrCode err = CommonManager.CreateSrvCtrl().SendDelReserve(list);
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約削除でエラーが発生しました。");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        /// <summary>
        /// 右クリックメニュー 有効無効イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_chg_no_Click(object sender, RoutedEventArgs e)
        {
            var reserve = (ReserveData)((MenuItem)sender).DataContext;
            byte originalRecMode = reserve.RecSetting.RecMode;
            //録画モード情報を維持して無効化
            reserve.RecSetting.RecMode = CommonManager.Instance.DB.CombineRecModeAndNoRec(reserve.RecSetting.GetRecMode(), true);
            string message = null;
            try
            {
                ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(new List<ReserveData>() { reserve });
                if (err != ErrCode.CMD_SUCCESS)
                {
                    message = CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。";
                }
            }
            catch (Exception ex)
            {
                message = ex.ToString();
            }
            reserve.RecSetting.RecMode = originalRecMode;
            if (message != null)
            {
                MessageBox.Show(message);
            }
        }

        /// <summary>
        /// 右クリックメニュー 予約モード変更イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_chg_recmode_Click(object sender, RoutedEventArgs e)
        {
            var reserve = (ReserveData)((MenuItem)sender).DataContext;
            byte originalRecMode = reserve.RecSetting.RecMode;
            reserve.RecSetting.RecMode = (byte)(sender == recmode_all ? 0 :
                                                sender == recmode_only ? 1 :
                                                sender == recmode_all_nodec ? 2 :
                                                sender == recmode_only_nodec ? 3 : 4);
            string message = null;
            try
            {
                ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(new List<ReserveData>() { reserve });
                if (err != ErrCode.CMD_SUCCESS)
                {
                    message = CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。";
                }
            }
            catch (Exception ex)
            {
                message = ex.ToString();
            }
            reserve.RecSetting.RecMode = originalRecMode;
            if (message != null)
            {
                MessageBox.Show(message);
            }
        }

        /// <summary>
        /// 右クリックメニュー 優先度変更イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_chg_priority_Click(object sender, RoutedEventArgs e)
        {
            var reserve = (ReserveData)((MenuItem)sender).DataContext;
            byte originalPriority = reserve.RecSetting.Priority;
            reserve.RecSetting.Priority = (byte)(sender == priority_1 ? 1 :
                                                 sender == priority_2 ? 2 :
                                                 sender == priority_3 ? 3 :
                                                 sender == priority_4 ? 4 : 5);
            string message = null;
            try
            {
                ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(new List<ReserveData>() { reserve });
                if (err != ErrCode.CMD_SUCCESS)
                {
                    message = CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。";
                }
            }
            catch (Exception ex)
            {
                message = ex.ToString();
            }
            reserve.RecSetting.Priority = originalPriority;
            if (message != null)
            {
                MessageBox.Show(message);
            }
        }

        /// <summary>
        /// 右クリックメニュー 自動予約登録イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_autoadd_Click(object sender, RoutedEventArgs e)
        {
            var reserve = (ReserveData)((MenuItem)sender).DataContext;
            SearchWindow search = ((MainWindow)Application.Current.MainWindow).CreateSearchWindow();

            var key = new EpgSearchKeyInfo();
            if (reserve.Title != null)
            {
                key.andKey = reserve.Title;
            }
            key.serviceList.Add((long)CommonManager.Create64Key(reserve.OriginalNetworkID, reserve.TransportStreamID, reserve.ServiceID));

            search.SetSearchDefKey(key);
            search.Show();
        }

        /// <summary>
        /// 右クリックメニュー 追っかけ再生イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_timeShiftPlay_Click(object sender, RoutedEventArgs e)
        {
            {
                var reserve = (ReserveData)((MenuItem)sender).DataContext;
                CommonManager.Instance.FilePlay(reserve.ReserveID);
            }
        }

        private void grid_content_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            Point cursorPos = Mouse.GetPosition(tunerReserveView.canvas);
            ReserveData reserve = GetReserveItem(cursorPos);
            grid_content.ContextMenu.DataContext = reserve;

            cm_chg.IsEnabled = reserve != null;
            cm_del.IsEnabled = reserve != null;
            cm_autoadd.IsEnabled = reserve != null;
            cm_timeshift.IsEnabled = reserve != null;
            if (reserve != null)
            {
                for (int i = 0; i <= 4; i++)
                {
                    ((MenuItem)cm_chg.Items[cm_chg.Items.IndexOf(recmode_all) + i]).IsChecked = (i == reserve.RecSetting.GetRecMode());
                }
                for (int i = 0; i < cm_pri.Items.Count; i++)
                {
                    ((MenuItem)cm_pri.Items[i]).IsChecked = (i + 1 == reserve.RecSetting.Priority);
                }
                cm_pri.Header = string.Format((string)cm_pri.Tag, reserve.RecSetting.Priority);
            }
            else
            {
                //表示をキャンセル
                e.Handled = true;
            }
        }

        /// <summary>
        /// 予約変更
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ChangeReserve(ReserveData reserveInfo)
        {
            var win = new ChgReserveWindow();
            ((MainWindow)Application.Current.MainWindow).SwapOwnedReserveWindow(win);
            win.SetReserveInfo(reserveInfo);
            win.Show();
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            var ps = PresentationSource.FromVisual(this);
            if (ps != null)
            {
                //高DPI環境でTunerReserveViewの位置を物理ピクセルに合わせるためにヘッダの幅を微調整する
                //RootにUseLayoutRoundingを適用できれば不要だがボタン等が低品質になるので自力でやる
                Point p = grid_container.TransformToVisual(ps.RootVisual).Transform(new Point(40, 40));
                Matrix m = ps.CompositionTarget.TransformToDevice;
                grid_container.ColumnDefinitions[0].Width = new GridLength(40 + Math.Floor(p.X * m.M11) / m.M11 - p.X);
                grid_container.RowDefinitions[0].Height = new GridLength(40 + Math.Floor(p.Y * m.M22) / m.M22 - p.Y);
            }
        }

        /// <summary>
        /// 予約情報更新通知
        /// </summary>
        public void Refresh()
        {
            updateReserveData = true;
            if (this.IsVisible == true)
            {
                ReloadReserveViewItem();
                updateReserveData = false;
            }
        }

        /// <summary>
        /// 予約情報の再描画
        /// </summary>
        private void ReloadReserveViewItem()
        {
            tunerReserveView.ClearInfo();
            tunerReserveTimeView.ClearInfo();
            tunerReserveNameView.ClearInfo();
            var timeList = new List<DateTime>();
            var tunerList = new List<TunerNameViewItem>();
            reserveList.Clear();
            try
            {
                double leftPos = 0;
                foreach (TunerReserveInfo info in CommonManager.Instance.DB.TunerReserveList.Values)
                {
                    double width = 150;
                    int addOffset = reserveList.Count();
                    foreach (uint reserveID in info.reserveList)
                    {
                        ReserveData reserveInfo;
                        if (CommonManager.Instance.DB.ReserveList.TryGetValue(reserveID, out reserveInfo) == false)
                        {
                            continue;
                        }

                        DateTime startTime = reserveInfo.StartTime;
                        DateTime endTime = startTime.AddSeconds(reserveInfo.DurationSecond);
                        if (reserveInfo.RecSetting.UseMargineFlag == 1)
                        {
                            if (reserveInfo.RecSetting.StartMargine < 0)
                            {
                                startTime = startTime.AddSeconds(-reserveInfo.RecSetting.StartMargine);
                            }
                            if (reserveInfo.RecSetting.EndMargine < 0)
                            {
                                endTime = endTime.AddSeconds(reserveInfo.RecSetting.EndMargine);
                            }
                        }

                        var viewItem = new ReserveViewItem(reserveInfo);
                        viewItem.Height = Math.Floor((endTime - startTime).TotalMinutes * Settings.Instance.EpgSettingList[0].MinHeight);
                        if (viewItem.Height < Settings.Instance.EpgSettingList[0].MinHeight)
                        {
                            viewItem.Height = Settings.Instance.EpgSettingList[0].MinHeight;
                        }
                        viewItem.Width = 150;
                        viewItem.LeftPos = leftPos;

                        for (int i = addOffset; i < reserveList.Count; i++)
                        {
                            ReserveData addInfo = reserveList[i].ReserveInfo;
                            DateTime startTimeAdd = addInfo.StartTime;
                            DateTime endTimeAdd = startTimeAdd.AddSeconds(addInfo.DurationSecond);
                            if (addInfo.RecSetting.UseMargineFlag == 1)
                            {
                                if (addInfo.RecSetting.StartMargine < 0)
                                {
                                    startTimeAdd = startTimeAdd.AddSeconds(-addInfo.RecSetting.StartMargine);
                                }
                                if (addInfo.RecSetting.EndMargine < 0)
                                {
                                    endTimeAdd = endTimeAdd.AddSeconds(addInfo.RecSetting.EndMargine);
                                }
                            }

                            if ((startTimeAdd <= startTime && startTime < endTimeAdd) ||
                                (startTimeAdd < endTime && endTime <= endTimeAdd) ||
                                (startTime <= startTimeAdd && startTimeAdd < endTime) ||
                                (startTime < endTimeAdd && endTimeAdd <= endTime)
                                )
                            {
                                if (reserveList[i].LeftPos == viewItem.LeftPos)
                                {
                                    //追加済みのものと重なるので移動して再チェック
                                    i = addOffset - 1;
                                    viewItem.LeftPos += 150;
                                    width = Math.Max(width, viewItem.LeftPos + viewItem.Width - leftPos);
                                }
                            }
                        }

                        reserveList.Add(viewItem);

                        //必要時間リストの構築

                        DateTime chkStartTime = new DateTime(startTime.Year, startTime.Month, startTime.Day, startTime.Hour, 0, 0);
                        while (chkStartTime <= endTime)
                        {
                            int index = timeList.BinarySearch(chkStartTime);
                            if (index < 0)
                            {
                                timeList.Insert(~index, chkStartTime);
                            }
                            chkStartTime = chkStartTime.AddHours(1);
                        }

                    }
                    tunerList.Add(new TunerNameViewItem(info, width));
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
                            startTime = startTime.AddSeconds(-item.ReserveInfo.RecSetting.StartMargine);
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
                        item.TopPos = (index * 60 + (startTime - chkStartTime).TotalMinutes) * Settings.Instance.EpgSettingList[0].MinHeight;
                    }
                }

                tunerReserveTimeView.SetTime(timeList, true);
                tunerReserveNameView.SetTunerInfo(tunerList);
                tunerReserveView.SetReserveList(reserveList,
                    leftPos,
                    timeList.Count * 60 * Settings.Instance.EpgSettingList[0].MinHeight);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (updateReserveData && IsVisible)
            {
                ReloadReserveViewItem();
                updateReserveData = false;
            }
        }
    }
}
