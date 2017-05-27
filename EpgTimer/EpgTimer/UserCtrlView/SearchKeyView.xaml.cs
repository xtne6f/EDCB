using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace EpgTimer
{
    /// <summary>
    /// SearchKeyView.xaml の相互作用ロジック
    /// </summary>
    public partial class SearchKeyView : UserControl
    {
        public SearchKeyView()
        {
            InitializeComponent();

            foreach (string info in Settings.Instance.AndKeyList)
            {
                comboBox_andKey.Items.Add(info);
            }
            foreach (string info in Settings.Instance.NotKeyList)
            {
                comboBox_notKey.Items.Add(info);
            }

            EnableContentListBox(false);
            comboBox_content.ItemsSource = CommonManager.Instance.ContentKindList;
            comboBox_content.SelectedIndex = 0;

            EnableDateListBox(false);
            comboBox_time_sw.ItemsSource = CommonManager.Instance.DayOfWeekArray;
            comboBox_time_sw.SelectedIndex = 0;
            comboBox_time_sh.ItemsSource = Enumerable.Range(0, 24);
            comboBox_time_sh.SelectedIndex = 0;
            comboBox_time_sm.ItemsSource = Enumerable.Range(0, 60);
            comboBox_time_sm.SelectedIndex = 0;
            comboBox_time_ew.ItemsSource = CommonManager.Instance.DayOfWeekArray;
            comboBox_time_ew.SelectedIndex = 6;
            comboBox_time_eh.ItemsSource = Enumerable.Range(0, 24);
            comboBox_time_eh.SelectedIndex = 23;
            comboBox_time_em.ItemsSource = Enumerable.Range(0, 60);
            comboBox_time_em.SelectedIndex = 59;
            comboBox_week_sh.ItemsSource = Enumerable.Range(0, 24);
            comboBox_week_sh.SelectedIndex = 0;
            comboBox_week_sm.ItemsSource = Enumerable.Range(0, 60);
            comboBox_week_sm.SelectedIndex = 0;
            radioButton_time.IsChecked = true;

            var serviceList = new List<ServiceItem>();
            foreach (ChSet5Item info in ChSet5.Instance.ChListSelected)
            {
                ServiceItem item = new ServiceItem();
                item.ServiceInfo = CommonManager.ConvertChSet5To(info);
                serviceList.Add(item);
            }
            listView_service.ItemsSource = serviceList;
        }

        public void FocusAndKey()
        {
            comboBox_andKey.Focus();
        }

        public void SaveSearchLog()
        {
            string key = comboBox_andKey.Text;
            if (key.Length > 0)
            {
                foreach (string info in comboBox_andKey.Items)
                {
                    if (string.Compare(key, info, true) == 0)
                    {
                        comboBox_andKey.Items.Remove(info);
                        comboBox_andKey.Text = key;
                        break;
                    }
                }
                comboBox_andKey.Items.Insert(0, key);
            }
            key = comboBox_notKey.Text;
            if (key.Length > 0)
            {
                foreach (string info in comboBox_notKey.Items)
                {
                    if (string.Compare(key, info, true) == 0)
                    {
                        comboBox_notKey.Items.Remove(info);
                        comboBox_notKey.Text = key;
                        break;
                    }
                }
                comboBox_notKey.Items.Insert(0, key);
            }
            SaveSearchLogSettings();
        }

        public void GetSearchKey(ref EpgSearchKeyInfo key)
        {
            key.andKey = comboBox_andKey.Text;
            key.notKey = comboBox_notKey.Text;
            key.regExpFlag = checkBox_regExp.IsChecked == true ? 1 : 0;
            key.aimaiFlag = (byte)(key.regExpFlag == 0 && checkBox_aimai.IsChecked == true ? 1 : 0);
            key.titleOnlyFlag = checkBox_titleOnly.IsChecked == true ? 1 : 0;
            uint durMin;
            uint durMax;
            if (uint.TryParse(textBox_chkDurationMin.Text, out durMin) &&
                uint.TryParse(textBox_chkDurationMax.Text, out durMax) && (durMin > 0 || durMax > 0))
            {
                key.andKey = "D!{" + ((10000 + Math.Min(durMin, 9999)) * 10000 + Math.Min(durMax, 9999)) + "}" + key.andKey;
            }
            if (checkBox_case.IsChecked == true)
            {
                key.andKey = "C!{999}" + key.andKey;
            }
            if (checkBox_keyDisabled.IsChecked == true)
            {
                key.andKey = "^!{999}" + key.andKey;
            }

            key.contentList.Clear();
            if (listBox_content.IsEnabled)
            {
                foreach (ContentKindInfo info in listBox_content.Items)
                {
                    EpgContentData item = new EpgContentData();
                    item.content_nibble_level_1 = info.Nibble1;
                    item.content_nibble_level_2 = info.Nibble2;
                    key.contentList.Add(item);
                }
            }
            key.notContetFlag = (byte)(checkBox_notContent.IsChecked == true ? 1 : 0);

            key.dateList.Clear();
            if (listBox_date.IsEnabled)
            {
                foreach (DateItem info in listBox_date.Items)
                {
                    key.dateList.Add(info.DateInfo);
                }
            }
            key.notDateFlag = (byte)(checkBox_notDate.IsChecked == true ? 1 : 0);

            key.serviceList.Clear();
            foreach (ServiceItem info in listView_service.Items)
            {
                if (info.IsSelected)
                {
                    key.serviceList.Add((long)info.ID);
                }
            }

            key.freeCAFlag = (byte)Math.Min(Math.Max(comboBox_free.SelectedIndex, 0), 2);
            key.chkRecEnd = (byte)(checkBox_chkRecEnd.IsChecked == true ? 1 : 0);
            ushort.TryParse(textBox_chkRecDay.Text, out key.chkRecDay);
            if (checkBox_chkRecNoService.IsChecked == true)
            {
                key.chkRecDay = (ushort)(key.chkRecDay % 10000 + 40000);
            }
        }

        public void SetSearchKey(EpgSearchKeyInfo key)
        {
            comboBox_andKey.Text = Regex.Replace(key.andKey, @"^(?:\^!\{999\})?(?:C!\{999\})?(?:D!\{1[0-9]{8}\})?", "");
            comboBox_notKey.Text = key.notKey;
            checkBox_regExp.IsChecked = key.regExpFlag != 0;
            checkBox_aimai.IsChecked = key.aimaiFlag != 0;
            checkBox_titleOnly.IsChecked = key.titleOnlyFlag != 0;
            var match = Regex.Match(key.andKey, @"^((?:\^!\{999\})?)((?:C!\{999\})?)((?:D!\{1[0-9]{8}\})?)");
            checkBox_keyDisabled.IsChecked = match.Groups[1].Value.Length > 0;
            checkBox_case.IsChecked = match.Groups[2].Value.Length > 0;
            uint dur = 0;
            if (match.Groups[3].Value.Length > 0)
            {
                dur = uint.Parse(match.Groups[3].Value.Substring(3, 9));
            }
            textBox_chkDurationMin.Text = (dur / 10000 % 10000).ToString();
            textBox_chkDurationMax.Text = (dur % 10000).ToString();

            EnableContentListBox(true);
            listBox_content.Items.Clear();
            foreach (EpgContentData item in key.contentList)
            {
                ushort contentKey = (ushort)(item.content_nibble_level_1 << 8 | item.content_nibble_level_2);
                if (CommonManager.Instance.ContentKindDictionary.ContainsKey(contentKey))
                {
                    listBox_content.Items.Add(CommonManager.Instance.ContentKindDictionary[contentKey]);
                }
                else
                {
                    //未知のジャンル
                    listBox_content.Items.Add(new ContentKindInfo("?", "?", item.content_nibble_level_1, item.content_nibble_level_2));
                }
            }
            if (listBox_content.Items.Count == 0)
            {
                EnableContentListBox(false);
            }
            checkBox_notContent.IsChecked = key.notContetFlag != 0;

            EnableDateListBox(true);
            listBox_date.Items.Clear();
            foreach (EpgSearchDateInfo info in key.dateList)
            {
                DateItem item = new DateItem();
                item.DateInfo = info;
                listBox_date.Items.Add(item);
            }
            if (listBox_date.Items.Count == 0)
            {
                EnableDateListBox(false);
            }
            checkBox_notDate.IsChecked = key.notDateFlag != 0;

            var keySortedServiceList = new List<long>(key.serviceList);
            keySortedServiceList.Sort();
            ServiceItem firstSelected = null;
            foreach (ServiceItem info in listView_service.ItemsSource)
            {
                info.IsSelected = keySortedServiceList.BinarySearch((long)info.ID) >= 0;
                if (firstSelected == null && info.IsSelected)
                {
                    firstSelected = info;
                }
            }
            if (firstSelected != null)
            {
                listView_service.ScrollIntoView(firstSelected);
            }

            comboBox_free.SelectedIndex = key.freeCAFlag % 3;
            checkBox_chkRecEnd.IsChecked = key.chkRecEnd != 0;
            textBox_chkRecDay.Text = (key.chkRecDay >= 40000 ? key.chkRecDay % 10000 : key.chkRecDay).ToString();
            checkBox_chkRecNoService.IsChecked = key.chkRecDay >= 40000;
        }

        private void button_content_add_Click(object sender, RoutedEventArgs e)
        {
            ContentKindInfo select = (ContentKindInfo)comboBox_content.SelectedItem;
            if (select != null)
            {
                EnableContentListBox(true);
                foreach (ContentKindInfo info in listBox_content.Items)
                {
                    if (select.Nibble1 == info.Nibble1 && select.Nibble2 == info.Nibble2)
                    {
                        MessageBox.Show("すでに追加されています");
                        return;
                    }
                }
                listBox_content.Items.Add(select);
            }
        }

        private void button_content_del_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_content.IsEnabled && listBox_content.SelectedItem != null)
            {
                var delList = new List<ContentKindInfo>(listBox_content.SelectedItems.Count);
                foreach (ContentKindInfo info in listBox_content.SelectedItems)
                {
                    delList.Add(info);
                }
                foreach (ContentKindInfo info in delList)
                {
                    listBox_content.Items.Remove(info);
                }
                if (listBox_content.Items.Count == 0)
                {
                    EnableContentListBox(false);
                }
            }
        }

        private void button_date_add_Click(object sender, RoutedEventArgs e)
        {
            if (radioButton_time.IsChecked == true)
            {
                var info = new EpgSearchDateInfo();
                info.startDayOfWeek = (byte)Math.Min(Math.Max(comboBox_time_sw.SelectedIndex, 0), 6);
                info.startHour = (ushort)Math.Min(Math.Max(comboBox_time_sh.SelectedIndex, 0), 23);
                info.startMin = (ushort)Math.Min(Math.Max(comboBox_time_sm.SelectedIndex, 0), 59);
                info.endDayOfWeek = (byte)Math.Min(Math.Max(comboBox_time_ew.SelectedIndex, 0), 6);
                info.endHour = (ushort)Math.Min(Math.Max(comboBox_time_eh.SelectedIndex, 0), 23);
                info.endMin = (ushort)Math.Min(Math.Max(comboBox_time_em.SelectedIndex, 0), 59);
                EnableDateListBox(true);
                var item = new DateItem();
                item.DateInfo = info;
                listBox_date.Items.Add(item);
            }
            else
            {
                int startHour = Math.Min(Math.Max(comboBox_week_sh.SelectedIndex, 0), 23);
                int startMin = Math.Min(Math.Max(comboBox_week_sm.SelectedIndex, 0), 59);
                int endHour = Math.Min(Math.Max(comboBox_time_eh.SelectedIndex, 0), 23);
                int endMin = Math.Min(Math.Max(comboBox_time_em.SelectedIndex, 0), 59);
                var Add_week = new Action<CheckBox, byte>((chbox, day) =>
                {
                    if (chbox.IsChecked != true) return;
                    //
                    var info = new EpgSearchDateInfo();
                    info.startDayOfWeek = day;
                    info.startHour = (ushort)startHour;
                    info.startMin = (ushort)startMin;
                    info.endDayOfWeek = info.startDayOfWeek;
                    info.endHour = (ushort)endHour;
                    info.endMin = (ushort)endMin;
                    if (endHour * 60 + endMin < startHour * 60 + startMin)
                    {
                        //終了時間は翌日のものとみなす
                        info.endDayOfWeek = (byte)((info.endDayOfWeek + 1) % 7);
                    }
                    EnableDateListBox(true);
                    var item = new DateItem();
                    item.DateInfo = info;
                    listBox_date.Items.Add(item);
                });
                Add_week(checkBox_sun, 0);
                Add_week(checkBox_mon, 1);
                Add_week(checkBox_tue, 2);
                Add_week(checkBox_wed, 3);
                Add_week(checkBox_thu, 4);
                Add_week(checkBox_fri, 5);
                Add_week(checkBox_sat, 6);
            }
        }

        private void button_date_del_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_date.IsEnabled && listBox_date.SelectedItem != null)
            {
                var delList = new List<DateItem>(listBox_date.SelectedItems.Count);
                foreach (DateItem info in listBox_date.SelectedItems)
                {
                    delList.Add(info);
                }
                foreach (DateItem info in delList)
                {
                    listBox_date.Items.Remove(info);
                }
                if (listBox_date.Items.Count == 0)
                {
                    EnableDateListBox(false);
                }
            }
        }

        private void radioButton_time_Checked(object sender, RoutedEventArgs e)
        {
            SetVisibilityDateControls(false);
        }

        private void radioButton_week_Checked(object sender, RoutedEventArgs e)
        {
            SetVisibilityDateControls(true);
        }

        private void button_all_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                info.IsSelected = true;
            }
        }

        private void button_video_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                info.IsSelected = ChSet5.IsVideo(info.ServiceInfo.service_type);
            }
        }

        private void button_all_off_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                info.IsSelected = false;
            }
        }

        private void button_dttv_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if (ChSet5.IsDttv(info.ServiceInfo.ONID) &&
                    ChSet5.IsVideo(info.ServiceInfo.service_type))
                {
                    info.IsSelected = true;
                }
            }
        }

        private void button_bs_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if (ChSet5.IsBS(info.ServiceInfo.ONID) &&
                    ChSet5.IsVideo(info.ServiceInfo.service_type))
                {
                    info.IsSelected = true;
                }
            }
        }

        private void button_cs_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if (ChSet5.IsCS(info.ServiceInfo.ONID) &&
                    ChSet5.IsVideo(info.ServiceInfo.service_type))
                {
                    info.IsSelected = true;
                }
            }
        }

        private void SetVisibilityDateControls(bool isWeek)
        {
            Visibility timeVis = isWeek ? Visibility.Hidden : Visibility.Visible;
            Visibility weekVis = isWeek ? Visibility.Visible : Visibility.Hidden;
            comboBox_time_sw.Visibility = timeVis;
            comboBox_time_sh.Visibility = timeVis;
            comboBox_time_sm.Visibility = timeVis;
            comboBox_time_dash.Visibility = timeVis;
            comboBox_time_ew.Visibility = timeVis;
            checkBox_sun.Visibility = weekVis;
            checkBox_mon.Visibility = weekVis;
            checkBox_tue.Visibility = weekVis;
            checkBox_wed.Visibility = weekVis;
            checkBox_thu.Visibility = weekVis;
            checkBox_fri.Visibility = weekVis;
            checkBox_sat.Visibility = weekVis;
            comboBox_week_sh.Visibility = weekVis;
            comboBox_week_sm.Visibility = weekVis;
            comboBox_week_dash.Visibility = weekVis;
        }

        private void EnableContentListBox(bool isEnabled)
        {
            if (listBox_content.IsEnabled != isEnabled)
            {
                listBox_content.Items.Clear();
                listBox_content.IsEnabled = isEnabled;
                checkBox_notContent.IsEnabled = isEnabled;
                if (isEnabled == false)
                {
                    listBox_content.Items.Add(new ContentKindInfo("(全ジャンル)", "", 0xFF, 0xFF));
                }
            }
        }

        private void EnableDateListBox(bool isEnabled)
        {
            if (listBox_date.IsEnabled != isEnabled)
            {
                listBox_date.Items.Clear();
                listBox_date.IsEnabled = isEnabled;
                checkBox_notDate.IsEnabled = isEnabled;
                if (isEnabled == false)
                {
                    listBox_date.Items.Add("(全期間)");
                }
            }
        }

        private void comboBox_andKey_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Delete && comboBox_andKey.IsDropDownOpen)
            {
                int i = comboBox_andKey.SelectedIndex;
                if (i >= 0)
                {
                    comboBox_andKey.Items.RemoveAt(i);
                    comboBox_andKey.SelectedIndex = Math.Min(i, comboBox_andKey.Items.Count - 1);
                    SaveSearchLogSettings();
                }
            }
        }

        private void comboBox_notKey_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Delete && comboBox_notKey.IsDropDownOpen)
            {
                int i = comboBox_notKey.SelectedIndex;
                if (i >= 0)
                {
                    comboBox_notKey.Items.RemoveAt(i);
                    comboBox_notKey.SelectedIndex = Math.Min(i, comboBox_notKey.Items.Count - 1);
                    SaveSearchLogSettings();
                }
            }
        }

        private void SaveSearchLogSettings()
        {
            Settings.Instance.AndKeyList.Clear();
            for (int i = 0; i < 30 && i < comboBox_andKey.Items.Count; i++)
            {
                Settings.Instance.AndKeyList.Add((string)comboBox_andKey.Items[i]);
            }
            Settings.Instance.NotKeyList.Clear();
            for (int i = 0; i < 30 && i < comboBox_notKey.Items.Count; i++)
            {
                Settings.Instance.NotKeyList.Add((string)comboBox_notKey.Items[i]);
            }
            Settings.SaveToXmlFile();
        }
    }
}
