using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;

namespace EpgTimer
{
    /// <summary>
    /// ChgReserveWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class ChgReserveWindow : ChgReserveWindowBase
    {
        protected override UInt64 DataID { get { return reserveInfo == null ? 0 : reserveInfo.ReserveID; } }

        private ReserveData reserveInfo = null;

        protected enum AddMode { Add, Re_Add, Change }
        private AddMode addMode = AddMode.Add;   //予約モード、再予約モード、変更モード
        private int openMode = 0;                  //EPGViewで番組表を表示するかどうか
        private bool resModeProgram = true;         //プログラム予約かEPG予約か

        private EpgEventInfo eventInfoNow = null;
        private ReserveData resInfoDisplay = null;

        public ChgReserveWindow()
        {
            InitializeComponent();

            base.SetParam(false, checkBox_windowPinned, checkBox_dataReplace);

            //コマンドの登録
            this.CommandBindings.Add(new CommandBinding(EpgCmds.Cancel, (sender, e) => this.Close()));
            this.CommandBindings.Add(new CommandBinding(EpgCmds.AddInDialog, button_chg_reserve_Click, (sender, e) => e.CanExecute = addMode != AddMode.Change));
            this.CommandBindings.Add(new CommandBinding(EpgCmds.ChangeInDialog, button_chg_reserve_Click, (sender, e) => e.CanExecute = addMode == AddMode.Change));
            this.CommandBindings.Add(new CommandBinding(EpgCmds.DeleteInDialog, button_del_reserve_Click, (sender, e) => e.CanExecute = addMode == AddMode.Change));

            //ボタンの設定
            mBinds.SetCommandToButton(button_cancel, EpgCmds.Cancel);
            mBinds.AddInputCommand(EpgCmds.AddInDialog);//ボタンへの割り振りは後で
            mBinds.AddInputCommand(EpgCmds.ChangeInDialog);//ボタンへの割り振りは後で
            mBinds.SetCommandToButton(button_del_reserve, EpgCmds.DeleteInDialog);
            mBinds.ResetInputBindings(this);

            //その他設定
            //深夜時間関係は、comboBoxの表示だけ変更する手もあるが、
            //オプション変更タイミングなどいろいろ面倒なので、実際の値で処理することにする。
            comboBox_service.ItemsSource = ChSet5.ChList.Values;
            comboBox_sh.ItemsSource = CommonManager.CustomHourList;
            comboBox_eh.ItemsSource = CommonManager.CustomHourList;
            comboBox_sm.ItemsSource = Enumerable.Range(0, 60);
            comboBox_em.ItemsSource = Enumerable.Range(0, 60);
            comboBox_ss.ItemsSource = Enumerable.Range(0, 60);
            comboBox_es.ItemsSource = Enumerable.Range(0, 60);
        }

        //Addモードではデータの入れ替え関係は未使用なので保存しない
        protected override void SaveDataReplace()
        {
            if (addMode != AddMode.Add) base.SaveDataReplace();
        }

        private void SetAddMode(AddMode mode)
        {
            addMode = mode;
            mBinds.SetCommandToButton(button_chg_reserve, mode == AddMode.Change ? EpgCmds.ChangeInDialog : EpgCmds.AddInDialog);
            button_del_reserve.Visibility = mode == AddMode.Add ? Visibility.Collapsed : Visibility.Visible;
            checkBox_dataReplace.Visibility = mode == AddMode.Add ? Visibility.Collapsed : Visibility.Visible;
            stack_Status.Visibility = mode == AddMode.Add ? Visibility.Collapsed : Visibility.Visible;
            stack_Status.IsEnabled = mode == AddMode.Change;
            switch (mode)
            {
                case AddMode.Add:
                    button_chg_reserve.Content = "予約";
                    this.EnableDataChange = false;
                    break;
                case AddMode.Re_Add:
                    button_chg_reserve.Content = "再予約";
                    checkBox_releaseAutoAdd.IsChecked = false;
                    text_Status.ItemsSource = null;
                    label_errStar.Content = null;
                    //なお、削除ボタンはCanExeの判定でグレーアウトする。
                    break;
                case AddMode.Change:
                    button_chg_reserve.Content = "変更";
                    break;
            }
        }
        private void SetResModeProgram(bool mode)
        {
            resModeProgram = mode;

            radioButton_Epg.IsChecked = !resModeProgram;
            radioButton_Program.IsChecked = resModeProgram;

            textBox_title.SetReadOnlyWithEffect(!resModeProgram);
            comboBox_service.IsEnabled = resModeProgram;
            stack_start.IsEnabled = resModeProgram;
            stack_end.IsEnabled = resModeProgram;
            recSettingView.SetViewMode(!resModeProgram);
        }

        public void SetReserveInfo(ReserveData info, int? epgInfoOpenMode = null)
        {
            if (info == null) return;
            if (epgInfoOpenMode != null) openMode = epgInfoOpenMode == 1 ? 1 : 0;
            addMode = AddMode.Change;
            reserveInfo = info.Clone();
            recSettingView.SetDefSetting(reserveInfo.RecSetting, reserveInfo.IsManual);
            checkBox_releaseAutoAdd.IsChecked = false;
            checkBox_releaseAutoAdd.IsEnabled = reserveInfo.IsAutoAdded;
        }
        public void ChangeReserveInfo(ReserveData info)
        {
            if (info == null) return;
            SetReserveInfo(info);
            UpdateWindow();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            UpdateWindow();
        }

        private void UpdateWindow()
        {
            if (reserveInfo == null)
            {
                addMode = AddMode.Add;
                openMode = 0;
                reserveInfo = new ReserveData();
                reserveInfo.StartTime = DateTime.Now.AddMinutes(1);
                reserveInfo.StartTimeEpg = reserveInfo.StartTime;
                reserveInfo.DurationSecond = 1800;
                reserveInfo.EventID = 0xFFFF;
            }

            SetAddMode(addMode);
            SetResModeProgram(reserveInfo.IsManual);
            SetReserveTimeInfo(reserveInfo);

            //エラー状況の表示
            text_Status.ItemsSource = null;

            if (addMode != AddMode.Add)
            {
                var resItem = new ReserveItem(reserveInfo);
                text_Status.ItemsSource = new string[] { resItem.CommentBase }.Concat(resItem.ErrComment.Select(s => "＊" + s));
                text_Status.SelectedIndex = 0;
                label_errStar.Content = text_Status.Items.Count > 1 ? string.Format("＊×{0}", text_Status.Items.Count - 1) : null;
            }

            //番組詳細タブを初期化
            richTextBox_descInfo.Document = CommonManager.ConvertDisplayText(null);
            eventInfoNow = null;
            resInfoDisplay = null;

            tabControl.SelectedIndex = -1;
            tabControl.SelectedIndex = openMode;
        }

        private void SetReserveTimeInfo(ReserveData resInfo)
        {
            if (resInfo == null) return;

            try
            {
                Title = ViewUtil.WindowTitleText(resInfo.Title, addMode == AddMode.Add ? "予約登録" : "予約変更");

                //テキストの選択位置を戻す
                textBox_title.Text = null;
                Dispatcher.BeginInvoke(new Action(() => textBox_title.Text = resInfo.Title), DispatcherPriority.Render);

                comboBox_service.SelectedIndex = 0;
                foreach (ChSet5Item ch in comboBox_service.Items)
                {
                    if (ch.Key == resInfo.Create64Key())
                    {
                        comboBox_service.SelectedItem = ch;
                        break;
                    }
                }

                DateTime startTime = resInfo.StartTime;
                DateTime endTime = resInfo.StartTime.AddSeconds(resInfo.DurationSecond);

                //深夜時間帯の処理
                bool use28 = Settings.Instance.LaterTimeUse == true && (endTime - startTime).TotalDays < 1;
                bool late_start = use28 && startTime.Hour + 24 < comboBox_sh.Items.Count && DateTime28.IsLateHour(startTime.Hour);
                bool late_end = use28 && endTime.Hour + 24 < comboBox_eh.Items.Count && DateTime28.JudgeLateHour(endTime, startTime);

                datePicker_start.SelectedDate = startTime.AddDays(late_start == true ? -1 : 0);
                comboBox_sh.SelectedIndex = startTime.Hour + (late_start == true ? 24 : 0);
                comboBox_sm.SelectedIndex = startTime.Minute;
                comboBox_ss.SelectedIndex = startTime.Second;

                datePicker_end.SelectedDate = endTime.AddDays(late_end == true ? -1 : 0);
                comboBox_eh.SelectedIndex = endTime.Hour + (late_end == true ? 24 : 0);
                comboBox_em.SelectedIndex = endTime.Minute;
                comboBox_es.SelectedIndex = endTime.Second;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        private int GetReserveTimeInfo(ref ReserveData resInfo)
        {
            if (resInfo == null) return -1;

            try
            {
                resInfo.Title = textBox_title.Text;
                ChSet5Item ch = comboBox_service.SelectedItem as ChSet5Item;

                resInfo.StationName = ch.ServiceName;
                resInfo.OriginalNetworkID = ch.ONID;
                resInfo.TransportStreamID = ch.TSID;
                resInfo.ServiceID = ch.SID;

                //深夜時間帯の処理
                DateTime date_start = datePicker_start.SelectedDate.Value.AddDays((int)(comboBox_sh.SelectedIndex / 24));
                DateTime date_end = datePicker_end.SelectedDate.Value.AddDays((int)(comboBox_eh.SelectedIndex / 24));

                resInfo.StartTime = new DateTime(date_start.Year, date_start.Month, date_start.Day,
                    comboBox_sh.SelectedIndex % 24,
                    comboBox_sm.SelectedIndex,
                    comboBox_ss.SelectedIndex,
                    0,
                    DateTimeKind.Utc
                    );
                resInfo.StartTimeEpg = resInfo.StartTime;

                DateTime endTime = new DateTime(date_start.Year, date_end.Month, date_end.Day,
                    comboBox_eh.SelectedIndex % 24,
                    comboBox_em.SelectedIndex,
                    comboBox_es.SelectedIndex,
                    0,
                    DateTimeKind.Utc
                    );
                if (resInfo.StartTime > endTime)
                {
                    resInfo.DurationSecond = 0;
                    return -2;
                }
                else
                {
                    TimeSpan duration = endTime - resInfo.StartTime;
                    resInfo.DurationSecond = (uint)duration.TotalSeconds;
                    return 0;
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return -1;
        }

        private bool CheckExistReserveItem()
        {
            bool retval = CommonManager.Instance.DB.ReserveList.ContainsKey(this.reserveInfo.ReserveID);
            if (retval == false)
            {
                MessageBox.Show("項目がありません。\r\n" + "既に削除されています。", "データエラー", MessageBoxButton.OK, MessageBoxImage.Exclamation);

                SetAddMode(AddMode.Re_Add);
            }
            return retval;
        }

        private void button_chg_reserve_Click(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                if (CmdExeUtil.IsDisplayKgMessage(e) == true)
                {
                    bool change_proc = false;
                    switch (addMode)
                    {
                        case AddMode.Add:
                            change_proc = (MessageBox.Show("予約を追加します。\r\nよろしいですか？", "予約の確認", MessageBoxButton.OKCancel) == MessageBoxResult.OK);
                            break;
                        case AddMode.Re_Add:
                            change_proc = (MessageBox.Show("この内容で再予約します。\r\nよろしいですか？", "再予約の確認", MessageBoxButton.OKCancel) == MessageBoxResult.OK);
                            break;
                        case AddMode.Change:
                            change_proc = (MessageBox.Show("この予約を変更します。\r\nよろしいですか？", "変更の確認", MessageBoxButton.OKCancel) == MessageBoxResult.OK);
                            break;
                    }
                    if (change_proc == false) return;
                }

                if (addMode == AddMode.Change && CheckExistReserveItem() == false) return;

                var resInfo = reserveInfo.Clone();

                if (resModeProgram == true)
                {
                    if (GetReserveTimeInfo(ref resInfo) == -2)
                    {
                        MessageBox.Show("終了日時が開始日時より前です");
                        return;
                    }

                    //サービスや時間が変わったら、個別予約扱いにする。タイトルのみ変更は見ない。
                    if (resInfo.EventID != 0xFFFF || reserveInfo.IsSamePg(resInfo) == false)
                    {
                        resInfo.EventID = 0xFFFF;
                        resInfo.Comment = "";
                    }
                }
                else
                {
                    //EPG予約に変える場合、またはEPG予約で別の番組に変わる場合
                    if (eventInfoNow != null && (reserveInfo.IsManual == true || reserveInfo.IsSamePg(eventInfoNow) == false))
                    {
                        //基本的にAddReserveEpgWindowと同じ処理内容
                        if (MenuUtil.IsEnableReserveAdd(eventInfoNow) == false) return;
                        eventInfoNow.ConvertToReserveData(ref resInfo);
                        resInfo.Comment = "";
                    }
                }
                if (addMode != AddMode.Change || checkBox_releaseAutoAdd.IsChecked == true)
                {
                    resInfo.Comment = "";
                }

                resInfo.RecSetting = recSettingView.GetRecSetting();

                if (addMode == AddMode.Change)
                {
                    bool ret = MenuUtil.ReserveChange(CommonUtil.ToList(resInfo));
                    StatusManager.StatusNotifySet(ret, "録画予約を変更");
                }
                else
                {
                    bool ret = MenuUtil.ReserveAdd(CommonUtil.ToList(resInfo));
                    StatusManager.StatusNotifySet(ret, "録画予約を追加");
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }

            this.Close();
        }
        private void button_del_reserve_Click(object sender, ExecutedRoutedEventArgs e)
        {
            if (CmdExeUtil.IsDisplayKgMessage(e) == true)
            {
                if (MessageBox.Show("この予約を削除します。\r\nよろしいですか？", "削除の確認", MessageBoxButton.OKCancel) != MessageBoxResult.OK)
                { return; }
            }

            if (CheckExistReserveItem() == false) return;

            bool ret = MenuUtil.ReserveDelete(CommonUtil.ToList(reserveInfo));
            StatusManager.StatusNotifySet(ret, "録画予約を削除");

            this.Close();
        }

        //一応大丈夫だが、クリックのたびに実行されないようにしておく。
        private void radioButton_Epg_Click(object sender, RoutedEventArgs e)
        {
            if (resModeProgram == true && radioButton_Epg.IsChecked == true)
            {
                ReserveModeChanged(false);
            }
        }
        private void radioButton_Program_Click(object sender, RoutedEventArgs e)
        {
            if (resModeProgram == false && radioButton_Program.IsChecked == true)
            {
                ReserveModeChanged(true);
            }
        }
        private void ReserveModeChanged(bool programMode)
        {
            SetResModeProgram(programMode);

            eventInfoNow = null;
            if (programMode == false)
            {
                var resInfo = new ReserveData();
                GetReserveTimeInfo(ref resInfo);

                //EPGデータが読込まれていない場合も考慮し、先に判定する。
                if (reserveInfo.IsEpgReserve == true && reserveInfo.IsSamePg(resInfo) == true)
                {
                    //EPG予約で、元の状態に戻る場合
                    textBox_title.Text = reserveInfo.Title;
                }
                else
                {
                    eventInfoNow = resInfo.SearchEventInfoLikeThat();
                    if (eventInfoNow == null)
                    {
                        MessageBox.Show("変更可能な番組がありません。\r\n" +
                                        "EPGの期間外か、EPGデータが読み込まれていません。");
                        SetResModeProgram(true);
                    }
                    else
                    {
                        SetReserveTimeInfo(CtrlCmdDefEx.ConvertEpgToReserveData(eventInfoNow));
                    }
                }
            }
        }

        private void tabControl_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            //ComboBoxのSelectionChangedにも反応するので。(WPFの仕様)
            if (sender != e.OriginalSource) return;

            if (tabItem_program.IsSelected)
            {
                var resInfo = new ReserveData();
                GetReserveTimeInfo(ref resInfo);

                //描画軽減。人の操作では気にするほどのことはないが、保険。
                if (resInfo.IsSamePg(resInfoDisplay) == true) return;
                resInfoDisplay = resInfo;

                EpgEventInfo eventInfo = null;
                //EPGを自動で読み込んでない時でも、元がEPG予約ならその番組情報は表示させられるようにする
                if (reserveInfo.IsEpgReserve == true && reserveInfo.IsSamePg(resInfo) == true)
                {
                    eventInfo = eventInfoNow ?? reserveInfo.SearchEventInfo(true);
                }
                else
                {
                    eventInfo = eventInfoNow ?? resInfo.SearchEventInfoLikeThat();
                }

                richTextBox_descInfo.Document = CommonManager.ConvertDisplayText(eventInfo);
            }
        }
    }
    public class ChgReserveWindowBase : ReserveWindowBase<ChgReserveWindow> { }
    public class ReserveWindowBase<T> : AttendantDataWindow<T>
    {
        protected override UserCtrlView.DataViewBase DataView { get { return ViewUtil.MainWindow.reserveView; } }
    }
}
