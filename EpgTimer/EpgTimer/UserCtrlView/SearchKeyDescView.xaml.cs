using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

namespace EpgTimer
{
    /// <summary>
    /// SearchKeyDescView.xaml の相互作用ロジック
    /// </summary>
    public partial class SearchKeyDescView : UserControl
    {
        private EpgSearchKeyInfo defKey = new EpgSearchKeyInfo();
        private List<ServiceItem> serviceList = new List<ServiceItem>();
        private Dictionary<Int64, ServiceItem> serviceDict = new Dictionary<long, ServiceItem>();

        public SearchKeyDescView()
        {
            InitializeComponent();
            try
            {
                foreach (ChSet5Item info in ChSet5.Instance.ChList.Values)
                {
                    ServiceItem item = new ServiceItem();

                    item.ServiceInfo = CommonManager.ConvertChSet5To(info);
                    serviceList.Add(item);
                    serviceDict.Add((Int64)item.ID, item);
                }
                listView_service.ItemsSource = serviceList;

                comboBox_content.DataContext = CommonManager.Instance.ContentKindList;
                comboBox_content.SelectedIndex = 0;

                comboBox_time_sw.DataContext = CommonManager.Instance.DayOfWeekDictionary.Values;
                comboBox_time_sw.SelectedIndex = 0;
                comboBox_time_sh.DataContext = CommonManager.Instance.HourDictionary.Values;
                comboBox_time_sh.SelectedIndex = 0;
                comboBox_time_sm.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_time_sm.SelectedIndex = 0;
                comboBox_time_ew.DataContext = CommonManager.Instance.DayOfWeekDictionary.Values;
                comboBox_time_ew.SelectedIndex = 6;
                comboBox_time_eh.DataContext = CommonManager.Instance.HourDictionary.Values;
                comboBox_time_eh.SelectedIndex = 23;
                comboBox_time_em.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_time_em.SelectedIndex = 59;
                comboBox_week_sh.DataContext = CommonManager.Instance.HourDictionarySelect.Values;
                comboBox_week_sh.SelectedIndex = 0;
                comboBox_week_sm.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_week_sm.SelectedIndex = 0;
                comboBox_week_eh.DataContext = CommonManager.Instance.HourDictionarySelect.Values;
                comboBox_week_eh.SelectedIndex = 23;
                comboBox_week_em.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_week_em.SelectedIndex = 59;

                var bxc = new BoxExchangeEdit.BoxExchangeEditor(null, listBox_content, true, true, true);
                button_content_clear.Click += new RoutedEventHandler(bxc.button_DeleteAll_Click);
                button_content_del.Click += new RoutedEventHandler(bxc.button_Delete_Click);

                var bxd = new BoxExchangeEdit.BoxExchangeEditor(null, listBox_date, true, true, true);
                button_date_clear.Click += new RoutedEventHandler(bxd.button_DeleteAll_Click);
                button_date_del.Click += new RoutedEventHandler(bxd.button_Delete_Click);

            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public void SetChangeMode(int chgMode)
        {
            CommonManager.Instance.VUtil.SetSpecificChgAppearance(listBox_content);
            listBox_content.Focus();
            if (listBox_content.Items.Count != 0) listBox_content.SelectedIndex = 0;
        }

        public void SetSearchKey(EpgSearchKeyInfo key)
        {
            defKey = key.Clone();
            UpdateView();
        }

        public void GetSearchKey(ref EpgSearchKeyInfo key)
        {
            try
            {
                key.regExpFlag = (byte)(checkBox_regExp.IsChecked == true ? 1 : 0);
                key.aimaiFlag = (byte)(checkBox_aimai.IsChecked == true ? 1 : 0);
                key.titleOnlyFlag = (byte)(checkBox_titleOnly.IsChecked == true ? 1 : 0);
                key.caseFlag = (byte)(checkBox_case.IsChecked == true ? 1 : 0);
                key.keyDisabledFlag = (byte)(checkBox_keyDisabled.IsChecked == true ? 1 : 0);

                key.contentList.Clear();
                foreach (ContentKindInfo info in listBox_content.Items)
                {
                    EpgContentData item = new EpgContentData();
                    item.content_nibble_level_1 = info.Nibble1;
                    item.content_nibble_level_2 = info.Nibble2;
                    key.contentList.Add(item);
                }
                key.notContetFlag = (byte)(checkBox_notContent.IsChecked == true ? 1 : 0);

                key.serviceList.Clear();
                foreach (ServiceItem info in listView_service.Items)
                {
                    if (info.IsSelected == true)
                    {
                        key.serviceList.Add((Int64)info.ID);
                    }
                }

                key.dateList.Clear();
                foreach (DateItem info in listBox_date.Items)
                {
                    key.dateList.Add(info.DateInfo);
                }
                key.notDateFlag = (byte)(checkBox_notDate.IsChecked == true ? 1 : 0);

                if (radioButton_free_2.IsChecked == true)
                {
                    //無料
                    key.freeCAFlag = 1;
                }
                else if (radioButton_free_3.IsChecked == true)
                {
                    //有料
                    key.freeCAFlag = 2;
                }
                else
                {
                    key.freeCAFlag = 0;
                }

                var mutil = CommonManager.Instance.MUtil;

                key.chkRecEnd = (byte)(checkBox_chkRecEnd.IsChecked == true ? 1 : 0);
                key.chkRecDay = mutil.MyToNumerical(textBox_chkRecDay, Convert.ToUInt16, ushort.MinValue);
                key.chkRecNoService = (byte)(radioButton_chkRecNoService2.IsChecked == true ? 1 : 0);
                key.chkDurationMin = mutil.MyToNumerical(textBox_chkDurationMin, Convert.ToUInt16, ushort.MinValue);
                key.chkDurationMax = mutil.MyToNumerical(textBox_chkDurationMax, Convert.ToUInt16, ushort.MinValue);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void UpdateView()
        {
            try
            {
                checkBox_regExp.IsChecked = (defKey.regExpFlag == 1);
                checkBox_aimai.IsChecked = (defKey.aimaiFlag == 1);
                checkBox_titleOnly.IsChecked = (defKey.titleOnlyFlag == 1);
                checkBox_case.IsChecked = (defKey.caseFlag == 1);
                checkBox_keyDisabled.IsChecked = (defKey.keyDisabledFlag == 1);

                listBox_content.Items.Clear();
                foreach (EpgContentData item in defKey.contentList)
                {
                    int nibble1 = item.content_nibble_level_1;
                    int nibble2 = item.content_nibble_level_2;
                    UInt16 contentKey = (UInt16)(nibble1 << 8 | nibble2);
                    if (CommonManager.Instance.ContentKindDictionary.ContainsKey(contentKey) == true)
                    {
                        listBox_content.Items.Add(CommonManager.Instance.ContentKindDictionary[contentKey]);
                    }
                }

                foreach (ServiceItem info in serviceDict.Values)
                {
                    info.IsSelected = false;
                }
                foreach (Int64 info in defKey.serviceList)
                {
                    if (serviceDict.ContainsKey(info) == true)
                    {
                        serviceDict[info].IsSelected = true;
                    }
                }

                listBox_date.Items.Clear();
                foreach (EpgSearchDateInfo info in defKey.dateList)
                {
                    listBox_date.Items.Add(new DateItem(info));
                }

                checkBox_notContent.IsChecked = (defKey.notContetFlag == 1);
                checkBox_notDate.IsChecked = (defKey.notDateFlag == 1);

                switch (defKey.freeCAFlag)
                {
                    case 1:
                        radioButton_free_2.IsChecked = true;
                        break;
                    case 2:
                        radioButton_free_3.IsChecked = true;
                        break;
                    default:
                        radioButton_free_1.IsChecked = true;
                        break;
                }

                checkBox_chkRecEnd.IsChecked = (defKey.chkRecEnd == 1);
                textBox_chkRecDay.Text = defKey.chkRecDay.ToString();
                radioButton_chkRecNoService1.IsChecked = (defKey.chkRecNoService == 0);
                radioButton_chkRecNoService2.IsChecked = (defKey.chkRecNoService != 0);

                textBox_chkDurationMin.Text = defKey.chkDurationMin.ToString();
                textBox_chkDurationMax.Text = defKey.chkDurationMax.ToString();   
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_content_add_Click(object sender, RoutedEventArgs e)
        {
            if (comboBox_content.SelectedItem != null)
            {
                foreach (ContentKindInfo info in listBox_content.Items)
                {
                    ContentKindInfo select = comboBox_content.SelectedItem as ContentKindInfo;
                    if (select.Nibble1 == info.Nibble1 && select.Nibble2 == info.Nibble2)
                    {
                        MessageBox.Show("すでに追加されています");
                        return;
                    }
                }
                listBox_content.Items.Add(comboBox_content.SelectedItem);
            }
        }

        private void button_all_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem item in listView_service.Items) { item.IsSelected = true; }
        }

        private void button_all_off_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem item in listView_service.Items) { item.IsSelected = false; }
        }

        private void button_video_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                info.IsSelected = (ChSet5.IsVideo(info.ServiceInfo.service_type) == true);
            }
        }

