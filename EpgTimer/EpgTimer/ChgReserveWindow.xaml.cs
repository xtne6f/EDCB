using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    /// <summary>
    /// ChgReserveWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class ChgReserveWindow : Window
    {
        private ReserveData reserveInfo = null;
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private bool manualAddMode = false;
        private byte openMode = 0;

        private bool resModeProgram = true;

        private ReserveData resInfoDisplay = null;
        private EpgEventInfo eventInfoDisplay = null;
        private EpgEventInfo eventInfoNew = null;
        private EpgEventInfo eventInfoSelected = null;

        public ChgReserveWindow()
        {
            InitializeComponent();

            if (Settings.Instance.NoStyle == 0)
            {
                ResourceDictionary rd = new ResourceDictionary();
                rd.MergedDictionaries.Add(
                    Application.LoadComponent(new Uri("/PresentationFramework.Aero, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/aero.normalcolor.xaml", UriKind.Relative)) as ResourceDictionary
                    //Application.LoadComponent(new Uri("/PresentationFramework.Classic, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35, ProcessorArchitecture=MSIL;component/themes/Classic.xaml", UriKind.Relative)) as ResourceDictionary
                    );
                this.Resources = rd;
            }
            else
            {
                button_chg_reserve.Style = null;
                button_del_reserve.Style = null;
                button_cancel.Style = null;
            }

            comboBox_service.ItemsSource = ChSet5.Instance.ChList.Values;
            comboBox_sh.ItemsSource = CommonManager.Instance.HourDictionary.Values;
            comboBox_eh.ItemsSource = CommonManager.Instance.HourDictionary.Values;
            comboBox_sm.ItemsSource = CommonManager.Instance.MinDictionary.Values;
            comboBox_em.ItemsSource = CommonManager.Instance.MinDictionary.Values;
            comboBox_ss.ItemsSource = CommonManager.Instance.MinDictionary.Values;
            comboBox_es.ItemsSource = CommonManager.Instance.MinDictionary.Values;

        }

        public void SetOpenMode(byte mode)
        {
            openMode = mode;
        }

        public void SetAddReserveMode()
        {
            manualAddMode = true;
        }

        /// <summary>
        /// 初期値の予約情報をセットする
        /// </summary>
        /// <param name="info">[IN] 初期値の予約情報</param>
        public void SetReserveInfo(ReserveData info)
        {
            reserveInfo = info;
            recSettingView.SetDefSetting(info.RecSetting);
        }

        private void SetResModeProgram(bool mode)
        {
            resModeProgram = mode;

            radioButton_Epg.IsChecked = !resModeProgram;
            radioButton_Program.IsChecked = resModeProgram;

            textBox_title.IsEnabled = resModeProgram;
            comboBox_service.IsEnabled = resModeProgram;
            datePicker_start.IsEnabled = resModeProgram;
            comboBox_sh.IsEnabled = resModeProgram;
            comboBox_sm.IsEnabled = resModeProgram;
            comboBox_ss.IsEnabled = resModeProgram;
            datePicker_end.IsEnabled = resModeProgram;
            comboBox_eh.IsEnabled = resModeProgram;
            comboBox_em.IsEnabled = resModeProgram;
            comboBox_es.IsEnabled = resModeProgram;
            recSettingView.SetViewMode(!resModeProgram);
        }

        //問題は起きないはずだが、ロード時に何度もSelectionChangedが呼び出されるので一応制御を入れておく
        //SelectedIndexChanged使えないい？
        int TabSelectedIndex = -1;

        private void tabControl_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (tabControl.SelectedIndex == TabSelectedIndex) return;
            TabSelectedIndex = tabControl.SelectedIndex;

            if (tabItem_program.IsSelected)
            {
                if (resModeProgram == true)
                {
                    var resInfo = new ReserveData();
                    GetReserveTimeInfo(ref resInfo);

                    //描画回数の削減を気にしないなら、この条件文は無くてもいい
                    if (CommonManager.EqualsPg(resInfoDisplay, resInfo, false, true) == false)
                    {
                        //EPGを自動で読み込んでない時でも、元がEPG予約ならその番組情報は表示させられるようにする
                        if (reserveInfo.EventID != 0xFFFF && CommonManager.EqualsPg(reserveInfo, resInfo, false, true) == true)
                        {
                            SetProgramContent(CommonManager.Instance.GetEpgEventInfoFromReserveData(reserveInfo, true));
                        }
                        else
                        {
                            SetProgramContent(SearchEventLikeThat(resInfo));
                        }

                        resInfoDisplay = resInfo;
                    }
                }
                else
                {
                    //EPG予約を変更していない場合引っかかる。
                    //最も表示される可能性が高いので、何度も探しにいかせないようにする。
                    if (eventInfoSelected == null)
                    {
                        eventInfoSelected = CommonManager.Instance.GetEpgEventInfoFromReserveData(reserveInfo, true);
                    }
                    SetProgramContent(eventInfoSelected);
                    resInfoDisplay = null;
                }
            }
        }

        private void SetProgramContent(EpgEventInfo info)
        {
            //放映時刻情報に対してEPGデータ無い場合もあるので、resInfoDisplayとは別にeventInfoDisplayを管理する
            if (CommonManager.EqualsPg(eventInfoDisplay, info) == false)
            {
                richTextBox_descInfo.Document = CommonManager.Instance.ConvertDisplayText(info);
            }
            eventInfoDisplay = info;
        }

        private void ResetProgramContent()
        {
            richTextBox_descInfo.Document = CommonManager.Instance.ConvertDisplayText(null);
            eventInfoDisplay = null;
        }

        private EpgEventInfo SearchEventLikeThat(ReserveData resInfo)
        {
            double dist = double.MaxValue, dist1;
            EpgEventInfo eventPossible = null;

            UInt64 key = CommonManager.Create64Key(resInfo.OriginalNetworkID, resInfo.TransportStreamID, resInfo.ServiceID);
            if (CommonManager.Instance.DB.ServiceEventList.ContainsKey(key) == true)
            {
                foreach (EpgEventInfo eventChkInfo in CommonManager.Instance.DB.ServiceEventList[key].eventList)
                {
                    dist1 = Math.Abs((resInfo.StartTime - eventChkInfo.start_time).TotalSeconds);
                    double overlapLength = CulcOverlapLength(resInfo.StartTime, resInfo.DurationSecond,
                                                            eventChkInfo.start_time, eventChkInfo.durationSec);

                    //開始時間が最も近いものを選ぶ。同じ差なら時間が前のものを選ぶ
                    if (overlapLength >= 0 && (dist > dist1 ||
                        dist == dist1 && (eventPossible == null || resInfo.StartTime > eventChkInfo.start_time)))
                     {
                        dist = dist1;
                        eventPossible = eventChkInfo;
                        if (dist == 0) break;
                    }
                }
            }

            return eventPossible;
        }

        private double CulcOverlapLength(DateTime s1, uint d1, DateTime s2, uint d2)
        {
            //重複してない場合は負数が返る。
            TimeSpan ts1 = s1 + TimeSpan.FromSeconds(d1) - s2;
            TimeSpan ts2 = s2 + TimeSpan.FromSeconds(d2) - s1;
            return Math.Min(Math.Min(ts1.TotalSeconds, ts2.TotalSeconds), Math.Min(d1, d2));
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if (manualAddMode == true || reserveInfo == null)
            {
                manualAddMode = true;
                SetResModeProgram(true);

                if (comboBox_service.Items.Count > 0)
                {
                    comboBox_service.SelectedIndex = 0;
                }

                this.Title = "予約登録";
                button_chg_reserve.Content = "予約";
                button_del_reserve.Visibility = System.Windows.Visibility.Hidden;
                SetReserveTime(DateTime.Now.AddMinutes(1), DateTime.Now.AddMinutes(31));
                reserveInfo = new ReserveData();
            }
            else
            {
                SetResModeProgram(reserveInfo.EventID == 0xFFFF);
                SetReserveTimeInfo(reserveInfo);
            }

            ResetProgramContent();                  //番組詳細を初期表示
            tabControl.SelectedIndex = openMode;
        }

        private void SetReserveTime(DateTime startTime, DateTime endTime)
        {
            datePicker_start.SelectedDate = startTime;
            comboBox_sh.SelectedIndex = startTime.Hour;
            comboBox_sm.SelectedIndex = startTime.Minute;
            comboBox_ss.SelectedIndex = 0;

            datePicker_end.SelectedDate = endTime;
            comboBox_eh.SelectedIndex = endTime.Hour;
            comboBox_em.SelectedIndex = endTime.Minute;
            comboBox_es.SelectedIndex = 0;
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
                //resInfo.EventID = 0xFFFF;　条件付の情報なのでここでは書き換えないことにする

                resInfo.StartTime = new DateTime(datePicker_start.SelectedDate.Value.Year,
                    datePicker_start.SelectedDate.Value.Month,
                    datePicker_start.SelectedDate.Value.Day,
                    comboBox_sh.SelectedIndex,
                    comboBox_sm.SelectedIndex,
                    comboBox_ss.SelectedIndex,
                    0,
                    DateTimeKind.Utc
                    );

                DateTime endTime = new DateTime(datePicker_end.SelectedDate.Value.Year,
                    datePicker_end.SelectedDate.Value.Month,
                    datePicker_end.SelectedDate.Value.Day,
                    comboBox_eh.SelectedIndex,
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
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return -1;
        }

        private void SetReserveTimeInfo(ReserveData resInfo)
        {
            if (resInfo == null) return;

            try
            {
                textBox_title.Text = resInfo.Title;

                foreach (ChSet5Item ch in comboBox_service.Items)
                {
                    if (ch.ONID == resInfo.OriginalNetworkID &&
                        ch.TSID == resInfo.TransportStreamID &&
                        ch.SID == resInfo.ServiceID)
                    {
                        comboBox_service.SelectedItem = ch;
                        break;
                    }
                }

                SetReserveTime(resInfo.StartTime,resInfo.StartTime.AddSeconds(resInfo.DurationSecond));
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private bool CheckExistReserveItem()
        {
            bool retval = CommonManager.Instance.DB.ReserveList.ContainsKey(this.reserveInfo.ReserveID);
            if (retval == false)
            {
                MessageBox.Show("項目がありません。\r\n" +
                    "既に削除されています。\r\n" +
                    "(別のEpgtimerによる操作など)");

                //予約復旧を提示。これだけで大丈夫だったりする。
                manualAddMode = true;
                button_chg_reserve.Content = "再予約";
                this.button_del_reserve.IsEnabled = false;
            }
            return retval;
        }

        private void button_chg_reserve_Click(object sender, RoutedEventArgs e)
        {
            if (manualAddMode == false && CheckExistReserveItem() == false) return;

            try
            {
                var setInfo = new RecSettingData();
                recSettingView.GetRecSetting(ref setInfo);

                //ダイアログを閉じないときはreserveInfoを変更しないよう注意する
                if (resModeProgram == true)
                {
                    var resInfo = new ReserveData();
                    if (GetReserveTimeInfo(ref resInfo) == -2)
                    {
                        MessageBox.Show("終了日時が開始日時より前です");
                        return;
                    }

                    GetReserveTimeInfo(ref reserveInfo);
                    if (reserveInfo.EventID != 0xFFFF)
                    {
                        reserveInfo.EventID = 0xFFFF;
                        reserveInfo.Comment = "";
                    }
                    reserveInfo.StartTimeEpg = reserveInfo.StartTime;

                    setInfo.TuijyuuFlag = 0;
                    setInfo.PittariFlag = 0;
                }
                else
                {
                    //EPG予約に変える場合、またはEPG予約で別の番組に変わる場合
                    if (eventInfoNew != null)
                    {
                        //基本的にAddReserveEpgWindowと同じ処理内容
                        if (CommonManager.Instance.MUtil.IsEnableReserveAdd(eventInfoNew) == false) return;
                        CommonManager.ConvertEpgToReserveData(eventInfoNew, ref reserveInfo);
                        reserveInfo.Comment = "";
                    }
                }

                reserveInfo.RecSetting = setInfo;

                if (manualAddMode == false)
                {
                    CommonManager.Instance.MUtil.ReserveChange(reserveInfo);
                }
                else
                {
                    CommonManager.Instance.MUtil.ReserveAdd(reserveInfo);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }

            DialogResult = true;
        }

        private void button_del_reserve_Click(object sender, RoutedEventArgs e)
        {
            if (CheckExistReserveItem() == false) return;

            CommonManager.Instance.MUtil.ReserveDelete(reserveInfo);

            DialogResult = true;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }

        //一応大丈夫だが、クリックのたびに実行されないようにしておく。
        private void radioButton_Epg_Click(object sender, RoutedEventArgs e)
        {
            if (resModeProgram == true && radioButton_Epg.IsChecked == true)
            {
                SetResModeProgram(false);
                ReserveModeChanged();
            }
        }

        private void radioButton_Program_Click(object sender, RoutedEventArgs e)
        {
            if (resModeProgram == false && radioButton_Program.IsChecked == true)
            {
                SetResModeProgram(true);
                ReserveModeChanged();
            }
        }

        private void ReserveModeChanged()
        {
            if (resModeProgram == true)
            {
                eventInfoNew = null;
            }
            else
            {
                var resInfo = new ReserveData();
                GetReserveTimeInfo(ref resInfo);

                if (reserveInfo.EventID != 0xFFFF && CommonManager.EqualsPg(reserveInfo, resInfo, false, true) == true)
                {
                    //EPG予約で、元の状態に戻る場合
                    textBox_title.Text = reserveInfo.Title;
                    eventInfoNew = null;
                }
                else
                {
                    eventInfoNew = SearchEventLikeThat(resInfo);
                    if (eventInfoNew == null)
                    {
                        MessageBox.Show("変更可能な番組がありません。\r\n" +
                                        "EPGの期間外か、EPGデータが読み込まれていません。");
                        SetResModeProgram(true);
                    }
                    else
                    {
                        SetReserveTimeInfo(CommonManager.ConvertEpgToReserveData(eventInfoNew));
                    }
                }
            }

            eventInfoSelected = eventInfoNew;
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            base.OnKeyDown(e);
            //
            if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control) && Keyboard.Modifiers.HasFlag(ModifierKeys.Shift))
            {
                switch (e.Key)
                {
                    case Key.C:
                        string captionText = button_chg_reserve.Content.ToString();
                        string msgText = button_chg_reserve.Content.ToString();
                        switch (captionText)
                        {
                            case "変更":
                                msgText = "この予約を変更します。";
                                break;
                            case "予約":
                                msgText = "予約を追加します。";
                                break;
                            case "再予約":
                                msgText = "この内容で再予約します。";
                                break;
                        }
                        if (MessageBox.Show(msgText + "\r\nよろしいですか？", captionText + "の確認", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
                        {
                            this.button_chg_reserve.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        }
                        break;
                    case Key.D:
                        if (this.button_del_reserve.Visibility == System.Windows.Visibility.Visible && this.button_del_reserve.IsEnabled == true)
                        {
                            if (MessageBox.Show("この予約を削除します。\r\nよろしいですか？", "削除の確認", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
                            {
                                this.button_del_reserve.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                            }
                        }
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.Escape:
                        this.button_cancel.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                }
            }
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            MainWindow mainWindow = Application.Current.MainWindow as MainWindow;
            mainWindow.ListFoucsOnVisibleChanged();
        }
    }
}
