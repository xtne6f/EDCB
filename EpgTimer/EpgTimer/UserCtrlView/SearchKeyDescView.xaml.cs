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
        private RadioBtnSelect freeRadioBtns;

        public SearchKeyDescView()
        {
            InitializeComponent();
            try
            {
                foreach (ChSet5Item info in ChSet5.ChList.Values)
                {
                    ServiceItem item = new ServiceItem();

                    item.ServiceInfo = CommonManager.ConvertChSet5To(info);
                    serviceList.Add(item);
                    serviceDict.Add((Int64)item.ID, item);
                }
                listView_service.ItemsSource = serviceList;

                comboBox_content.ItemsSource = CommonManager.ContentKindList;
                comboBox_content.SelectedIndex = 0;

                comboBox_time_sw.ItemsSource = new string[] { "日", "月", "火", "水", "木", "金", "土" };
                comboBox_time_sw.SelectedIndex = 0;
                comboBox_time_sh.ItemsSource = Enumerable.Range(0, 24);
                comboBox_time_sh.SelectedIndex = 0;
                comboBox_time_sm.ItemsSource = Enumerable.Range(0, 60);
                comboBox_time_sm.SelectedIndex = 0;
                comboBox_time_ew.ItemsSource = new string[] { "日", "月", "火", "水", "木", "金", "土" };
                comboBox_time_ew.SelectedIndex = 6;
                comboBox_time_eh.ItemsSource = Enumerable.Range(0, 24);
                comboBox_time_eh.SelectedIndex = 23;
                comboBox_time_em.ItemsSource = Enumerable.Range(0, 60);
                comboBox_time_em.SelectedIndex = 59;
                comboBox_week_sh.ItemsSource = CommonManager.CustomHourList;
                comboBox_week_sh.SelectedIndex = 0;
                comboBox_week_sm.ItemsSource = Enumerable.Range(0, 60);
                comboBox_week_sm.SelectedIndex = 0;
                comboBox_week_eh.ItemsSource = CommonManager.CustomHourList;
                comboBox_week_eh.SelectedIndex = 23;
                comboBox_week_em.ItemsSource = Enumerable.Range(0, 60);
                comboBox_week_em.SelectedIndex = 59;

                freeRadioBtns = new RadioBtnSelect(radioButton_free_1, radioButton_free_2, radioButton_free_3);
                
                var bxc = new BoxExchangeEdit.BoxExchangeEditor(null, listBox_content, true, true, true);
                button_content_clear.Click += new RoutedEventHandler(bxc.button_DeleteAll_Click);
                button_content_del.Click += new RoutedEventHandler(bxc.button_Delete_Click);

                var bxd = new BoxExchangeEdit.BoxExchangeEditor(null, listBox_date, true, true, true);
                button_date_clear.Click += new RoutedEventHandler(bxd.button_DeleteAll_Click);
                button_date_del.Click += new RoutedEventHandler(bxd.button_Delete_Click);

                new BoxExchangeEdit.BoxExchangeEditor(null, listView_service, true);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public void SetChangeMode(int chgMode)
        {
            ViewUtil.SetSpecificChgAppearance(listBox_content);
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
                key.contentList = listBox_content.Items.OfType<ContentKindInfo>().Select(info => new EpgContentData { content_nibble_level_1 = info.Nibble1, content_nibble_level_2 = info.Nibble2 }).ToList();
                key.notContetFlag = (byte)(checkBox_notContent.IsChecked == true ? 1 : 0);
                key.serviceList = listView_service.Items.OfType<ServiceItem>().Where(info => info.IsSelected == true).Select(info => (Int64)info.ID).ToList();
                key.dateList = listBox_date.Items.OfType<DateItem>().Select(info => info.DateInfo).ToList();
                key.notDateFlag = (byte)(checkBox_notDate.IsChecked == true ? 1 : 0);
                key.freeCAFlag = (byte)freeRadioBtns.Value;
                key.chkRecEnd = (byte)(checkBox_chkRecEnd.IsChecked == true ? 1 : 0);
                key.chkRecDay = (ushort)MenuUtil.MyToNumerical(textBox_chkRecDay, Convert.ToUInt32, 9999u, 0u, 0u);
                key.chkRecNoService = (byte)(radioButton_chkRecNoService2.IsChecked == true ? 1 : 0);
                key.chkDurationMin = (ushort)MenuUtil.MyToNumerical(textBox_chkDurationMin, Convert.ToUInt32, 9999u, 0u, 0u);
                key.chkDurationMax = (ushort)MenuUtil.MyToNumerical(textBox_chkDurationMax, Convert.ToUInt32, 9999u, 0u, 0u);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
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
                    var contentKey = (UInt16)(item.content_nibble_level_1 << 8 | item.content_nibble_level_2);
                    if (CommonManager.ContentKindDictionary.ContainsKey(contentKey) == true)
                    {
                        listBox_content.Items.Add(CommonManager.ContentKindDictionary[contentKey]);
                    }
                    else
                    {
                        //未知のジャンル
                        listBox_content.Items.Add(new ContentKindInfo("?", "?", item.content_nibble_level_1, item.content_nibble_level_2));
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

                freeRadioBtns.Value = defKey.freeCAFlag;

                checkBox_chkRecEnd.IsChecked = (defKey.chkRecEnd == 1);
                textBox_chkRecDay.Text = defKey.chkRecDay.ToString();
                radioButton_chkRecNoService1.IsChecked = (defKey.chkRecNoService == 0);
                radioButton_chkRecNoService2.IsChecked = (defKey.chkRecNoService != 0);

                textBox_chkDurationMin.Text = defKey.chkDurationMin.ToString();
                textBox_chkDurationMax.Text = defKey.chkDurationMax.ToString();   
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
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

            info.startDayOfWeek = (byte)Math.Min(comboBox_time_sw.SelectedIndex, 6);
            info.startHour = (UInt16)comboBox_time_sh.SelectedIndex;
            info.startMin = (UInt16)comboBox_time_sm.SelectedIndex;
            info.endDayOfWeek = (byte)Math.Min(comboBox_time_ew.SelectedIndex, 6);
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
