using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    /// <summary>
    /// EpgDataViewSetting.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgDataViewSetting : UserControl
    {
        private EpgSearchKeyInfo searchKey = new EpgSearchKeyInfo();
        private int tabInfoID = -1;

        public EpgDataViewSetting()
        {
            InitializeComponent();

            try
            {
                comboBox_timeH_week.ItemsSource = CommonManager.Instance.HourDictionary.Values;
                comboBox_timeH_week.SelectedIndex = 4;

                listBox_serviceDttv.ItemsSource = ChSet5.Instance.ChList.Values.Where(info => info.IsDttv == true);
                listBox_serviceBS.ItemsSource = ChSet5.Instance.ChList.Values.Where(info => info.IsBS == true);
                listBox_serviceCS.ItemsSource = ChSet5.Instance.ChList.Values.Where(info => info.IsCS == true);
                listBox_serviceOther.ItemsSource = ChSet5.Instance.ChList.Values.Where(info => info.IsOther == true);
                listBox_serviceAll.ItemsSource = ChSet5.Instance.ChList.Values;

                listBox_jyanru.ItemsSource = CommonManager.Instance.ContentKindList;

                radioButton_rate.IsChecked = true;
                radioButton_week.IsChecked = false;
                radioButton_list.IsChecked = false;

                listBox_Button_Set();
                listBox_serviceView_ContextMenu_Set();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        /// <summary>
        /// デフォルト表示の設定値
        /// </summary>
        /// <param name="setInfo"></param>
        public void SetSetting(CustomEpgTabInfo setInfo)
        {
            tabInfoID = setInfo.ID;
            searchKey = setInfo.SearchKey.Clone();

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
            info.SearchKey = searchKey.Clone();
            info.ID = tabInfoID;

            info.ViewServiceList = listBox_serviceView.Items.OfType<ChSet5Item>().Select(item => item.Key).ToList();
            info.ViewContentKindList = listBox_jyanruView.Items.OfType<ContentKindInfo>().Select(item => item.ID).ToList();
        }

        private BoxExchangeEditor bxs = new BoxExchangeEditor();
        private BoxExchangeEditor bxj = new BoxExchangeEditor();
        private void listBox_Button_Set()
        {
            bxs.TargetBox = this.listBox_serviceView;
            bxs.KeyActionAllow();
            bxs.DoubleClickMoveAllow();

            //サービス選択関係はソースの ListBox が複数あるので、全ての ListBoxItem にイベントを追加する。
            foreach (TabItem tab in tab_ServiceList.Items)
            {
                if (tab.Content is ListBox)
                {
                    ListBox box = tab.Content as ListBox;
                    bxs.sourceBoxKeyEnable(box, bxs.button_add_Click);//button_service_add.Clickに追加があるなら、RaiseEventをあてる
                    bxs.doubleClickSetter(box, bxs.button_add_Click);
                }
            }
            //ソースのリストボックスは複数あるので、リストボックスが選択されたときに SourceBox の登録を行う
            tab_ServiceList.SelectionChanged += (sender, e) =>
            {
                try { bxs.SourceBox = ((sender as TabControl).SelectedItem as TabItem).Content as ListBox; }
                catch { bxs.SourceBox = null; }
            };
            button_service_addAll.Click += new RoutedEventHandler(bxs.button_addAll_Click);
            button_service_add.Click += new RoutedEventHandler(bxs.button_add_Click);
            button_service_ins.Click += new RoutedEventHandler(bxs.button_insert_Click);
            button_service_del.Click += new RoutedEventHandler(bxs.button_del_Click);
            button_service_delAll.Click += new RoutedEventHandler(bxs.button_delAll_Click);
            button_service_top.Click += new RoutedEventHandler(bxs.button_top_Click);
            button_service_up.Click += new RoutedEventHandler(bxs.button_up_Click);
            button_service_down.Click += new RoutedEventHandler(bxs.button_down_Click);
            button_service_bottom.Click += new RoutedEventHandler(bxs.button_bottom_Click);

            //ジャンル選択関係
            bxj.SourceBox = this.listBox_jyanru;
            bxj.TargetBox = this.listBox_jyanruView;
            bxj.KeyActionAllow();
            bxj.DoubleClickMoveAllow();
            button_jyanru_addAll.Click += new RoutedEventHandler(bxj.button_addAll_Click);
            button_jyanru_add.Click += new RoutedEventHandler(bxj.button_add_Click);
            button_jyanru_ins.Click += new RoutedEventHandler(bxj.button_insert_Click);
            button_jyanru_del.Click += new RoutedEventHandler(bxj.button_del_Click);
            button_jyanru_delAll.Click += new RoutedEventHandler(bxj.button_delAll_Click);
        }

        List<Tuple<int, int>> sortList;
        private void listBox_serviceView_ContextMenu_Set()
        {
            // 右クリックメニューにSIDのソートを登録
            var cm = new ContextMenu();
            var menuItemAsc = new MenuItem();
            menuItemAsc.Header = "サブチャンネルの結合表示を解除";
            menuItemAsc.ToolTip = "同一TSIDのサービスの結合表示が解除されるようServiceIDを昇順に並び替えます";
            menuItemAsc.Tag = 0;
            cm.Items.Insert(0, menuItemAsc);
            var menuItemDesc = new MenuItem();
            menuItemDesc.Header = "サブチャンネルを番組表で結合表示";
            menuItemDesc.ToolTip = "同一TSIDのサービスをServiceIDが逆順になるよう並べると番組表で結合表示される機能を使い、\r\nサブチャンネルを含めて1サービスの幅で表示します";
            menuItemDesc.Tag = 1;
            cm.Items.Insert(0, menuItemDesc);
            foreach (MenuItem item in cm.Items)
            {
                item.Click += listBox_serviceView_SidSort;
                ToolTipService.SetShowOnDisabled(item, true);
                ToolTipService.SetShowDuration(item, 20000);
            }
            listBox_serviceView.ContextMenu = cm;
            listBox_serviceView.ContextMenuOpening += listBox_serviceView_ContextMenu_Opening;
            listBox_serviceView.ContextMenuClosing += (s, e) => sortList = null;
        }

        private void listBox_serviceView_ContextMenu_Opening(object sender, ContextMenuEventArgs e)
        {
            var grpDic = new Dictionary<int, Tuple<int, int>>();
            var grpDicAdd = new Action<int, int>((first, end) =>
            {
                for (int i = first; i <= end; i++) grpDic.Add(i, new Tuple<int, int>(first, end));
            });
            var grpDicRemove = new Action<int, int>((first, end) =>
            {
                for (int i = first; i <= end; i++) grpDic.Remove(i);
            });

            //並べ替え可能なグループを抽出
            int itemIndex = 0, firstTsidIndex = 0;
            for (; itemIndex < listBox_serviceView.Items.Count - 1; itemIndex++)
            {
                // 同一TSIDが連続する部分を選択中の中から探す(散らばっているTSIDはまとめない)
                var a = listBox_serviceView.Items[itemIndex] as ChSet5Item;
                var b = listBox_serviceView.Items[itemIndex + 1] as ChSet5Item;
                if (a.TSID == b.TSID) continue;

                // 見つかった場合 firstTsidIndex < itemIndex になる
                if (itemIndex != firstTsidIndex) grpDicAdd(firstTsidIndex, itemIndex);
                firstTsidIndex = itemIndex + 1;
            }
            if (itemIndex != firstTsidIndex) grpDicAdd(firstTsidIndex, itemIndex);

            // 対象があるときのみソート可能になるようにする
            sortList = new List<Tuple<int, int>>();
            foreach (var item in listBox_serviceView.SelectedItems)
            {
                Tuple<int, int> data;
                if (grpDic.TryGetValue(listBox_serviceView.Items.IndexOf(item), out data) == true)
                {
                    sortList.Add(data);
                    grpDicRemove(data.Item1, data.Item2);
                }
            }

            bool isSortable = sortList.Count != 0;
            ((MenuItem)listBox_serviceView.ContextMenu.Items[0]).IsEnabled = isSortable;
            ((MenuItem)listBox_serviceView.ContextMenu.Items[1]).IsEnabled = isSortable;

            if (isSortable == false) return;

            //全選択時以外は選択状態を変更する
            if (listBox_serviceView.Items.Count == listBox_serviceView.SelectedItems.Count) return;

            listBox_serviceView.UnselectAll();
            foreach (var data in sortList)
            {
                for (int i = data.Item1; i <= data.Item2; i++)
                {
                    listBox_serviceView.SelectedItems.Add(listBox_serviceView.Items[i]);
                }
            }
        }

        //残りの追加イベント
        /// <summary>映像のみ全追加</summary>
        private void button_service_addVideo_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ListBox listBox = bxs.SourceBox;
                if (listBox == null) return;

                listBox.UnselectAll();
                foreach (ChSet5Item info in listBox.Items)
                {
                    if (info.IsVideo == true)
                    {
                        listBox.SelectedItems.Add(info);
                    }
                }
                button_service_add.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        private void listBox_serviceView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            Display_ServiceView(listBox_serviceView, textBox_serviceView1);
        }

        private void listBox_service_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            Display_ServiceView(bxs.SourceBox, textBox_serviceView2);
        }

        private void listBox_serviceView_SidSort(object sender, RoutedEventArgs e)
        {
            // 昇順・逆順用コンパレーター
            var comparerator = ((sender as MenuItem).Tag as int?) == 0 ?
                new Func<ushort, ushort, bool>((a, b) => a <= b) : new Func<ushort, ushort, bool>((a, b) => a >= b);

            // 実質、高々3つの並べ替えなので Bubble Sort で十分
            var sort = new Action<int, int>((start, end) =>
            {
                for (int i = start; i < end; i++)
                {
                    for (int j = i + 1; j <= end; j++)
                    {
                        if (comparerator((listBox_serviceView.Items[j] as ChSet5Item).SID, (listBox_serviceView.Items[i] as ChSet5Item).SID))
                        {
                            var tmp = listBox_serviceView.Items[i];
                            listBox_serviceView.Items[i] = listBox_serviceView.Items[j];
                            listBox_serviceView.Items[j] = tmp;
                        }
                    }
                }
            });

            //選択状態を保存
            var listKeeper = new ListViewSelectedKeeper(listBox_serviceView, true, item => (item as ChSet5Item).Key);

            //ソート
            sortList.ForEach(data => sort(data.Item1,data.Item2));

            //選択状態を復元
            listKeeper.RestoreListViewSelected();
        }

        private void Display_ServiceView(ListBox srclistBox, TextBox targetBox)
        {
            try
            {
                if (srclistBox == null || targetBox == null) return;

                targetBox.Text = "";
                if (srclistBox.SelectedItem == null) return;

                var info = (ChSet5Item)srclistBox.SelectedItems[srclistBox.SelectedItems.Count - 1];
                targetBox.Text = 
                    info.ServiceName + "\r\n" +
                    info.NetworkName + "\r\n" +
                    "OriginalNetworkID : " + info.ONID.ToString() + " (0x" + info.ONID.ToString("X4") + ")\r\n" +
                    "TransportStreamID : " + info.TSID.ToString() + " (0x" + info.TSID.ToString("X4") + ")\r\n" +
                    "ServiceID : " + info.SID.ToString() + " (0x" + info.SID.ToString("X4") + ")\r\n";
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        private void button_searchKey_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new SetDefSearchSettingWindow();
            dlg.Owner = CommonUtil.GetTopWindow(this);
            dlg.SetDefSetting(searchKey);

            if (dlg.ShowDialog() == true)
            {
                searchKey = dlg.GetSetting();
            }
        }
    }
}
