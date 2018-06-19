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
    /// AddReserveEpgWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class AddReserveEpgWindow : Window
    {
        private EpgEventInfo eventInfo = null;

        public AddReserveEpgWindow()
        {
            InitializeComponent();
        }

        public void SetOpenMode(byte mode)
        {
            tabControl.SelectedIndex = mode == 0 && tabItem_reserve.IsEnabled ? 0 : 1;
        }

        public void SetReservable(bool reservable)
        {
            tabItem_reserve.IsEnabled = reservable;
            button_add_reserve.IsEnabled = reservable;
            SetOpenMode((byte)tabControl.SelectedIndex);
        }

        public void SetEventInfo(EpgEventInfo eventData)
        {
            try
            {
                eventInfo = eventData;
                textBox_info.Text = CommonManager.Instance.ConvertProgramText(eventData, EventInfoTextMode.BasicOnly);
                String text = CommonManager.Instance.ConvertProgramText(eventData, EventInfoTextMode.ExtOnly);

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

        private void button_add_reserve_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (eventInfo.StartTimeFlag == 0)
                {
                    MessageBox.Show("開始時間未定のため予約できません");
                    if (this.Visibility == System.Windows.Visibility.Visible)
                    {
                        DialogResult = false;
                    }
                }

                ReserveData reserveInfo = new ReserveData();
                if (eventInfo.ShortInfo != null)
                {
                    reserveInfo.Title = eventInfo.ShortInfo.event_name;
                }

                reserveInfo.StartTime = eventInfo.start_time;
                reserveInfo.StartTimeEpg = eventInfo.start_time;

                if (eventInfo.DurationFlag == 0)
                {
                    reserveInfo.DurationSecond = 10 * 60;
                }
                else
                {
                    reserveInfo.DurationSecond = eventInfo.durationSec;
                }

                UInt64 key = CommonManager.Create64Key(eventInfo.original_network_id, eventInfo.transport_stream_id, eventInfo.service_id);
                if (ChSet5.Instance.ChList.ContainsKey(key) == true)
                {
                    reserveInfo.StationName = ChSet5.Instance.ChList[key].ServiceName;
                }
                reserveInfo.OriginalNetworkID = eventInfo.original_network_id;
                reserveInfo.TransportStreamID = eventInfo.transport_stream_id;
                reserveInfo.ServiceID = eventInfo.service_id;
                reserveInfo.EventID = eventInfo.event_id;

                RecSettingData setInfo = new RecSettingData();
                recSettingView.GetRecSetting(ref setInfo);
                reserveInfo.RecSetting = setInfo;

                List<ReserveData> list = new List<ReserveData>();
                list.Add(reserveInfo);
                ErrCode err = CommonManager.CreateSrvCtrl().SendAddReserve(list);
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約登録でエラーが発生しました。終了時間がすでに過ぎている可能性があります。");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            if (this.Visibility == System.Windows.Visibility.Visible)
            {
                DialogResult = true;
            }
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            base.OnKeyDown(e);
            //
            if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control) && Keyboard.Modifiers.HasFlag(ModifierKeys.Shift))
            {
                switch (e.Key)
                {
                    case Key.A:
                        this.button_add_reserve.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                }
            }
            else
            {
                switch (e.Key)
                {
                    case Key.Escape:
                        this.button_cancel.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                }
            }
        }
    }
}
