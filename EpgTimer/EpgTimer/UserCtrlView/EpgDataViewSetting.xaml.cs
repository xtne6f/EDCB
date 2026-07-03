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

            comboBox_timeH_week.ItemsSource = Enumerable.Range(0, 24);
            comboBox_timeH_week.SelectedIndex = 4;

            for (int i = 0; i < 5; i++)
            {
                ListBox target = i == 0 ? listBox_serviceTere :
                                 i == 1 ? listBox_serviceBS :
                                 i == 2 ? listBox_serviceCS :
                                 i == 3 ? listBox_serviceCS3 : listBox_serviceOther;
                target.Items.Add(new Tuple<string, ulong>("[" + ((TabItem)target.Parent).Header + "]",
                                                          (ulong)(i == 0 ? CustomEpgTabInfo.SpecialViewServices.ViewServiceDttv :
                                                                  i == 1 ? CustomEpgTabInfo.SpecialViewServices.ViewServiceBS :
                                                                  i == 2 ? CustomEpgTabInfo.SpecialViewServices.ViewServiceCS :
                                                                  i == 3 ? CustomEpgTabInfo.SpecialViewServices.ViewServiceCS3 :
                                                                  CustomEpgTabInfo.SpecialViewServices.ViewServiceOther)));
                foreach (ChSet5Item info in ChSet5.Instance.ChListSelected)
                {
                    if (i == 0 && ChSet5.IsDttv(info.ONID) ||
                        i == 1 && ChSet5.IsBS(info.ONID) ||
                        i == 2 && ChSet5.IsCS(info.ONID) ||
                        i == 3 && ChSet5.IsCS3(info.ONID) ||
                        i == 4 && ChSet5.IsOther(info.ONID))
                    {
                        target.Items.Add(new Tuple<string, ulong>(info.ServiceName, info.Key));
                    }
                }
            }
            foreach (ushort id in CommonManager.Instance.ContentKindList)
            {
                listBox_jyanru.Items.Add(new ContentKindInfo() { Nibble1 = (byte)(id >> 8), Nibble2 = (byte)id });
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
            radioButton_rate.IsChecked = setInfo.ViewMode != 1 && setInfo.ViewMode != 2;
            radioButton_week.IsChecked = setInfo.ViewMode == 1;
            radioButton_list.IsChecked = setInfo.ViewMode == 2;
            radioButton_designDefault.IsChecked = setInfo.EpgSettingIndex != 1 && setInfo.EpgSettingIndex != 2;
            radioButton_designSub1.IsChecked = setInfo.EpgSettingIndex == 1;
            radioButton_designSub2.IsChecked = setInfo.EpgSettingIndex == 2;

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

            checkBox_highlightContentKind.IsChecked = setInfo.HighlightContentKind;
            checkBox_searchMode.IsChecked = setInfo.SearchMode;

            foreach (ulong id in setInfo.ViewServiceList)
            {
                listBox_serviceView.Items.Add(
                    id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceDttv ? listBox_serviceTere.Items[0] :
                    id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceBS ? listBox_serviceBS.Items[0] :
                    id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceCS ? listBox_serviceCS.Items[0] :
                    id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceCS3 ? listBox_serviceCS3.Items[0] :
                    id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceOther ? listBox_serviceOther.Items[0] :
                    new Tuple<string, ulong>(ChSet5.Instance.ChList.ContainsKey(id) ? ChSet5.Instance.ChList[id].ServiceName : "???", id));
            }
            foreach (ushort id in setInfo.ViewContentKindList)
            {
                listBox_jyanruView.Items.Add(listBox_jyanru.Items.Cast<ContentKindInfo>().FirstOrDefault(info => info.ID == id) ??
                                             new ContentKindInfo() { Nibble1 = (byte)(id >> 8), Nibble2 = (byte)id });
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
            info.ViewMode = radioButton_week.IsChecked == true ? 1 :
                            radioButton_list.IsChecked == true ? 2 : 0;
            info.EpgSettingIndex = radioButton_designSub1.IsChecked == true ? 1 :
                                   radioButton_designSub2.IsChecked == true ? 2 : 0;
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

            info.HighlightContentKind = checkBox_highlightContentKind.IsChecked == true;
            info.SearchMode = checkBox_searchMode.IsChecked == true;

            if (checkBox_filterEnded.IsChecked == true)
            {
                info.FilterEnded = true;
            }
            else
            {
                info.FilterEnded = false;
            }

            info.SearchKey = searchKey;

            foreach (Tuple<string, ulong> item in listBox_serviceView.Items)
            {
                info.ViewServiceList.Add(item.Item2);
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
                foreach (var info in target.Items.Cast<Tuple<string, ulong>>().Skip(1))
                {
                    if (listBox_serviceView.Items.Cast<Tuple<string, ulong>>().All(info2 => info2.Item2 != info.Item2))
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
                foreach (var info in target.Items.Cast<Tuple<string, ulong>>().Skip(1))
                {
                    if (ChSet5.Instance.ChList.ContainsKey(info.Item2) &&
                        ChSet5.IsVideo(ChSet5.Instance.ChList[info.Item2].ServiceType) &&
                        listBox_serviceView.Items.Cast<Tuple<string, ulong>>().All(info2 => info2.Item2 != info.Item2))
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
                var info = (Tuple<string, ulong>)target.SelectedItem;
                if (listBox_serviceView.Items.Cast<Tuple<string, ulong>>().All(info2 => info2.Item2 != info.Item2))
                {
                    listBox_serviceView.Items.Add(info);
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
            int index = listBox_serviceView.SelectedIndex;
            if (index >= 0)
            {
                listBox_serviceView.Items.RemoveAt(index);
                listBox_serviceView.SelectedIndex = Math.Min(index, listBox_serviceView.Items.Count - 1);
            }
        }

        /// <summary>
        /// サービス全削除
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_service_delAll_Click(object sender, RoutedEventArgs e)
        {
            listBox_serviceView.Items.Clear();
        }

        /// <summary>
        /// 1つ上に移動
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_service_up_Click(object sender, RoutedEventArgs e)
        {
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
        }

        /// <summary>
        /// 1つ下に移動
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_service_down_Click(object sender, RoutedEventArgs e)
        {
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
        }

        /// <summary>
        /// ジャンル全追加
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_jyanru_addAll_Click(object sender, RoutedEventArgs e)
        {
            foreach (ContentKindInfo info in listBox_jyanru.Items)
            {
                if (listBox_jyanruView.Items.Contains(info) == false)
                {
                    listBox_jyanruView.Items.Add(info);
                }
            }
        }

        /// <summary>
        /// ジャンル１つ追加
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_jyanru_add_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_jyanru.SelectedItem != null)
            {
                if (listBox_jyanruView.Items.Contains(listBox_jyanru.SelectedItem) == false)
                {
                    listBox_jyanruView.Items.Add(listBox_jyanru.SelectedItem);
                }
            }
            else
            {
                MessageBox.Show("アイテムが選択されていません");
            }
        }

        /// <summary>
        /// ジャンル１つ削除
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_jyanru_del_Click(object sender, RoutedEventArgs e)
        {
            int index = listBox_jyanruView.SelectedIndex;
            if (index >= 0)
            {
                listBox_jyanruView.Items.RemoveAt(index);
                listBox_jyanruView.SelectedIndex = Math.Min(index, listBox_jyanruView.Items.Count - 1);
            }
        }

        /// <summary>
        /// ジャンル全削除
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_jyanru_delAll_Click(object sender, RoutedEventArgs e)
        {
            listBox_jyanruView.Items.Clear();
        }

        private void listBox_service_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string text = "";
            if (((ListBox)sender).SelectedItem != null)
            {
                var info = (Tuple<string, ulong>)((ListBox)sender).SelectedItem;
                if (info.Item2 >> 48 != 0)
                {
                    text = info.Item1;
                }
                else
                {
                    ushort onid = (ushort)(info.Item2 >> 32);
                    ushort tsid = (ushort)(info.Item2 >> 16);
                    ushort sid = (ushort)info.Item2;
                    text = (ChSet5.Instance.ChList.ContainsKey(info.Item2) ? ChSet5.Instance.ChList[info.Item2].NetworkName : "???") + "\r\n";
                    text += "OriginalNetworkID : " + onid + " (0x" + onid.ToString("X4") + ")\r\n";
                    text += "TransportStreamID : " + tsid + " (0x" + tsid.ToString("X4") + ")\r\n";
                    text += "ServiceID : " + sid + " (0x" + sid.ToString("X4") + ")" + (ChSet5.IsCS3(onid) ? " " + (sid & 0x3FF) + "ch" : "");
                }
            }
            (sender == listBox_serviceView ? textBox_serviceView1 : textBox_serviceView2).Text = text;
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
