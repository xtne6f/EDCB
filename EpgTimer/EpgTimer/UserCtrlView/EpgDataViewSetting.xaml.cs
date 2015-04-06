using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    /// <summary>
    /// EpgDataViewSetting.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgDataViewSetting : UserControl
    {
        private EpgSearchKeyInfo searchKey = new EpgSearchKeyInfo();
        public EpgDataViewSetting()
        {
            InitializeComponent();

            try
            {
                if (Settings.Instance.NoStyle == 1)
                {
                    button_searchKey.Style = null;
                    button_service_addAll.Style = null;
                    button_service_add.Style = null;
                    button_service_del.Style = null;
                    button_service_delAll.Style = null;
                    button_service_up.Style = null;
                    button_service_down.Style = null;
                    button_service_top.Style = null;
                    button_service_bottom.Style = null;
                    button_service_addVideo.Style = null;
                    button_jyanru_addAll.Style = null;
                    button_jyanru_add.Style = null;
                    button_jyanru_del.Style = null;
                    button_jyanru_delAll.Style = null;
                }

                comboBox_timeH_week.ItemsSource = CommonManager.Instance.HourDictionary.Values;
                comboBox_timeH_week.SelectedIndex = 4;

                foreach (ChSet5Item info in ChSet5.Instance.ChList.Values)
                {
                    if (info.ONID == 0x0004)
                    {
                        listBox_serviceBS.Items.Add(info);
                    }
                    else if (info.ONID == 0x0006 || info.ONID == 0x0007)
                    {
                        listBox_serviceCS.Items.Add(info);
                    }
                    else if (0x7880 <= info.ONID && info.ONID <= 0x7FE8)
                    {
                        listBox_serviceTere.Items.Add(info);
                    }
                    else
                    {
                        listBox_serviceOther.Items.Add(info);
                    }
                    listBox_serviceAll.Items.Add(info);
                }
                listBox_jyanru.DataContext = CommonManager.Instance.ContentKindList;

                radioButton_rate.IsChecked = true;
                radioButton_week.IsChecked = false;
                radioButton_list.IsChecked = false;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }

        }

        /// <summary>
        /// デフォルト表示の設定値
        /// </summary>
        /// <param name="setInfo"></param>
        public void SetSetting(CustomEpgTabInfo setInfo)
        {
            setInfo.SearchKey.CopyTo(searchKey);

            textBox_tabName.Text = setInfo.TabName;
            radioButton_rate.IsChecked = false;
            radioButton_week.IsChecked = false;
            radioButton_list.IsChecked = false;
            switch (setInfo.ViewMode)
            {
                case 1:
                    radioButton_week.IsChecked = true;
                    break;
                case 2:
                    radioButton_list.IsChecked = true;
                    break;
                default:
                    radioButton_rate.IsChecked = true;
                    break;
            }

            checkBox_noTimeView_rate.IsChecked = setInfo.NeedTimeOnlyBasic;
            checkBox_noTimeView_week.IsChecked = setInfo.NeedTimeOnlyWeek;
            comboBox_timeH_week.SelectedIndex = setInfo.StartTimeWeek;
            checkBox_searchMode.IsChecked = setInfo.SearchMode;
            checkBox_filterEnded.IsChecked = (setInfo.FilterEnded == true);

            foreach (UInt64 id in setInfo.ViewServiceList)
            {
                if (ChSet5.Instance.ChList.ContainsKey(id) == true)
                {
                    listBox_serviceView.Items.Add(ChSet5.Instance.ChList[id]);
                }
            }
            foreach (UInt16 id in setInfo.ViewContentKindList)
            {
                if (CommonManager.Instance.ContentKindDictionary.ContainsKey(id) == true)
                {
                    listBox_jyanruView.Items.Add(CommonManager.Instance.ContentKindDictionary[id]);
                }
            }
        }

        /// <summary>
        /// 設定値の取得
        /// </summary>
        /// <param name="setInfo"></param>
        public void GetSetting(ref CustomEpgTabInfo info)
        {
            info.TabName = textBox_tabName.Text;
            info.ViewMode = 0;
            if (radioButton_week.IsChecked == true)
            {
                info.ViewMode = 1;
            }
            else if (radioButton_list.IsChecked == true)
            {
                info.ViewMode = 2;
            }

            info.NeedTimeOnlyBasic = (checkBox_noTimeView_rate.IsChecked == true);
            info.NeedTimeOnlyWeek = (checkBox_noTimeView_week.IsChecked == true);
            info.StartTimeWeek = comboBox_timeH_week.SelectedIndex;
            info.SearchMode = (checkBox_searchMode.IsChecked == true);
            info.FilterEnded = (checkBox_filterEnded.IsChecked == true);

            searchKey.CopyTo(info.SearchKey);
 
            info.ViewServiceList.Clear();
            foreach (ChSet5Item item in listBox_serviceView.Items)
            {
                info.ViewServiceList.Add(item.Key);
            }

            info.ViewContentKindList.Clear();
            foreach (ContentKindInfo item in listBox_jyanruView.Items)
            {
                info.ViewContentKindList.Add(item.ID);
            }
        }

        private ListBox SelectedServiceListBox()
        {
            if (tabItem_bs.IsSelected == true) return listBox_serviceBS;
            if (tabItem_cs.IsSelected == true) return listBox_serviceCS;
            if (tabItem_tere.IsSelected == true) return listBox_serviceTere;
            if (tabItem_other.IsSelected == true) return listBox_serviceOther;
            if (tabItem_all.IsSelected == true) return listBox_serviceAll;
            return null;
        }

        /// <summary>サービス全追加</summary>
        private void button_service_addAll_Click(object sender, RoutedEventArgs e)
        {
            ListBox listBox = SelectedServiceListBox();
            if (listBox == null) return;

            listBox.UnselectAll();
            listBox.SelectAll();
            addItems(listBox, listBox_serviceView);
        }

        /// <summary>映像のみ全追加</summary>
        private void button_service_addVideo_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ListBox listBox = SelectedServiceListBox();
                if (listBox == null) return;

                listBox.UnselectAll();
                foreach (ChSet5Item info in listBox.Items)
                {
                    if (info.ServiceType != 0x01 && info.ServiceType != 0xA5) continue;
                    listBox.SelectedItems.Add(info);//重い。
                }
                addItems(listBox, listBox_serviceView);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>選択サービス追加</summary>
        private void button_service_add_Click(object sender, RoutedEventArgs e)
        {
            addItems(SelectedServiceListBox(), listBox_serviceView);
        }

        /// <summary>選択サービス削除</summary>
        private void button_service_del_Click(object sender, RoutedEventArgs e)
        {
            deleteItems(listBox_serviceView);
        }

        /// <summary>サービス全削除</summary>
        private void button_service_delAll_Click(object sender, RoutedEventArgs e)
        {
            listBox_serviceView.Items.Clear();
        }

        private void addItems(ListBox src, ListBox target)
        {
            try
            {
                if (src == null || target == null) return;

                foreach (object info in src.SelectedItems)
                {
                    if (target.Items.Contains(info) == false)
                    {
                        target.Items.Add(info);
                    }
                }
                if (target.Items.Count != 0)
                {
                    target.SelectedIndex = target.Items.Count - 1;
                    target.ScrollIntoView(target.SelectedItems);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void deleteItems(ListBox target)
        {
            try
            {
                if (target == null) return;

                var delItems = target.SelectedItems.Cast<object>().ToList();
                int newSelectedIndex = 0;
                foreach (object info in delItems)
                {
                    newSelectedIndex = target.Items.IndexOf(info);
                    target.Items.RemoveAt(newSelectedIndex);
                }

                if (target.Items.Count != 0)
                {
                    newSelectedIndex = (newSelectedIndex == target.Items.Count ? newSelectedIndex - 1 : newSelectedIndex);
                    target.SelectedIndex = newSelectedIndex;
                    target.ScrollIntoView(target.SelectedItems);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>1つ上に移動</summary>
        private void button_service_up_Click(object sender, RoutedEventArgs e)
        {
            move_item(listBox_serviceView, -1);
        }

        /// <summary>1つ下に移動</summary>
        private void button_service_down_Click(object sender, RoutedEventArgs e)
        {
            move_item(listBox_serviceView, 1);
        }

        /// <summary>一番上に移動</summary>
        private void button_service_top_Click(object sender, RoutedEventArgs e)
        {
            move_item(listBox_serviceView, -1 * listBox_serviceView.SelectedIndex);
        }

        /// <summary>一番下に移動</summary>
        private void button_service_bottom_Click(object sender, RoutedEventArgs e)
        {
            move_item(listBox_serviceView, listBox_serviceView.Items.Count - 1 - listBox_serviceView.SelectedIndex);
        }

        /// <summary>アイテムを移動</summary>
        private void move_item(ListBox target, int direction)
        {
            try
            {
                if (target == null || target.SelectedItem == null) return;

                object temp = target.SelectedItem;
                int newIndex = ((target.SelectedIndex + direction) % target.Items.Count + target.Items.Count) % target.Items.Count;
                target.Items.RemoveAt(target.SelectedIndex);
                target.Items.Insert(newIndex, temp);
                target.SelectedIndex = newIndex;
                target.ScrollIntoView(target.SelectedItem);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>ジャンル全追加</summary>
        private void button_jyanru_addAll_Click(object sender, RoutedEventArgs e)
        {
            listBox_jyanru.UnselectAll();
            listBox_jyanru.SelectAll();
            addItems(listBox_jyanru, listBox_jyanruView);
        }

        /// <summary>選択ジャンル追加</summary>
        private void button_jyanru_add_Click(object sender, RoutedEventArgs e)
        {
            addItems(listBox_jyanru, listBox_jyanruView);
        }

        /// <summary>選択ジャンル削除</summary>
        private void button_jyanru_del_Click(object sender, RoutedEventArgs e)
        {
            deleteItems(listBox_jyanruView);
        }

        /// <summary>ジャンル全削除</summary>
        private void button_jyanru_delAll_Click(object sender, RoutedEventArgs e)
        {
            listBox_jyanruView.Items.Clear();
        }

        private void listBox_serviceView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            Display_ServiceView(listBox_serviceView, textBox_serviceView1);
        }

        private void listBox_service_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ListBox listBox = SelectedServiceListBox();
            if (listBox == null) return;

            Display_ServiceView(listBox, textBox_serviceView2);
        }

        private void Display_ServiceView(ListBox srclistBox, TextBox targetBox)
        {
            try
            {
                targetBox.Text = "";
                if (srclistBox.SelectedItem == null) return;

                ChSet5Item info = srclistBox.SelectedItems[srclistBox.SelectedItems.Count - 1] as ChSet5Item;
                targetBox.Text = 
                    info.ServiceName + "\r\n" +
                    info.NetworkName + "\r\n" +
                    "OriginalNetworkID : " + info.ONID.ToString() + " (0x" + info.ONID.ToString("X4") + ")\r\n" +
                    "TransportStreamID : " + info.TSID.ToString() + " (0x" + info.TSID.ToString("X4") + ")\r\n" +
                    "ServiceID : " + info.SID.ToString() + " (0x" + info.SID.ToString("X4") + ")\r\n";
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_searchKey_Click(object sender, RoutedEventArgs e)
        {
            SetDefSearchSettingWindow dlg = new SetDefSearchSettingWindow();
            PresentationSource topWindow = PresentationSource.FromVisual(this);
            if (topWindow != null)
            {
                dlg.Owner = (Window)topWindow.RootVisual;
            }
            dlg.SetDefSetting(searchKey);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetSetting(ref searchKey);
            }
        }

    }
}
