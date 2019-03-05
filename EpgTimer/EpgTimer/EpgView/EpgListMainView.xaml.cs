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
using System.ComponentModel;
using System.Collections.ObjectModel;
using System.Collections;
using System.Text.RegularExpressions;



namespace EpgTimer
{
    /// <summary>
    /// EpgListMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgListMainView : UserControl
    {
        public event Action<object, CustomEpgTabInfo, object> ViewModeChangeRequested;

        private CustomEpgTabInfo setViewInfo = null;
        private DateTime baseTime = DateTime.MaxValue;

        string _lastHeaderClicked = null;
        ListSortDirection _lastDirection = ListSortDirection.Ascending;
        string _lastHeaderClicked2 = null;
        ListSortDirection _lastDirection2 = ListSortDirection.Ascending;

        private Dictionary<UInt64, EpgServiceAllEventInfo> serviceEventList = new Dictionary<UInt64, EpgServiceAllEventInfo>();

        private bool updateEpgData = true;

        private Dictionary<ulong, bool> lastChkSID = new Dictionary<ulong, bool>();

        public EpgListMainView(CustomEpgTabInfo setInfo)
        {
            InitializeComponent();

            setViewInfo = setInfo;
        }

        /// <summary>
        /// 保持情報のクリア
        /// </summary>
        public void ClearInfo()
        {
            if (listBox_service.ItemsSource != null)
            {
                foreach (ServiceItem info in listBox_service.ItemsSource)
                {
                    lastChkSID[info.ID] = info.IsSelected;
                }
            }
            listBox_service.ItemsSource = null;
            listView_event.ItemsSource = null;
            serviceEventList.Clear();
            richTextBox_eventInfo.Document.Blocks.Clear();
            button_prev.IsEnabled = false;
            button_next.IsEnabled = false;
        }

        public bool HasService(ushort onid, ushort tsid, ushort sid)
        {
            return setViewInfo.ViewServiceList.Contains(CommonManager.Create64Key(onid, tsid, sid)) ||
                   setViewInfo.ViewServiceList.Contains(
                       (ulong)(ChSet5.IsDttv(onid) ? CustomEpgTabInfo.SpecialViewServices.ViewServiceDttv :
                               ChSet5.IsBS(onid) ? CustomEpgTabInfo.SpecialViewServices.ViewServiceBS :
                               ChSet5.IsCS(onid) ? CustomEpgTabInfo.SpecialViewServices.ViewServiceCS :
                               ChSet5.IsCS3(onid) ? CustomEpgTabInfo.SpecialViewServices.ViewServiceCS3 :
                               CustomEpgTabInfo.SpecialViewServices.ViewServiceOther));
        }

        /// <summary>
        /// 表示週変更
        /// </summary>
        void button_time_Click(object sender, RoutedEventArgs e)
        {
            DateTime lastTime = baseTime;
            baseTime = baseTime.AddDays(sender == button_prev ? -7 : 7);
            if (ReloadEpgData())
            {
                updateEpgData = false;
            }
            else
            {
                baseTime = lastTime;
            }
        }

        /// <summary>
        /// EPGデータ更新通知
        /// </summary>
        public void UpdateEpgData()
        {
            ClearInfo();
            updateEpgData = true;
            if (IsVisible || (Settings.Instance.NgAutoEpgLoadNW == false && Settings.Instance.PrebuildEpg))
            {
                if (ReloadEpgData() == true)
                {
                    updateEpgData = false;
                }
            }
        }

        /// <summary>
        /// 予約情報更新通知
        /// </summary>
        public void RefreshReserve()
        {
            if (listView_event.ItemsSource != null)
            {
                foreach (SearchItem item in listView_event.ItemsSource)
                {
                    if (item.Past == false)
                    {
                        item.ReserveInfo = null;
                        foreach (ReserveData info in CommonManager.Instance.DB.ReserveList.Values)
                        {
                            if (item.EventInfo.original_network_id == info.OriginalNetworkID &&
                                item.EventInfo.transport_stream_id == info.TransportStreamID &&
                                item.EventInfo.service_id == info.ServiceID &&
                                item.EventInfo.event_id == info.EventID)
                            {
                                item.ReserveInfo = info;
                                break;
                            }
                        }
                    }
                }
                CollectionViewSource.GetDefaultView(listView_event.ItemsSource).Refresh();
            }
        }

        private bool ReloadEpgData()
        {
            if (CommonManager.Instance.NWMode == false || CommonManager.Instance.NWConnectedIP != null)
            {
                Dictionary<ulong, EpgServiceAllEventInfo> list;
                ErrCode err;
                if (setViewInfo.SearchMode)
                {
                    err = CommonManager.Instance.DB.SearchWeeklyEpgData(baseTime, setViewInfo.SearchKey, out list);
                }
                else
                {
                    err = CommonManager.Instance.DB.LoadWeeklyEpgData(baseTime, out list);
                }
                if (err == ErrCode.CMD_SUCCESS)
                {
                    baseTime = baseTime > CommonManager.Instance.DB.EventBaseTime ? CommonManager.Instance.DB.EventBaseTime : baseTime;
                    serviceEventList = list;
                    ReloadProgramViewItem(baseTime > CommonManager.Instance.DB.EventMinTime, baseTime < CommonManager.Instance.DB.EventBaseTime);
                    return true;
                }
                if (IsVisible && err != ErrCode.CMD_ERR_BUSY)
                {
                    Dispatcher.BeginInvoke(new Action(() => MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "EPGデータの取得でエラーが発生しました。")));
                }
            }
            return false;
        }

