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

using System.Text.RegularExpressions;

namespace EpgTimer
{
    /// <summary>
    /// ChgReserveWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class ChgReserveWindow : Window
    {
        private ReserveData reserveInfo = null;
        private bool manualAddMode = false;

        public ChgReserveWindow()
        {
            InitializeComponent();

            comboBox_service.ItemsSource = ChSet5.Instance.ChListSelected;
            comboBox_sh.ItemsSource = Enumerable.Range(0, 24);
            comboBox_eh.ItemsSource = Enumerable.Range(0, 24);
            comboBox_sm.ItemsSource = Enumerable.Range(0, 60);
            comboBox_em.ItemsSource = Enumerable.Range(0, 60);
            comboBox_ss.ItemsSource = Enumerable.Range(0, 60);
            comboBox_es.ItemsSource = Enumerable.Range(0, 60);

        }

        public void SetOpenMode(byte mode)
        {
            tabControl.SelectedIndex = mode;
        }

        public void AddReserveMode(bool addMode)
        {
            if (addMode == true)
            {
                checkBox_program.IsChecked = true;
                checkBox_program.IsEnabled = false;
                recSettingView.SetViewMode(false);

                if (comboBox_service.Items.Count > 0)
                {
                    comboBox_service.SelectedIndex = 0;
                }

                this.Title = "プログラム予約追加";
                checkBox_program.Visibility = System.Windows.Visibility.Hidden;

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

                button_chg_reserve.Content = "予約";

                button_del_reserve.Visibility = System.Windows.Visibility.Hidden;

                manualAddMode = true;
                reserveInfo = new ReserveData();
            }
            else
            {
                checkBox_program.IsChecked = false;

                this.Title = "予約変更";
                checkBox_program.Visibility = System.Windows.Visibility.Visible;

                button_chg_reserve.Content = "変更";

                button_del_reserve.Visibility = System.Windows.Visibility.Visible;

                manualAddMode = false;

            }
        }

        /// <summary>
        /// 初期値の予約情報をセットする
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

                        int searchFrom = 0;
                        Paragraph para = new Paragraph();
                        string rtext = CommonManager.ReplaceText(text, CommonManager.Instance.ReplaceUrlDictionary);
                        if (rtext.Length == text.Length)
                        {
                            for (Match m = Regex.Match(rtext, @"https?://[0-9A-Za-z!#$%&'()~=@;:?_+\-*/.]+"); m.Success; m = m.NextMatch())
                            {
                                para.Inlines.Add(text.Substring(searchFrom, m.Index - searchFrom));
                                Hyperlink h = new Hyperlink(new Run(text.Substring(m.Index, m.Length)));
                                h.MouseLeftButtonDown += new MouseButtonEventHandler(h_MouseLeftButtonDown);
                                h.Foreground = Brushes.Blue;
                                h.Cursor = Cursors.Hand;
                                h.NavigateUri = new Uri(m.Value);
                                para.Inlines.Add(h);
                                searchFrom = m.Index + m.Length;
                            }
                        }
                        para.Inlines.Add(text.Substring(searchFrom));
                        richTextBox_descInfo.Document = new FlowDocument(para);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        void h_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            try
            {
                if (sender.GetType() == typeof(Hyperlink))
                {
                    Hyperlink h = sender as Hyperlink;
                    System.Diagnostics.Process.Start(h.NavigateUri.ToString());
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if (manualAddMode == false)
            {
                if (reserveInfo != null)
                {
                    UpdateView();
                }
                else
                {
                    MessageBox.Show("予約が選択されていません");
                }
            }
            if (tabControl.SelectedItem != null)
            {
                ((TabItem)tabControl.SelectedItem).Focus();
            }
        }

        private void UpdateView()
        {
            try
            {
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

                foreach (ChSet5Item ch in comboBox_service.Items)
                {
                    if (ch.ONID == reserveInfo.OriginalNetworkID &&
                        ch.TSID == reserveInfo.TransportStreamID &&
                        ch.SID == reserveInfo.ServiceID)
                    {
                        comboBox_service.SelectedItem = ch;
                        break;
                    }
                }

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
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
        
        private void button_chg_reserve_Click(object sender, RoutedEventArgs e)
        {
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

                reserveInfo.Title = textBox_title.Text;
                ChSet5Item ch = comboBox_service.SelectedItem as ChSet5Item;

                reserveInfo.StationName = ch.ServiceName;
                reserveInfo.OriginalNetworkID = ch.ONID;
                reserveInfo.TransportStreamID = ch.TSID;
                reserveInfo.ServiceID = ch.SID;
                reserveInfo.EventID = 0xFFFF;

                reserveInfo.StartTime = startTime;
                TimeSpan duration = endTime - reserveInfo.StartTime;
                reserveInfo.DurationSecond = (uint)duration.TotalSeconds;
                reserveInfo.RecSetting = recSettingView.GetRecSetting();
                reserveInfo.RecSetting.TuijyuuFlag = 0;
                reserveInfo.RecSetting.PittariFlag = 0;
            }
            else
            {
                reserveInfo.RecSetting = recSettingView.GetRecSetting();
            }
            List<ReserveData> list = new List<ReserveData>();
            list.Add(reserveInfo);
            if (manualAddMode == false)
            {
                ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。");
                }
            }
            else
            {
                reserveInfo.StartTimeEpg = reserveInfo.StartTime;
                ErrCode err = CommonManager.CreateSrvCtrl().SendAddReserve(list);
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約追加でエラーが発生しました。");
                }
            }
            DialogResult = true;
        }

        private void button_del_reserve_Click(object sender, RoutedEventArgs e)
        {
            List<UInt32> list = new List<UInt32>();
            list.Add(reserveInfo.ReserveID);
            ErrCode err = CommonManager.CreateSrvCtrl().SendDelReserve(list);
            if (err != ErrCode.CMD_SUCCESS)
            {
                MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約削除でエラーが発生しました。");
            }
            DialogResult = true;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
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
