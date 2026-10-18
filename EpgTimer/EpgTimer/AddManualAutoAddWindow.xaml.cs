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
    /// AddManualAutoAddWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class AddManualAutoAddWindow : Window
    {
        private ManualAutoAddData defKey = null;

        public AddManualAutoAddWindow()
        {
            InitializeComponent();

            {
                comboBox_startHH.ItemsSource = Enumerable.Range(0, 24);
                comboBox_startHH.SelectedIndex = 0;
                comboBox_startMM.ItemsSource = Enumerable.Range(0, 60);
                comboBox_startMM.SelectedIndex = 0;
                comboBox_startSS.ItemsSource = Enumerable.Range(0, 60);
                comboBox_startSS.SelectedIndex = 0;
                comboBox_endHH.ItemsSource = Enumerable.Range(0, 24);
                comboBox_endHH.SelectedIndex = 0;
                comboBox_endMM.ItemsSource = Enumerable.Range(0, 60);
                comboBox_endMM.SelectedIndex = 0;
                comboBox_endSS.ItemsSource = Enumerable.Range(0, 60);
                comboBox_endSS.SelectedIndex = 0;

                comboBox_service.ItemsSource = ChSet5.Instance.ChListSelected;
                comboBox_service.SelectedIndex = 0;

                recSettingView.SetViewMode(false);
            }
        }

        private void button_add_Click(object sender, RoutedEventArgs e)
        {
            var item = new ManualAutoAddData();

            item.dayOfWeekFlag = (byte)((checkBox_week0.IsChecked == true ? 0x01 : 0) |
                                        (checkBox_week1.IsChecked == true ? 0x02 : 0) |
                                        (checkBox_week2.IsChecked == true ? 0x04 : 0) |
                                        (checkBox_week3.IsChecked == true ? 0x08 : 0) |
                                        (checkBox_week4.IsChecked == true ? 0x10 : 0) |
                                        (checkBox_week5.IsChecked == true ? 0x20 : 0) |
                                        (checkBox_week6.IsChecked == true ? 0x40 : 0));

            item.startTime = (uint)(comboBox_startHH.SelectedIndex * 60 * 60 + comboBox_startMM.SelectedIndex * 60 + comboBox_startSS.SelectedIndex);
            item.durationSecond = ((uint)(comboBox_endHH.SelectedIndex * 60 * 60 + comboBox_endMM.SelectedIndex * 60 + comboBox_endSS.SelectedIndex) +
                                   24 * 60 * 60 - item.startTime) % (24 * 60 * 60);

            item.title = textBox_title.Text;

            ChSet5Item chItem = comboBox_service.SelectedItem as ChSet5Item;
            if (chItem != null)
            {
                item.stationName = chItem.ServiceName;
                item.originalNetworkID = chItem.ONID;
                item.transportStreamID = chItem.TSID;
                item.serviceID = chItem.SID;
            }
            else if (defKey != null)
            {
                item.stationName = defKey.stationName;
                item.originalNetworkID = defKey.originalNetworkID;
                item.transportStreamID = defKey.transportStreamID;
                item.serviceID = defKey.serviceID;
            }
            else
            {
                MessageBox.Show("サービスが未選択です");
                return;
            }
            item.recSetting = recSettingView.GetRecSetting();

            if (defKey != null)
            {
                item.dataID = defKey.dataID;
                CommonManager.CreateSrvCtrl().SendChgManualAdd(new List<ManualAutoAddData>() { item });
            }
            else
            {
                CommonManager.CreateSrvCtrl().SendAddManualAdd(new List<ManualAutoAddData>() { item });
            }
            Close();
        }

        /// <summary>
        /// 自動登録情報をセットし、ウィンドウを変更モードにする
        /// </summary>
        public void SetChangeModeData(ManualAutoAddData item)
        {
            defKey = item;
            button_add.Content = "変更";

            {
                if ((defKey.dayOfWeekFlag & 0x01) != 0)
                {
                    checkBox_week0.IsChecked = true;
                }
                if ((defKey.dayOfWeekFlag & 0x02) != 0)
                {
                    checkBox_week1.IsChecked = true;
                }
                if ((defKey.dayOfWeekFlag & 0x04) != 0)
                {
                    checkBox_week2.IsChecked = true;
                }
                if ((defKey.dayOfWeekFlag & 0x08) != 0)
                {
                    checkBox_week3.IsChecked = true;
                }
                if ((defKey.dayOfWeekFlag & 0x10) != 0)
                {
                    checkBox_week4.IsChecked = true;
                }
                if ((defKey.dayOfWeekFlag & 0x20) != 0)
                {
                    checkBox_week5.IsChecked = true;
                }
                if ((defKey.dayOfWeekFlag & 0x40) != 0)
                {
                    checkBox_week6.IsChecked = true;
                }

                DateTime startTime = (new DateTime(2000, 1, 2)).AddSeconds(defKey.startTime);
                comboBox_startHH.SelectedIndex = startTime.Hour;
                comboBox_startMM.SelectedIndex = startTime.Minute;
                comboBox_startSS.SelectedIndex = startTime.Second;

                DateTime endTime = startTime.AddSeconds(defKey.durationSecond);
                comboBox_endHH.SelectedIndex = endTime.Hour;
                comboBox_endMM.SelectedIndex = endTime.Minute;
                comboBox_endSS.SelectedIndex = endTime.Second;

                textBox_title.Text = defKey.title;

                comboBox_service.SelectedItem = comboBox_service.Items.Cast<ChSet5Item>().FirstOrDefault(ch =>
                    ch.ONID == defKey.originalNetworkID &&
                    ch.TSID == defKey.transportStreamID &&
                    ch.SID == defKey.serviceID);
                defKey.recSetting.PittariFlag = 0;
                defKey.recSetting.TuijyuuFlag = 0;
                recSettingView.SetDefSetting(defKey.recSetting);
            }
        }

        private void Window_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.S:
                        // バインディング更新のためフォーカスを移す
                        button_add.Focus();
                        button_add.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        e.Handled = true;
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.Escape:
                        Close();
                        e.Handled = true;
                        break;
                }
            }
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }
    }
}
