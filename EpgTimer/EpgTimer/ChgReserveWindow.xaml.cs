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
using System.Windows.Shapes;

namespace EpgTimer
{
    /// <summary>
    /// ChgReserveWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class ChgReserveWindow : Window
    {
        private ReserveData reserveInfo = null;

        public ChgReserveWindow()
        {
            InitializeComponent();

            comboBox_service.ItemsSource = ChSet5.Instance.ChListSelected;
            comboBox_service.SelectedIndex = 0;
            comboBox_sh.ItemsSource = Enumerable.Range(0, 24);
            comboBox_eh.ItemsSource = Enumerable.Range(0, 24);
            comboBox_sm.ItemsSource = Enumerable.Range(0, 60);
            comboBox_em.ItemsSource = Enumerable.Range(0, 60);
            comboBox_ss.ItemsSource = Enumerable.Range(0, 60);
            comboBox_es.ItemsSource = Enumerable.Range(0, 60);

            DateTime startTime = DateTime.UtcNow.AddHours(9).AddMinutes(1);
            datePicker_start.SelectedDate = startTime;
            comboBox_sh.SelectedIndex = startTime.Hour;
            comboBox_sm.SelectedIndex = startTime.Minute;
            comboBox_ss.SelectedIndex = 0;
            DateTime endTime = startTime.AddMinutes(30);
            datePicker_end.SelectedDate = endTime;
            comboBox_eh.SelectedIndex = endTime.Hour;
            comboBox_em.SelectedIndex = endTime.Minute;
            comboBox_es.SelectedIndex = 0;

            recSettingView.SetViewMode(false);
        }

        public void SetOpenMode(byte mode)
        {
            tabControl.SelectedIndex = mode;
        }

        /// <summary>
        /// 初期値の予約情報をセットし、ウィンドウを予約変更モードにする
        /// </summary>
        /// <param name="info">[IN] 初期値の予約情報</param>
        public void SetReserveInfo(ReserveData info)
        {
            try
            {
                reserveInfo = info;
                recSettingView.SetDefSetting(info.RecSetting);

                if (info.EventID != 0xFFFF)
                {
                    EpgEventInfo eventInfo = CommonManager.Instance.DB.GetPgInfo(info.OriginalNetworkID, info.TransportStreamID,
                                                                                 info.ServiceID, info.EventID, false);
                    if (eventInfo != null)
                    {
                        String text = CommonManager.Instance.ConvertProgramText(eventInfo, EventInfoTextMode.All);
                        richTextBox_descInfo.Document = new FlowDocument(CommonManager.ConvertDisplayText(text));
                    }
                }

                Title = "予約変更";
                button_chg_reserve.Content = "変更";
                button_del_reserve.Visibility = Visibility.Visible;
                if (reserveInfo.EventID == 0xFFFF)
                {
                    checkBox_program.IsChecked = true;
                    checkBox_program.IsEnabled = false;
                    recSettingView.SetViewMode(false);
                }
                else
                {
                    checkBox_program.IsChecked = false;
                    checkBox_program.IsEnabled = true;
                    recSettingView.SetViewMode(true);
                }

                textBox_title.Text = reserveInfo.Title;

                comboBox_service.SelectedItem = comboBox_service.Items.Cast<ChSet5Item>().FirstOrDefault(ch =>
                    ch.ONID == reserveInfo.OriginalNetworkID &&
                    ch.TSID == reserveInfo.TransportStreamID &&
                    ch.SID == reserveInfo.ServiceID);

                DateTime startTime = reserveInfo.StartTime;
                DateTime endTime = startTime.AddSeconds(reserveInfo.DurationSecond);
                datePicker_start.SelectedDate = startTime;
                datePicker_end.SelectedDate = endTime;
                comboBox_sh.SelectedIndex = startTime.Hour;
                comboBox_sm.SelectedIndex = startTime.Minute;
                comboBox_ss.SelectedIndex = startTime.Second;
                comboBox_eh.SelectedIndex = endTime.Hour;
                comboBox_em.SelectedIndex = endTime.Minute;
                comboBox_es.SelectedIndex = endTime.Second;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if (tabControl.SelectedItem != null)
            {
                ((TabItem)tabControl.SelectedItem).Focus();
            }
        }

        private void button_chg_reserve_Click(object sender, RoutedEventArgs e)
        {
            var info = new ReserveData();
            if (reserveInfo != null)
            {
                // 初期値をコピー(予約変更で無視されるフィールドは省略)
                info.Title = reserveInfo.Title;
                info.StartTime = reserveInfo.StartTime;
                info.DurationSecond = reserveInfo.DurationSecond;
                info.StationName = reserveInfo.StationName;
                info.OriginalNetworkID = reserveInfo.OriginalNetworkID;
                info.TransportStreamID = reserveInfo.TransportStreamID;
                info.ServiceID = reserveInfo.ServiceID;
                info.EventID = reserveInfo.EventID;
                info.Comment = reserveInfo.Comment;
                info.ReserveID = reserveInfo.ReserveID;
                info.OverlapMode = reserveInfo.OverlapMode;
                info.StartTimeEpg = reserveInfo.StartTimeEpg;
            }
            info.RecSetting = recSettingView.GetRecSetting();

            if (checkBox_program.IsChecked == true)
            {
                var startTime = DateTime.MaxValue;
                var endTime = DateTime.MinValue;
                if (datePicker_start.SelectedDate != null && datePicker_end.SelectedDate != null)
                {
                    startTime = new DateTime(datePicker_start.SelectedDate.Value.Year,
                                             datePicker_start.SelectedDate.Value.Month,
                                             datePicker_start.SelectedDate.Value.Day,
                                             comboBox_sh.SelectedIndex,
                                             comboBox_sm.SelectedIndex,
                                             comboBox_ss.SelectedIndex, 0, DateTimeKind.Utc);
                    endTime = new DateTime(datePicker_end.SelectedDate.Value.Year,
                                           datePicker_end.SelectedDate.Value.Month,
                                           datePicker_end.SelectedDate.Value.Day,
                                           comboBox_eh.SelectedIndex,
                                           comboBox_em.SelectedIndex,
                                           comboBox_es.SelectedIndex, 0, DateTimeKind.Utc);
                }
                if (startTime > endTime)
                {
                    MessageBox.Show("終了日時が開始日時より前です");
                    return;
                }

                info.Title = textBox_title.Text;
                ChSet5Item ch = comboBox_service.SelectedItem as ChSet5Item;
                if (ch != null)
                {
                    info.StationName = ch.ServiceName;
                    info.OriginalNetworkID = ch.ONID;
                    info.TransportStreamID = ch.TSID;
                    info.ServiceID = ch.SID;
                }
                else if (reserveInfo == null)
                {
                    MessageBox.Show("サービスが未選択です");
                    return;
                }
                info.EventID = 0xFFFF;

                info.StartTime = startTime;
                info.DurationSecond = (uint)(endTime - startTime).TotalSeconds;
                info.RecSetting.TuijyuuFlag = 0;
                info.RecSetting.PittariFlag = 0;
            }

            if (reserveInfo != null)
            {
                ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(new List<ReserveData>() { info });
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。");
                }
            }
            else
            {
                info.StartTimeEpg = info.StartTime;
                ErrCode err = CommonManager.CreateSrvCtrl().SendAddReserve(new List<ReserveData>() { info });
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約追加でエラーが発生しました。");
                }
            }
            DialogResult = true;
        }

        private void button_del_reserve_Click(object sender, RoutedEventArgs e)
        {
            if (reserveInfo != null)
            {
                ErrCode err = CommonManager.CreateSrvCtrl().SendDelReserve(new List<uint>() { reserveInfo.ReserveID });
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約削除でエラーが発生しました。");
                }
                DialogResult = true;
            }
        }

        private void checkBox_program_Click(object sender, RoutedEventArgs e)
        {
            recSettingView.SetViewMode(checkBox_program.IsChecked == false);
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
                        this.button_chg_reserve.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.D:
                        this.button_del_reserve.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                }
            }
        }

    }
}