        private void ReloadProgramViewItem(bool enablePrev, bool enableNext)
        {
            try
            {
                if (listBox_service.ItemsSource != null)
                {
                    foreach (ServiceItem info in listBox_service.ItemsSource)
                    {
                        lastChkSID[info.ID] = info.IsSelected;
                    }
                }
                listBox_service.ItemsSource = null;
                var serviceList = new List<ServiceItem>();

                //特殊なサービス指定の展開と重複除去
                var viewIDList = new List<ulong>();
                foreach (ulong id in setViewInfo.ViewServiceList)
                {
                    IEnumerable<EpgServiceAllEventInfo> sel =
                        id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceDttv ?
                            serviceEventList.Values.Where(info => ChSet5.IsDttv(info.serviceInfo.ONID)) :
                        id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceBS ?
                            serviceEventList.Values.Where(info => ChSet5.IsBS(info.serviceInfo.ONID)) :
                        id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceCS ?
                            serviceEventList.Values.Where(info => ChSet5.IsCS(info.serviceInfo.ONID)) :
                        id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceCS3 ?
                            serviceEventList.Values.Where(info => ChSet5.IsCS3(info.serviceInfo.ONID)) :
                        id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceOther ?
                            serviceEventList.Values.Where(info => ChSet5.IsOther(info.serviceInfo.ONID)) : null;
                    if (sel == null)
                    {
                        if (viewIDList.Contains(id) == false)
                        {
                            viewIDList.Add(id);
                        }
                        continue;
                    }
                    foreach (EpgServiceInfo info in DBManager.SelectServiceEventList(sel).Select(allInfo => allInfo.serviceInfo))
                    {
                        if (viewIDList.Contains(CommonManager.Create64Key(info.ONID, info.TSID, info.SID)) == false)
                        {
                            viewIDList.Add(CommonManager.Create64Key(info.ONID, info.TSID, info.SID));
                        }
                    }
                }
                for (int i = 0; i < viewIDList.Count;)
                {
                    //TSIDが同じでSIDが逆順のときは正順にする
                    int skip = i + 1;
                    while (viewIDList.Count > skip &&
                           viewIDList[skip] >> 16 == viewIDList[skip - 1] >> 16 &&
                           (viewIDList[skip] & 0xFFFF) < (viewIDList[skip - 1] & 0xFFFF))
                    {
                        skip++;
                    }
                    for (int j = skip - 1; j >= i; j--)
                    {
                        ulong id = viewIDList[j];
                        if (serviceEventList.ContainsKey(id))
                        {
                            var item = new ServiceItem(serviceEventList[id].serviceInfo);
                            item.IsSelected = lastChkSID.ContainsKey(id) == false || lastChkSID[id];
                            serviceList.Add(item);
                        }
                    }
                    i = skip;
                }
                listBox_service.ItemsSource = serviceList;
                button_prev.IsEnabled = enablePrev;
                button_next.IsEnabled = enableNext;

                UpdateEventList();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void UpdateEventList()
        {
            try
            {
                listView_event.ItemsSource = null;
                var programList = new List<SearchItem>();
                List<ushort> contentKindList = setViewInfo.ViewContentKindList.ToList();
                contentKindList.Sort();

                DateTime now = DateTime.UtcNow.AddHours(9);
                foreach (ServiceItem info in listBox_service.ItemsSource ?? Enumerable.Empty<object>())
                {
                    if (info.IsSelected == true)
                    {
                        EpgServiceAllEventInfo allInfo;
                        if (serviceEventList.TryGetValue(info.ID, out allInfo))
                        {
                            int eventInfoIndex = -1;
                            foreach (EpgEventInfo eventInfo in Enumerable.Concat(allInfo.eventArcList, allInfo.eventList))
                            {
                                bool past = ++eventInfoIndex < allInfo.eventArcList.Count;
                                if (eventInfo.StartTimeFlag == 0)
                                {
                                    //開始未定は除外
                                    continue;
                                }
                                if (setViewInfo.FilterEnded)
                                {
                                    if (eventInfo.start_time.AddSeconds(eventInfo.DurationFlag == 0 ? 0 : eventInfo.durationSec) < now)
                                        continue;
                                }
                                //ジャンル絞り込み
                                if (contentKindList.Count > 0)
                                {
                                    bool find = false;
                                    if (eventInfo.ContentInfo == null || eventInfo.ContentInfo.nibbleList.Count == 0)
                                    {
                                        //ジャンル情報ない
                                        find = contentKindList.BinarySearch(0xFFFF) >= 0;
                                    }
                                    else
                                    {
                                        foreach (EpgContentData contentInfo in eventInfo.ContentInfo.nibbleList)
                                        {
                                            int nibble1 = contentInfo.content_nibble_level_1;
                                            int nibble2 = contentInfo.content_nibble_level_2;
                                            if (nibble1 == 0x0E && nibble2 <= 0x01)
                                            {
                                                nibble1 = contentInfo.user_nibble_1 | (0x60 + nibble2 * 16);
                                                nibble2 = contentInfo.user_nibble_2;
                                            }
                                            if (contentKindList.BinarySearch((ushort)(nibble1 << 8 | 0xFF)) >= 0 ||
                                                contentKindList.BinarySearch((ushort)(nibble1 << 8 | nibble2)) >= 0)
                                            {
                                                find = true;
                                                break;
                                            }
                                        }
                                    }
                                    if (find == false)
                                    {
                                        //ジャンル見つからないので除外
                                        continue;
                                    }
                                }
                                //イベントグループのチェック
                                if (eventInfo.EventGroupInfo != null)
                                {
                                    bool spanFlag = false;
                                    foreach (EpgEventData data in eventInfo.EventGroupInfo.eventDataList)
                                    {
                                        if (info.ServiceInfo.ONID == data.original_network_id &&
                                            info.ServiceInfo.TSID == data.transport_stream_id &&
                                            info.ServiceInfo.SID == data.service_id)
                                        {
                                            spanFlag = true;
                                            break;
                                        }
                                    }

                                    if (spanFlag == false)
                                    {
                                        //サービス２やサービス３の結合されるべきもの
                                        continue;
                                    }
                                }

                                SearchItem item = new SearchItem(eventInfo, past);
                                item.ServiceName = info.ServiceInfo.service_name;
                                programList.Add(item);
                            }
                        }
                    }
                }
                listView_event.ItemsSource = programList;
                RefreshReserve();

                if (_lastHeaderClicked != null)
                {
                    //string header = ((Binding)_lastHeaderClicked.DisplayMemberBinding).Path.Path;
                    Sort(_lastHeaderClicked, _lastDirection);
                }
                else
                {
                    string header = ((Binding)gridView_event.Columns[1].DisplayMemberBinding).Path.Path;
                    Sort(header, _lastDirection);
                    _lastHeaderClicked = header;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void CheckBox_Checked(object sender, RoutedEventArgs e)
        {
            UpdateEventList();
        }

        private void CheckBox_Unchecked(object sender, RoutedEventArgs e)
        {
            UpdateEventList();
        }

        private void button_chkAll_Click(object sender, RoutedEventArgs e)
        {
            var serviceList = listBox_service.ItemsSource;
            if (serviceList != null)
            {
                listBox_service.ItemsSource = null;
                foreach (ServiceItem info in serviceList)
                {
                    info.IsSelected = true;
                }
                listBox_service.ItemsSource = serviceList;
                UpdateEventList();
            }
        }

        private void button_clearAll_Click(object sender, RoutedEventArgs e)
        {
            var serviceList = listBox_service.ItemsSource;
            if (serviceList != null)
            {
                listBox_service.ItemsSource = null;
                foreach (ServiceItem info in serviceList)
                {
                    info.IsSelected = false;
                }
                listBox_service.ItemsSource = serviceList;
                UpdateEventList();
            }
        }

        private void listView_event_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            richTextBox_eventInfo.Document.Blocks.Clear();
            if (listView_event.SelectedItem != null)
            {
                SearchItem item = listView_event.SelectedItem as SearchItem;

                EpgEventInfo eventInfo = item.EventInfo;

                String text = CommonManager.Instance.ConvertProgramText(eventInfo, EventInfoTextMode.All);

                int searchFrom = 0;
                Paragraph para = new Paragraph();
                string rtext = CommonManager.ReplaceText(text, CommonManager.Instance.ReplaceUrlDictionary);
                if (rtext.Length == text.Length)
                {
                    for (Match m = Regex.Match(rtext, @"https?://[0-9A-Za-z!#$%&'()~=@;:?_+\-*/.]+"); m.Success; m = m.NextMatch())
                    {
                        para.Inlines.Add(text.Substring(searchFrom, m.Index - searchFrom));
                        Hyperlink h = new Hyperlink(new Run(text.Substring(m.Index, m.Length)));
                        h.MouseLeftButtonDown += new MouseButtonEventHandler(h_MouseLeftButtonDown);
                        h.Foreground = Brushes.Blue;
                        h.Cursor = Cursors.Hand;
                        h.NavigateUri = new Uri(m.Value);
                        para.Inlines.Add(h);
                        searchFrom = m.Index + m.Length;
                    }
                }
                para.Inlines.Add(text.Substring(searchFrom));
                richTextBox_eventInfo.Document = new FlowDocument(para);
            }
        }

        void h_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            try
            {
                if (sender.GetType() == typeof(Hyperlink))
                {
                    Hyperlink h = sender as Hyperlink;
                    System.Diagnostics.Process.Start(h.NavigateUri.ToString());
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void listView_event_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            try
            {
                if (listView_event.SelectedItem != null)
                {
                    SearchItem item = listView_event.SelectedItem as SearchItem;
                    if (item.IsReserved == true)
                    {
                        ChangeReserve(item.ReserveInfo);
                    }
                    else
                    {
                        AddReserve(item.EventInfo, item.Past == false);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void ChangeReserve(ReserveData reserveInfo)
        {
            try{
                ChgReserveWindow dlg = new ChgReserveWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetOpenMode(Settings.Instance.EpgInfoOpenMode);
                dlg.SetReserveInfo(reserveInfo);
                if (dlg.ShowDialog() == true)
                {
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void AddReserve(EpgEventInfo eventInfo, bool reservable)
        {
            try
            {
                AddReserveEpgWindow dlg = new AddReserveEpgWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetOpenMode(Settings.Instance.EpgInfoOpenMode);
                dlg.SetReservable(reservable);
                dlg.SetEventInfo(eventInfo);
                if (dlg.ShowDialog() == true)
                {
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void listView_event_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            if (sender.GetType() == typeof(ListView))
            {
                try
                {
                    if (listView_event.SelectedItem != null)
                    {
                        SearchItem item = listView_event.SelectedItem as SearchItem;
                        if (item.IsReserved == true)
                        {
                            cm_new.IsEnabled = false;
                            cm_del.IsEnabled = true;
                            cm_chg.IsEnabled = true;
                            for (int i = 0; i <= 5; i++)
                            {
                                ((MenuItem)cm_chg.Items[cm_chg.Items.IndexOf(recmode_all) + i]).IsChecked = (i == item.ReserveInfo.RecSetting.RecMode);
                            }
                            for (int i = 0; i < cm_pri.Items.Count; i++)
                            {
                                ((MenuItem)cm_pri.Items[i]).IsChecked = (i + 1 == item.ReserveInfo.RecSetting.Priority);
                            }
                            cm_pri.Header = string.Format((string)cm_pri.Tag, item.ReserveInfo.RecSetting.Priority);
                            cm_add.IsEnabled = false;
                            cm_autoadd.IsEnabled = true;
                            cm_timeshift.IsEnabled = true;
                        }
                        else
                        {
                            cm_new.IsEnabled = item.Past == false;
                            cm_del.IsEnabled = false;
                            cm_chg.IsEnabled = false;
                            cm_add.IsEnabled = true;
                            cm_autoadd.IsEnabled = true;
                            cm_timeshift.IsEnabled = false;
                            for (int i = cm_add.Items.Count - 1; cm_add.Items[i] != cm_add_separator; i--)
                            {
                                cm_add.Items.RemoveAt(i);
                            }
                            foreach (RecPresetItem info in Settings.GetRecPresetList())
                            {
                                MenuItem menuItem = new MenuItem();
                                menuItem.Header = info.DisplayName;
                                menuItem.Tag = info.ID;
                                menuItem.Click += new RoutedEventHandler(cm_add_preset_Click);
                                menuItem.IsEnabled = item.Past == false;
                                cm_add.Items.Add(menuItem);
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }
            }
        }

        void cm_add_preset_Click(object sender, RoutedEventArgs e)
        {
            var meun = sender as MenuItem;
            if (meun != null)
            {
                AddReserveFromPreset((uint)meun.Tag);
            }
        }

        private void AddReserveFromPreset(uint presetID)
        {
            try
            {
                if (listView_event.SelectedItem == null)
                {
                    return;
                }

                SearchItem item = listView_event.SelectedItem as SearchItem;
                EpgEventInfo eventInfo = item.EventInfo;
                if (eventInfo.StartTimeFlag == 0)
                {
                    MessageBox.Show("開始時間未定のため予約できません");
                    return;
                }

                ReserveData reserveInfo = new ReserveData();
                if (eventInfo.ShortInfo != null)
                {
                    reserveInfo.Title = eventInfo.ShortInfo.event_name;
                }

                reserveInfo.StartTime = eventInfo.start_time;
                reserveInfo.StartTimeEpg = eventInfo.start_time;

                if (eventInfo.DurationFlag == 0)
                {
                    reserveInfo.DurationSecond = 10 * 60;
                }
                else
                {
                    reserveInfo.DurationSecond = eventInfo.durationSec;
                }

                UInt64 key = CommonManager.Create64Key(eventInfo.original_network_id, eventInfo.transport_stream_id, eventInfo.service_id);
                if (ChSet5.Instance.ChList.ContainsKey(key) == true)
                {
                    reserveInfo.StationName = ChSet5.Instance.ChList[key].ServiceName;
                }
                reserveInfo.OriginalNetworkID = eventInfo.original_network_id;
                reserveInfo.TransportStreamID = eventInfo.transport_stream_id;
                reserveInfo.ServiceID = eventInfo.service_id;
                reserveInfo.EventID = eventInfo.event_id;
                reserveInfo.RecSetting = Settings.CreateRecSetting(presetID);

                List<ReserveData> list = new List<ReserveData>();
                list.Add(reserveInfo);
                ErrCode err = CommonManager.CreateSrvCtrl().SendAddReserve(list);
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約登録でエラーが発生しました。終了時間がすでに過ぎている可能性があります。");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void cm_del_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                List<UInt32> list = new List<UInt32>();
                foreach(SearchItem item in listView_event.SelectedItems)
                {
                    if (item.IsReserved == true)
                    {
                        list.Add(item.ReserveInfo.ReserveID);
                    }
                }
                if (list.Count > 0)
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendDelReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約削除でエラーが発生しました。");
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void cm_change_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView_event.SelectedItem != null)
                {
                    SearchItem item = listView_event.SelectedItem as SearchItem;
                    if (item.IsReserved == true)
                    {
                        ChangeReserve(item.ReserveInfo);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void cm_add_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView_event.SelectedItem != null)
                {
                    SearchItem item = listView_event.SelectedItem as SearchItem;
                    if (item.IsReserved == false)
                    {
                        AddReserve(item.EventInfo, item.Past == false);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void cm_chg_recmode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                List<ReserveData> list = new List<ReserveData>();
                foreach (SearchItem item in listView_event.SelectedItems)
                {
                    if (item.IsReserved == true)
                    {
                        ReserveData reserveInfo = item.ReserveInfo;

                        byte recMode = 0;
                        if (sender == recmode_all)
                        {
                            recMode = 0;
                        }
                        else if (sender == recmode_only)
                        {
                            recMode = 1;
                        }
                        else if (sender == recmode_all_nodec)
                        {
                            recMode = 2;
                        }
                        else if (sender == recmode_only_nodec)
                        {
                            recMode = 3;
                        }
                        else if (sender == recmode_view)
                        {
                            recMode = 4;
                        }
                        else if (sender == recmode_no)
                        {
                            recMode = 5;
                        }
                        else
                        {
                            return;
                        }
                        reserveInfo.RecSetting.RecMode = recMode;
                        
                        list.Add(reserveInfo);
                    }
                }
                if (list.Count > 0)
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。");
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void cm_chg_priority_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                List<ReserveData> list = new List<ReserveData>();
                foreach (SearchItem item in listView_event.SelectedItems)
                {
                    if (item.IsReserved == true)
                    {
                        ReserveData reserveInfo = item.ReserveInfo;

                        byte priority = 1;
                        if (sender == priority_1)
                        {
                            priority = 1;
                        }
                        else if (sender == priority_2)
                        {
                            priority = 2;
                        }
                        else if (sender == priority_3)
                        {
                            priority = 3;
                        }
                        else if (sender == priority_4)
                        {
                            priority = 4;
                        }
                        else if (sender == priority_5)
                        {
                            priority = 5;
                        }
                        else
                        {
                            return;
                        }
                        reserveInfo.RecSetting.Priority = priority;

                        list.Add(reserveInfo);
                    }
                }
                if (list.Count > 0)
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。");
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void cm_autoadd_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (sender.GetType() != typeof(MenuItem))
                {
                    return;
                }
                if (listView_event.SelectedItem != null)
                {
                    SearchItem item = listView_event.SelectedItem as SearchItem;

                    SearchWindow dlg = new SearchWindow();
                    dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                    dlg.SetViewMode(1);

                    EpgSearchKeyInfo key = new EpgSearchKeyInfo();

                    if (item.EventInfo.ShortInfo != null)
                    {
                        key.andKey = item.EventInfo.ShortInfo.event_name;
                    }
                    Int64 sidKey = ((Int64)item.EventInfo.original_network_id) << 32 | ((Int64)item.EventInfo.transport_stream_id) << 16 | ((Int64)item.EventInfo.service_id);
                    key.serviceList.Add(sidKey);

                    dlg.SetSearchDefKey(key);
                    dlg.ShowDialog();                
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void cm_timeShiftPlay_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView_event.SelectedItem != null)
                {
                    SearchItem item = listView_event.SelectedItem as SearchItem;
                    if (item.IsReserved == true)
                    {
                        CommonManager.Instance.FilePlay(item.ReserveInfo.ReserveID);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 右クリックメニュー 表示設定イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_viewSet_Click(object sender, RoutedEventArgs e)
        {
            if (Settings.Instance.UseCustomEpgView == false)
            {
                MessageBox.Show("デフォルト表示では設定を変更することはできません。");
            }
            else
            {
                var dlg = new EpgDataViewSettingWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetDefSetting(setViewInfo);
                if (dlg.ShowDialog() == true)
                {
                    var setInfo = dlg.GetSetting();
                    if (setInfo.ViewMode == setViewInfo.ViewMode)
                    {
                        setViewInfo = setInfo;
                        UpdateEpgData();
                    }
                    else if (ViewModeChangeRequested != null)
                    {
                        ViewModeChangeRequested(this, setInfo, null);
                    }
                }
            }
        }

        /// <summary>
        /// 右クリックメニュー 表示モードイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_chg_viewMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (sender.GetType() != typeof(MenuItem))
                {
                    return;
                }
                if (ViewModeChangeRequested != null)
                {
                    MenuItem item = sender as MenuItem;
                    CustomEpgTabInfo setInfo = setViewInfo.DeepClone();
                    if (sender == cm_chg_viewMode2)
                    {
                        setInfo.ViewMode = 1;
                    }
                    else
                    {
                        setInfo.ViewMode = 0;
                    }
                    ViewModeChangeRequested(this, setInfo, null);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked = e.OriginalSource as GridViewColumnHeader;
            ListSortDirection direction;

            if (headerClicked != null)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {
                    string header = "Reserved";
                    if (headerClicked.Column.DisplayMemberBinding != null)
                    {
                        header = ((Binding)headerClicked.Column.DisplayMemberBinding).Path.Path;
                    }
                    if (header != _lastHeaderClicked)
                    {
                        direction = ListSortDirection.Ascending;
                        _lastHeaderClicked2 = _lastHeaderClicked;
                        _lastDirection2 = _lastDirection;
                    }
                    else
                    {
                        if (_lastDirection == ListSortDirection.Ascending)
                        {
                            direction = ListSortDirection.Descending;
                        }
                        else
                        {
                            direction = ListSortDirection.Ascending;
                        }
                    }

                    Sort(header, direction);

                    _lastHeaderClicked = header;
                    _lastDirection = direction;
                }
            }
        }

        private void Sort(string sortBy, ListSortDirection direction)
        {
            if (listView_event.ItemsSource == null)
            {
                return;
            }
            ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_event.ItemsSource);

            using (dataView.DeferRefresh())
            {
                dataView.SortDescriptions.Clear();

                SortDescription sd = new SortDescription(sortBy, direction);
                dataView.SortDescriptions.Add(sd);
                if (_lastHeaderClicked2 != null)
                {
                    if (sortBy != _lastHeaderClicked2)
                    {
                        SortDescription sd2 = new SortDescription(_lastHeaderClicked2, _lastDirection2);
                        dataView.SortDescriptions.Add(sd2);
                    }
                }
            }
        }

        /// <summary>
        /// 右クリックメニュー 簡易予約イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_new_Click(object sender, RoutedEventArgs e)
        {
            AddReserveFromPreset(0);
        }

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (IsVisible)
            {
                if (updateEpgData && ReloadEpgData())
                {
                    updateEpgData = false;
                }
            }
        }
    }
}