        private void button_bs_on_Click(object sender, RoutedEventArgs e)
        {
            button_all_off_Click(sender, e);
            button_bs_on2_Click(sender, e);
        }

        private void button_bs_on2_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if (ChSet5.IsBS(info.ServiceInfo.ONID) == true && ChSet5.IsVideo(info.ServiceInfo.service_type) == true)
                {
                    info.IsSelected = true;
                }
            }
        }

        private void button_cs_on_Click(object sender, RoutedEventArgs e)
        {
            button_all_off_Click(sender, e);
            button_cs_on2_Click(sender, e);
        }

        private void button_cs_on2_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if (ChSet5.IsCS(info.ServiceInfo.ONID) == true && ChSet5.IsVideo(info.ServiceInfo.service_type) == true)
                {
                    info.IsSelected = true;
                }
            }
        }

        private void button_dttv_on_Click(object sender, RoutedEventArgs e)
        {
            button_all_off_Click(sender, e);
            button_dttv_on2_Click(sender, e);
        }

        private void button_dttv_on2_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if (ChSet5.IsDttv(info.ServiceInfo.ONID) == true && ChSet5.IsVideo(info.ServiceInfo.service_type) == true)
                {
                    info.IsSelected = true;
                }
            }
        }

        private void button_1seg_on_Click(object sender, RoutedEventArgs e)
        {
            button_all_off_Click(sender, e);
            button_1seg_on2_Click(sender, e);
        }

        private void button_1seg_on2_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if (ChSet5.IsDttv(info.ServiceInfo.ONID) == true && info.ServiceInfo.partialReceptionFlag == 1)
                {
                    info.IsSelected = true;
                }
            }
        }

        private void button_other_on_Click(object sender, RoutedEventArgs e)
        {
            button_all_off_Click(sender, e);
            button_other_on2_Click(sender, e);
        }

        private void button_other_on2_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if (ChSet5.IsOther(info.ServiceInfo.ONID) == true)
                {
                    info.IsSelected = true;
                }
            }
        }

        private void button_timeAdd_Click(object sender, RoutedEventArgs e)
        {
            if (comboBox_time_sw.SelectedItem == null ||
                comboBox_time_sh.SelectedItem == null ||
                comboBox_time_sm.SelectedItem == null ||
                comboBox_time_ew.SelectedItem == null ||
                comboBox_time_eh.SelectedItem == null ||
                comboBox_time_em.SelectedItem == null)
            {
                return;
            }

            EpgSearchDateInfo info = new EpgSearchDateInfo();
            DayOfWeekInfo startWeek = comboBox_time_sw.SelectedItem as DayOfWeekInfo;
            DayOfWeekInfo endWeek = comboBox_time_ew.SelectedItem as DayOfWeekInfo;

            info.startDayOfWeek = startWeek.Value;
            info.startHour = (UInt16)comboBox_time_sh.SelectedIndex;
            info.startMin = (UInt16)comboBox_time_sm.SelectedIndex;
            info.endDayOfWeek = endWeek.Value;
            info.endHour = (UInt16)comboBox_time_eh.SelectedIndex;
            info.endMin = (UInt16)comboBox_time_em.SelectedIndex;

            listBox_date.Items.Add(new DateItem(info));
        }

        private void button_weekAdd_Click(object sender, RoutedEventArgs e)
        {
            if (comboBox_week_sh.SelectedItem == null ||
                comboBox_week_sm.SelectedItem == null ||
                comboBox_week_eh.SelectedItem == null ||
                comboBox_week_em.SelectedItem == null)
            {
                return;
            }

            var Add_week = new Action<CheckBox, byte>((chbox, day) =>
            {
                if (chbox.IsChecked != true) return;
                //
                var info = new EpgSearchDateInfo();
                info.startDayOfWeek = day;
                info.startHour = (UInt16)comboBox_week_sh.SelectedIndex;
                info.startMin = (UInt16)comboBox_week_sm.SelectedIndex;
                info.endDayOfWeek = info.startDayOfWeek;
                info.endHour = (UInt16)comboBox_week_eh.SelectedIndex;
                info.endMin = (UInt16)comboBox_week_em.SelectedIndex;
                info.RegulateData();

                listBox_date.Items.Add(new DateItem(info));
            });

            Add_week(checkBox_mon, 1);
            Add_week(checkBox_tue, 2);
            Add_week(checkBox_wen, 3);
            Add_week(checkBox_thu, 4);
            Add_week(checkBox_fri, 5);
            Add_week(checkBox_sat, 6);
            Add_week(checkBox_sun, 0);
        }

        private void checkBox_regExp_Checked(object sender, RoutedEventArgs e)
        {
            checkBox_aimai.IsEnabled = checkBox_regExp.IsChecked != true;
        }
    }

    public class DateItem
    {
        public DateItem(EpgSearchDateInfo info) { DateInfo = info; }
        public EpgSearchDateInfo DateInfo { get; private set; }
        public override string ToString() { return CommonManager.ConvertTimeText(DateInfo); }
    }
}
