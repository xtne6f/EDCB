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
            foreach (ushort id in CommonManager.Instance.ContentKindList)
            {
                comboBox_content.Items.Add(new ContentKindInfo() { Nibble1 = (byte)(id >> 8), Nibble2 = (byte)id });
            }
            comboBox_content.SelectedIndex = 0;

            EnableDateListBox(false);
            comboBox_time_sw.ItemsSource = Enumerable.Range(0, 7).Select(i => (new DateTime(2000, 1, 2 + i)).ToString("ddd"));
            comboBox_time_sw.SelectedIndex = 0;
            comboBox_time_sh.ItemsSource = Enumerable.Range(0, 24);
            comboBox_time_sh.SelectedIndex = 0;
            comboBox_time_sm.ItemsSource = Enumerable.Range(0, 60);
            comboBox_time_sm.SelectedIndex = 0;
            comboBox_time_ew.ItemsSource = Enumerable.Range(0, 7).Select(i => (new DateTime(2000, 1, 2 + i)).ToString("ddd"));
            comboBox_time_ew.SelectedIndex = 6;
            comboBox_time_eh.ItemsSource = Enumerable.Range(0, 24);
            comboBox_time_eh.SelectedIndex = 23;
            comboBox_time_em.ItemsSource = Enumerable.Range(0, 60);
            comboBox_time_em.SelectedIndex = 59;
            comboBox_week_sh.ItemsSource = Enumerable.Range(0, 24);
            comboBox_week_sh.SelectedIndex = 0;
            comboBox_week_sm.ItemsSource = Enumerable.Range(0, 60);
            comboBox_week_sm.SelectedIndex = 0;
            radioButton_week.IsChecked = true;

            foreach (ChSet5Item info in ChSet5.Instance.ChListSelected)
            {
                if (ChSet5.IsCS3(info.ONID))
                {
                    button_cs3.Visibility = Visibility.Visible;
                }
                listView_service.Items.Add(new ServiceItem(CommonManager.ConvertChSet5To(info)));
            }

            if (0 <= Settings.Instance.SearchWndNotKeyRatio && Settings.Instance.SearchWndNotKeyRatio <= 1)
            {
                columnDefinition_notKey.Width = new GridLength(Settings.Instance.SearchWndNotKeyRatio, GridUnitType.Star);
                columnDefinition_note.Width = new GridLength(1 - Settings.Instance.SearchWndNotKeyRatio, GridUnitType.Star);
            }
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
                    if (key.Equals(info, StringComparison.OrdinalIgnoreCase))
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
                    if (key.Equals(info, StringComparison.OrdinalIgnoreCase))
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

        public EpgSearchKeyInfo GetSearchKey()
        {
            var key = new EpgSearchKeyInfo();
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
            if (textBox_note.Text.Length > 0 || key.notKey.StartsWith(":note:", StringComparison.Ordinal))
            {
                key.notKey = ":note:" + textBox_note.Text.Replace("\\", "\\\\").Replace(" ", "\\s").Replace("　", "\\m") +
                             (key.notKey.Length > 0 ? " " + key.notKey : "");
            }

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

            if (listBox_date.IsEnabled)
            {
                foreach (Tuple<string, EpgSearchDateInfo> info in listBox_date.Items)
                {
                    key.dateList.Add(info.Item2);
                }
            }
            key.notDateFlag = (byte)(checkBox_notDate.IsChecked == true ? 1 : 0);

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
            return key;
        }

        public void SetSearchKey(EpgSearchKeyInfo key)
        {
            comboBox_andKey.Text = Regex.Replace(key.andKey, @"^(?:\^!\{999\})?(?:C!\{999\})?(?:D!\{1[0-9]{8}\})?", "");
            comboBox_notKey.Text = Regex.Replace(key.notKey, "^:note:[^ 　]*[ 　]?", "");
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
            match = Regex.Match(key.notKey, "^:note:([^ 　]*)");
            textBox_note.Text = match.Success ? match.Groups[1].Value.Replace("\\s", " ").Replace("\\m", "　").Replace("\\\\", "\\") : "";

            EnableContentListBox(true);
            listBox_content.Items.Clear();
            foreach (EpgContentData item in key.contentList)
            {
                ushort contentKey = (ushort)(item.content_nibble_level_1 << 8 | item.content_nibble_level_2);
                ContentKindInfo info = comboBox_content.Items.Cast<ContentKindInfo>().FirstOrDefault(a => a.ID == contentKey);
                if (info == null)
                {
                    //未知のジャンル
                    info = new ContentKindInfo() { Nibble1 = item.content_nibble_level_1, Nibble2 = item.content_nibble_level_2 };
                }
                listBox_content.Items.Add(info);
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
                listBox_date.Items.Add(new Tuple<string, EpgSearchDateInfo>(
                    (new DateTime(2000, 1, 2 + info.startDayOfWeek % 7, info.startHour % 24, info.startMin % 60, 0)).ToString("ddd HH\\:mm") +
                    (new DateTime(2000, 1, 2 + info.endDayOfWeek % 7, info.endHour % 24, info.endMin % 60, 0)).ToString(" ～ ddd HH\\:mm"), info));
            }
            if (listBox_date.Items.Count == 0)
            {
                EnableDateListBox(false);
            }
            checkBox_notDate.IsChecked = key.notDateFlag != 0;

            var keySortedServiceList = new List<long>(key.serviceList);
            keySortedServiceList.Sort();
            ServiceItem firstSelected = null;
            foreach (ServiceItem info in listView_service.Items)
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
                if (listBox_content.Items.Contains(select))
                {
                    MessageBox.Show("すでに追加されています");
                    return;
                }
                listBox_content.Items.Add(select);
            }
        }

        private void button_content_del_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_content.IsEnabled && listBox_content.SelectedItem != null)
            {
                foreach (object info in listBox_content.SelectedItems.Cast<object>().ToList())
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
            for (byte day = 0; day < 7; day++)
            {
                var info = new EpgSearchDateInfo();
                info.endHour = (ushort)Math.Min(Math.Max(comboBox_time_eh.SelectedIndex, 0), 23);
                info.endMin = (ushort)Math.Min(Math.Max(comboBox_time_em.SelectedIndex, 0), 59);
                if (radioButton_week.IsChecked == false)
                {
                    info.startDayOfWeek = (byte)Math.Min(Math.Max(comboBox_time_sw.SelectedIndex, 0), 6);
                    info.startHour = (ushort)Math.Min(Math.Max(comboBox_time_sh.SelectedIndex, 0), 23);
                    info.startMin = (ushort)Math.Min(Math.Max(comboBox_time_sm.SelectedIndex, 0), 59);
                    info.endDayOfWeek = (byte)Math.Min(Math.Max(comboBox_time_ew.SelectedIndex, 0), 6);
                    day = 6;
                }
                else
                {
                    var chbox = (new CheckBox[] { checkBox_sun, checkBox_mon, checkBox_tue, checkBox_wed, checkBox_thu, checkBox_fri, checkBox_sat })[day];
                    if (chbox.IsChecked != true)
                    {
                        continue;
                    }
                    info.startDayOfWeek = day;
                    info.startHour = (ushort)Math.Min(Math.Max(comboBox_week_sh.SelectedIndex, 0), 23);
                    info.startMin = (ushort)Math.Min(Math.Max(comboBox_week_sm.SelectedIndex, 0), 59);
                    info.endDayOfWeek = info.startDayOfWeek;
                    if (info.endHour * 60 + info.endMin < info.startHour * 60 + info.startMin)
                    {
                        //終了時間は翌日のものとみなす
                        info.endDayOfWeek = (byte)((info.endDayOfWeek + 1) % 7);
                    }
                }
                EnableDateListBox(true);
                listBox_date.Items.Add(new Tuple<string, EpgSearchDateInfo>(
                    (new DateTime(2000, 1, 2 + info.startDayOfWeek, info.startHour, info.startMin, 0)).ToString("ddd HH\\:mm") +
                    (new DateTime(2000, 1, 2 + info.endDayOfWeek, info.endHour, info.endMin, 0)).ToString(" ～ ddd HH\\:mm"), info));
            }
        }

        private void button_date_del_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_date.IsEnabled && listBox_date.SelectedItem != null)
            {
                foreach (object info in listBox_date.SelectedItems.Cast<object>().ToList())
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
                    ChSet5.IsCS3(info.ServiceInfo.ONID) == false &&
                    ChSet5.IsVideo(info.ServiceInfo.service_type))
                {
                    info.IsSelected = true;
                }
            }
        }

        private void button_cs3_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if (ChSet5.IsCS3(info.ServiceInfo.ONID) &&
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
                    //"(全ジャンル)"表示用の特殊値
                    listBox_content.Items.Add(new ContentKindInfo() { Nibble1 = 0xFE, Nibble2 = 0xFF });
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
                    listBox_date.Items.Add(new Tuple<string, EpgSearchDateInfo>("(全期間)", null));
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

        private void comboBox_notKey_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            Settings.Instance.SearchWndNotKeyRatio =
                columnDefinition_notKey.ActualWidth / (columnDefinition_notKey.ActualWidth + columnDefinition_note.ActualWidth);
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
