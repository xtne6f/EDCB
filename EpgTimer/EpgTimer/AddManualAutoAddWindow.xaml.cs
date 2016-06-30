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
        private bool changeModeFlag = false;
        private ManualAutoAddData defKey = null;
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;

        public AddManualAutoAddWindow()
        {
            InitializeComponent();

            try
            {
                comboBox_startHH.DataContext = CommonManager.Instance.HourDictionary.Values;
                comboBox_startHH.SelectedIndex = 0;
                comboBox_startMM.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_startMM.SelectedIndex = 0;
                comboBox_startSS.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_startSS.SelectedIndex = 0;
                comboBox_endHH.DataContext = CommonManager.Instance.HourDictionary.Values;
                comboBox_endHH.SelectedIndex = 0;
                comboBox_endMM.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_endMM.SelectedIndex = 0;
                comboBox_endSS.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_endSS.SelectedIndex = 0;

                comboBox_service.ItemsSource = ChSet5.Instance.ChList.Values;
                comboBox_service.SelectedIndex = 0;

                recSettingView.SetViewMode(false);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void SetChangeMode(bool chgFlag)
        {
            if (chgFlag == true)
            {
                this.changeModeFlag = true;
                button_add.Content = "変更";
            }
            else
            {
                this.changeModeFlag = false;
                button_add.Content = "追加";
            }
        }

        public void SetDefaultSetting(ManualAutoAddData item)
        {
            defKey = item;
        }

        private void button_add_Click(object sender, RoutedEventArgs e)
        {
            if (defKey == null)
            {
                defKey = new ManualAutoAddData();
            }
            defKey.dayOfWeekFlag = 0;
            if (checkBox_week0.IsChecked == true)
            {
                defKey.dayOfWeekFlag |= 0x01;
            }
            if (checkBox_week1.IsChecked == true)
            {
                defKey.dayOfWeekFlag |= 0x02;
            }
            if (checkBox_week2.IsChecked == true)
            {
                defKey.dayOfWeekFlag |= 0x04;
            }
            if (checkBox_week3.IsChecked == true)
            {
                defKey.dayOfWeekFlag |= 0x08;
            }
            if (checkBox_week4.IsChecked == true)
            {
                defKey.dayOfWeekFlag |= 0x10;
            }
            if (checkBox_week5.IsChecked == true)
            {
                defKey.dayOfWeekFlag |= 0x20;
            }
            if (checkBox_week6.IsChecked == true)
            {
                defKey.dayOfWeekFlag |= 0x40;
            }

            defKey.startTime = ((UInt32)comboBox_startHH.SelectedIndex * 60 * 60) + ((UInt32)comboBox_startMM.SelectedIndex * 60) + (UInt32)comboBox_startSS.SelectedIndex;
            UInt32 endTime = ((UInt32)comboBox_endHH.SelectedIndex * 60 * 60) + ((UInt32)comboBox_endMM.SelectedIndex * 60) + (UInt32)comboBox_endSS.SelectedIndex;
            if (endTime < defKey.startTime)
            {
                defKey.durationSecond = (24 * 60 * 60 + endTime) - defKey.startTime;
            }
            else
            {
                defKey.durationSecond = endTime - defKey.startTime;
            }

            defKey.title = textBox_title.Text;

            ChSet5Item chItem = comboBox_service.SelectedItem as ChSet5Item;
            defKey.stationName = chItem.ServiceName;
            defKey.originalNetworkID = chItem.ONID;
            defKey.transportStreamID = chItem.TSID;
            defKey.serviceID = chItem.SID;

            RecSettingData recSet = new RecSettingData();
            recSettingView.GetRecSetting(ref recSet);
            defKey.recSetting = recSet;

            List<ManualAutoAddData> val = new List<ManualAutoAddData>();
            val.Add(defKey);

            if (changeModeFlag == true)
            {
                cmd.SendChgManualAdd(val);
            }
            else
            {
                cmd.SendAddManualAdd(val);
            }
            DialogResult = true;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if (defKey != null)
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

                UInt32 hh = defKey.startTime / (60 * 60);
                UInt32 mm = (defKey.startTime % (60 * 60)) / 60;
                UInt32 ss = defKey.startTime % 60;

                comboBox_startHH.SelectedIndex = (int)hh;
                comboBox_startMM.SelectedIndex = (int)mm;
                comboBox_startSS.SelectedIndex = (int)ss;

                UInt32 endTime = defKey.startTime + defKey.durationSecond;
                if (endTime > 24 * 60 * 60)
                {
                    endTime -= 24 * 60 * 60;
                }
                hh = endTime / (60 * 60);
                mm = (endTime % (60 * 60)) / 60;
                ss = endTime % 60;

                comboBox_endHH.SelectedIndex = (int)hh;
                comboBox_endMM.SelectedIndex = (int)mm;
                comboBox_endSS.SelectedIndex = (int)ss;

                textBox_title.Text = defKey.title;

                UInt64 key = ((UInt64)defKey.originalNetworkID) << 32 |
                    ((UInt64)defKey.transportStreamID) << 16 |
                    ((UInt64)defKey.serviceID);

                if (ChSet5.Instance.ChList.ContainsKey(key) == true)
                {
                    comboBox_service.SelectedItem = ChSet5.Instance.ChList[key];
                }
                defKey.recSetting.PittariFlag = 0;
                defKey.recSetting.TuijyuuFlag = 0;
                recSettingView.SetDefSetting(defKey.recSetting);
            }
        }
    }
}
