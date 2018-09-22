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
using System.Windows.Navigation;
using System.Windows.Shapes;

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
                comboBox_timeH_week.ItemsSource = Enumerable.Range(0, 24);
                comboBox_timeH_week.SelectedIndex = 4;


                foreach (ChSet5Item info in ChSet5.Instance.ChListSelected)
                {
                    if (ChSet5.IsBS(info.ONID))
                    {
                        tabItem_bs.Visibility = Visibility.Visible;
                        listBox_serviceBS.Items.Add(info);
                    }
                    else if (ChSet5.IsCS3(info.ONID))
                    {
                        tabItem_cs3.Visibility = Visibility.Visible;
                        listBox_serviceCS3.Items.Add(info);
                    }
                    else if (ChSet5.IsCS(info.ONID))
                    {
                        tabItem_cs.Visibility = Visibility.Visible;
                        listBox_serviceCS.Items.Add(info);
                    }
                    else if (ChSet5.IsDttv(info.ONID))
                    {
                        tabItem_tere.Visibility = Visibility.Visible;
                        listBox_serviceTere.Items.Add(info);
                    }
                    else
                    {
                        tabItem_other.Visibility = Visibility.Visible;
                        listBox_serviceOther.Items.Add(info);
                    }
                }
                TabItem item = tabControl2.Items.Cast<TabItem>().FirstOrDefault(a => a.Visibility == Visibility.Visible);
                if (item != null)
                {
                    item.IsSelected = true;
                }
                listBox_jyanru.ItemsSource = CommonManager.Instance.ContentKindList;

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
            searchKey = setInfo.SearchKey.DeepClone();

            textBox_tabName.Text = setInfo.TabName;
            if (setInfo.ViewMode == 1)
            {
                radioButton_rate.IsChecked = false;
                radioButton_week.IsChecked = true;
                radioButton_list.IsChecked = false;
            }
            else if (setInfo.ViewMode == 2)
            {
                radioButton_rate.IsChecked = false;
                radioButton_week.IsChecked = false;
                radioButton_list.IsChecked = true;
            }
            else
            {
                radioButton_rate.IsChecked = true;
                radioButton_week.IsChecked = false;
                radioButton_list.IsChecked = false;
            }

            if (setInfo.NeedTimeOnlyBasic == true)
            {
                checkBox_noTimeView_rate.IsChecked = true;
            }
            else
            {
                checkBox_noTimeView_rate.IsChecked = false;
            }
            if (setInfo.NeedTimeOnlyWeek == true)
            {
                checkBox_noTimeView_week.IsChecked = true;
            }
            else
            {
                checkBox_noTimeView_week.IsChecked = false;
            }
            comboBox_timeH_week.SelectedIndex = setInfo.StartTimeWeek;

            if (setInfo.SearchMode == true)
            {
                checkBox_searchMode.IsChecked = true;
            }
            else
            {
                checkBox_searchMode.IsChecked = false;
            }

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

            if (setInfo.FilterEnded == true)
            {
                checkBox_filterEnded.IsChecked = true;
            }
            else
            {
                checkBox_filterEnded.IsChecked = false;
            }
        }

        /// <summary>
        /// 設定値の取得
        /// </summary>
        public CustomEpgTabInfo GetSetting()
        {
            var info = new CustomEpgTabInfo();
            info.TabName = textBox_tabName.Text;
            if (radioButton_rate.IsChecked == true)
            {
                info.ViewMode = 0;
            }
            else if (radioButton_week.IsChecked == true)
            {
                info.ViewMode = 1;
            }
            else if (radioButton_list.IsChecked == true)
            {
                info.ViewMode = 2;
            }
            else
            {
                info.ViewMode = 0;
            }
            if (checkBox_noTimeView_rate.IsChecked == true)
            {
                info.NeedTimeOnlyBasic = true;
            }
            else
            {
                info.NeedTimeOnlyBasic = false;
            }
            if (checkBox_noTimeView_week.IsChecked == true)
            {
                info.NeedTimeOnlyWeek = true;
            } 
            else
            {
                info.NeedTimeOnlyWeek = false;
            }
            info.StartTimeWeek = comboBox_timeH_week.SelectedIndex;

            if (checkBox_searchMode.IsChecked == true)
            {
                info.SearchMode = true;
            }
            else
            {
                info.SearchMode = false;
            }

            if (checkBox_filterEnded.IsChecked == true)
            {
                info.FilterEnded = true;
            }
            else
            {
                info.FilterEnded = false;
            }

            info.SearchKey = searchKey;

            foreach (ChSet5Item item in listBox_serviceView.Items)
            {
                info.ViewServiceList.Add(item.Key);
            }

            foreach (ContentKindInfo item in listBox_jyanruView.Items)
            {
                info.ViewContentKindList.Add(item.ID);
            }
            return info;
        }

        /// <summary>
        /// サービス全追加
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_service_addAll_Click(object sender, RoutedEventArgs e)
        {
            ListBox target = tabItem_bs.IsSelected ? listBox_serviceBS :
                             tabItem_cs.IsSelected ? listBox_serviceCS :
                             tabItem_cs3.IsSelected ? listBox_serviceCS3 :
                             tabItem_tere.IsSelected ? listBox_serviceTere :
                             tabItem_other.IsSelected ? listBox_serviceOther : null;
            if (target != null)
            {
                foreach (ChSet5Item info in target.Items)
                {
                    if (listBox_serviceView.Items.Cast<ChSet5Item>().All(info2 => info2.Key != info.Key))
                    {
                        listBox_serviceView.Items.Add(info);
                    }
                }
            }
        }

        /// <summary>
        /// 映像のみ全追加
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_service_addVideo_Click(object sender, RoutedEventArgs e)
        {
            ListBox target = tabItem_bs.IsSelected ? listBox_serviceBS :
                             tabItem_cs.IsSelected ? listBox_serviceCS :
                             tabItem_cs3.IsSelected ? listBox_serviceCS3 :
                             tabItem_tere.IsSelected ? listBox_serviceTere :
                             tabItem_other.IsSelected ? listBox_serviceOther : null;
            if (target != null)
            {
                foreach (ChSet5Item info in target.Items)
                {
                    if (ChSet5.IsVideo(info.ServiceType) && listBox_serviceView.Items.Cast<ChSet5Item>().All(info2 => info2.Key != info.Key))
                    {
                        listBox_serviceView.Items.Add(info);
                    }
                }
            }
        }

        /// <summary>
        /// サービス１つ追加
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_service_add_Click(object sender, RoutedEventArgs e)
        {
            ListBox target = tabItem_bs.IsSelected ? listBox_serviceBS :
                             tabItem_cs.IsSelected ? listBox_serviceCS :
                             tabItem_cs3.IsSelected ? listBox_serviceCS3 :
                             tabItem_tere.IsSelected ? listBox_serviceTere :
                             tabItem_other.IsSelected ? listBox_serviceOther : null;
            if (target != null)
            {
                if (target.SelectedItem == null)
                {
                    MessageBox.Show("アイテムが選択されていません");
                    return;
                }
                if (listBox_serviceView.Items.Cast<ChSet5Item>().All(info => info.Key != ((ChSet5Item)target.SelectedItem).Key))
                {
                    listBox_serviceView.Items.Add(target.SelectedItem);
                }
            }
        }

        /// <summary>
        /// サービス１つ削除
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_service_del_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                listBox_serviceView.Items.RemoveAt(listBox_serviceView.SelectedIndex);
                if (listBox_serviceView.Items.Count > 0)
                {
                    listBox_serviceView.SelectedIndex = 0;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// サービス全削除
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_service_delAll_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                listBox_serviceView.Items.Clear();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 1つ上に移動
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_service_up_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listBox_serviceView.SelectedItem != null)
                {
                    if (listBox_serviceView.SelectedIndex >= 1)
                    {
                        object temp = listBox_serviceView.SelectedItem;
                        int index = listBox_serviceView.SelectedIndex;
                        listBox_serviceView.Items.RemoveAt(listBox_serviceView.SelectedIndex);
                        listBox_serviceView.Items.Insert(index - 1, temp);
                        listBox_serviceView.SelectedIndex = index - 1;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 1つ下に移動
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_service_down_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listBox_serviceView.SelectedItem != null)
                {
                    if (listBox_serviceView.SelectedIndex < listBox_serviceView.Items.Count - 1)
                    {
                        object temp = listBox_serviceView.SelectedItem;
                        int index = listBox_serviceView.SelectedIndex;
                        listBox_serviceView.Items.RemoveAt(listBox_serviceView.SelectedIndex);
                        listBox_serviceView.Items.Insert(index + 1, temp);
                        listBox_serviceView.SelectedIndex = index + 1;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// ジャンル全追加
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_jyanru_addAll_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                foreach (ContentKindInfo info in listBox_jyanru.Items)
                {
                    bool find = false;
                    foreach (ContentKindInfo info2 in listBox_jyanruView.Items)
                    {
                        if (info2.ID == info.ID)
                        {
                            find = true;
                            break;
                        }
                    }
                    if (find == false)
                    {
                        listBox_jyanruView.Items.Add(info);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// ジャンル１つ追加
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_jyanru_add_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listBox_jyanru.SelectedItem != null)
                {
                    ContentKindInfo info = listBox_jyanru.SelectedItem as ContentKindInfo;
                    bool find = false;
                    foreach (ContentKindInfo info2 in listBox_jyanruView.Items)
                    {
                        if (info2.ID == info.ID)
                        {
                            find = true;
                            break;
                        }
                    }
                    if (find == false)
                    {
                        listBox_jyanruView.Items.Add(info);
                    }
                }
                else
                {
                    MessageBox.Show("アイテムが選択されていません");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// ジャンル１つ削除
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_jyanru_del_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listBox_jyanruView.SelectedItem != null)
                {
                    listBox_jyanruView.Items.RemoveAt(listBox_jyanruView.SelectedIndex);
                    if (listBox_jyanruView.Items.Count > 0)
                    {
                        listBox_jyanruView.SelectedIndex = 0;
                    }
                }
                else
                {
                    MessageBox.Show("アイテムが選択されていません");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// ジャンル全削除
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_jyanru_delAll_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                listBox_jyanruView.Items.Clear();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
        
        private void listBox_serviceView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            textBox_serviceView1.Text = "";
            if (listBox_serviceView.SelectedItem == null)
            {
                return;
            }
            ChSet5Item info = listBox_serviceView.SelectedItem as ChSet5Item;

            textBox_serviceView1.Text = info.NetworkName + "\r\n";
            textBox_serviceView1.Text += "OriginalNetworkID : " + info.ONID.ToString() + " (0x" + info.ONID.ToString("X4") + ")\r\n";
            textBox_serviceView1.Text += "TransportStreamID : " + info.TSID.ToString() + " (0x" + info.TSID.ToString("X4") + ")\r\n";
            textBox_serviceView1.Text += "ServiceID : " + info.SID.ToString() + " (0x" + info.SID.ToString("X4") + ")" +
                                         (ChSet5.IsCS3(info.ONID) ? " " + (info.SID & 0x3FF) + "ch" : "") + "\r\n";
        }

        private void listBox_service_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            textBox_serviceView2.Text = "";
            if (((ListBox)sender).SelectedItem != null)
            {
                var info = (ChSet5Item)((ListBox)sender).SelectedItem;

                textBox_serviceView2.Text = info.NetworkName + "\r\n";
                textBox_serviceView2.Text += "OriginalNetworkID : " + info.ONID.ToString() + " (0x" + info.ONID.ToString("X4") + ")\r\n";
                textBox_serviceView2.Text += "TransportStreamID : " + info.TSID.ToString() + " (0x" + info.TSID.ToString("X4") + ")\r\n";
                textBox_serviceView2.Text += "ServiceID : " + info.SID.ToString() + " (0x" + info.SID.ToString("X4") + ")" +
                                             (ChSet5.IsCS3(info.ONID) ? " " + (info.SID & 0x3FF) + "ch" : "") + "\r\n";
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
                searchKey = dlg.GetSetting();
            }
        }

    }
}
