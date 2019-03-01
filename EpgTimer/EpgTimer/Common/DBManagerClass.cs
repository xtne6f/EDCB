using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;

namespace EpgTimer
{
    class EpgServiceAllEventInfo
    {
        public readonly EpgServiceInfo serviceInfo;
        public readonly List<EpgEventInfo> eventList;
        public readonly List<EpgEventInfo> eventArcList;
        public EpgServiceAllEventInfo(EpgServiceInfo serviceInfo, List<EpgEventInfo> eventList, List<EpgEventInfo> eventArcList)
        {
            this.serviceInfo = serviceInfo;
            this.eventList = eventList;
            this.eventArcList = eventArcList;
        }
        public EpgServiceAllEventInfo(EpgServiceInfo serviceInfo)
            : this(serviceInfo, new List<EpgEventInfo>(), new List<EpgEventInfo>())
        {
        }
    }

    class DBManager
    {
        private bool updateEpgData = true;
        private bool updateReserveInfo = true;
        private bool updateRecInfo = true;
        private bool updateAutoAddEpgInfo = true;
        private bool updateAutoAddManualInfo = true;

        Dictionary<UInt64, EpgServiceAllEventInfo> serviceEventList = new Dictionary<UInt64, EpgServiceAllEventInfo>();
        Dictionary<UInt32, ReserveData> reserveList = new Dictionary<UInt32, ReserveData>();
        Dictionary<UInt32, TunerReserveInfo> tunerReserveList = new Dictionary<UInt32, TunerReserveInfo>();
        Dictionary<UInt32, RecFileInfo> recFileInfo = new Dictionary<UInt32, RecFileInfo>();
        Dictionary<UInt32, ManualAutoAddData> manualAutoAddList = new Dictionary<UInt32, ManualAutoAddData>();
        Dictionary<UInt32, EpgAutoAddData> epgAutoAddList = new Dictionary<UInt32, EpgAutoAddData>();

        public Dictionary<UInt64, EpgServiceAllEventInfo> ServiceEventList
        {
            get { return serviceEventList; }
        }
        /// <summary>
        /// ServiceEventListの過去番組情報の取得下限
        /// </summary>
        public DateTime EventBaseTime
        {
            get;
            private set;
        }
        /// <summary>
        /// 全番組情報の最小開始時間
        /// </summary>
        public DateTime EventMinTime
        {
            get;
            private set;
        }
        public Dictionary<UInt32, ReserveData> ReserveList
        {
            get { return reserveList; }
        }
        public Dictionary<UInt32, TunerReserveInfo> TunerReserveList
        {
            get { return tunerReserveList; }
        }
        public RecSettingData DefaultRecSetting
        {
            get;
            private set;
        }
        public Dictionary<UInt32, RecFileInfo> RecFileInfo
        {
            get { return recFileInfo; }
        }
        public Dictionary<UInt32, ManualAutoAddData> ManualAutoAddList
        {
            get { return manualAutoAddList; }
        }
        public Dictionary<UInt32, EpgAutoAddData> EpgAutoAddList
        {
            get { return epgAutoAddList; }
        }

        public event Action EpgDataChanged;
        public event Action ReserveInfoChanged;

        /// <summary>
        /// データの更新があったことを通知
        /// </summary>
        /// <param name="updateInfo">[IN]更新のあったデータのフラグ</param>
        public void SetUpdateNotify(UpdateNotifyItem updateInfo)
        {
            if (updateInfo == UpdateNotifyItem.EpgData)
            {
                updateEpgData = true;
            }
            if (updateInfo == UpdateNotifyItem.ReserveInfo)
            {
                updateReserveInfo = true;
            }
            if (updateInfo == UpdateNotifyItem.RecInfo)
            {
                updateRecInfo = true;
            }
            if (updateInfo == UpdateNotifyItem.AutoAddEpgInfo)
            {
                updateAutoAddEpgInfo = true;
            }
            if (updateInfo == UpdateNotifyItem.AutoAddManualInfo)
            {
                updateAutoAddManualInfo = true;
            }
        }

        /// <summary>
        /// EPGデータの更新があれば再読み込みする
        /// </summary>
        /// <returns></returns>
        public ErrCode ReloadEpgData()
        {
            ErrCode ret = ErrCode.CMD_SUCCESS;
            if (updateEpgData)
            {
                DateTime now = DateTime.UtcNow.AddHours(9);
                //3日以上前の日曜0時
                EventBaseTime = now.AddDays(-(int)now.AddDays(-3).DayOfWeek - 3) - now.TimeOfDay;
                EventMinTime = DateTime.MaxValue;
                serviceEventList = new Dictionary<ulong, EpgServiceAllEventInfo>();
                var list = new List<EpgServiceEventInfo>();
                var arcList = new List<EpgServiceEventInfo>();
                try
                {
                    ret = ErrCode.CMD_ERR;
                    ret = CommonManager.CreateSrvCtrl().SendEnumPgAll(ref list);
                    //SendEnumPgAll()は番組情報未取得状態でCMD_ERRを返す。従来エラー扱いだったが、取得数0の成功とみなす
                    if (ret == ErrCode.CMD_SUCCESS || ret == ErrCode.CMD_ERR)
                    {
                        ret = ErrCode.CMD_ERR;
                        var mm = new List<long>();
                        if (CommonManager.CreateSrvCtrl().SendGetPgArcMinMax(new List<long> { 0xFFFFFFFFFFFF, 0xFFFFFFFFFFFF }, ref mm) == ErrCode.CMD_SUCCESS)
                        {
                            if (mm[0] != long.MaxValue)
                            {
                                //全過去番組情報の最小開始時間
                                EventMinTime = DateTime.FromFileTimeUtc(mm[0]);
                                CommonManager.CreateSrvCtrl().SendEnumPgArc(
                                    new List<long> { 0xFFFFFFFFFFFF, 0xFFFFFFFFFFFF, EventBaseTime.ToFileTime(), now.AddDays(1).ToFileTime() }, ref arcList);
                            }
                        }
                        ret = ErrCode.CMD_SUCCESS;
                    }
                }
                catch { }

                if (ret == ErrCode.CMD_SUCCESS)
                {
                    foreach (EpgServiceEventInfo info in list)
                    {
                        ulong id = CommonManager.Create64Key(info.serviceInfo.ONID, info.serviceInfo.TSID, info.serviceInfo.SID);
                        //対応する過去番組情報があれば付加する
                        int i = arcList.FindIndex(a => id == CommonManager.Create64Key(a.serviceInfo.ONID, a.serviceInfo.TSID, a.serviceInfo.SID));
                        serviceEventList.Add(id, new EpgServiceAllEventInfo(info.serviceInfo, info.eventList, i < 0 ? new List<EpgEventInfo>() : arcList[i].eventList));
                        foreach (EpgEventInfo eventInfo in info.eventList)
                        {
                            if (eventInfo.StartTimeFlag != 0)
                            {
                                EventMinTime = eventInfo.start_time > EventMinTime ? EventMinTime : eventInfo.start_time;
                            }
                        }
                    }
                    foreach (EpgServiceEventInfo info in arcList)
                    {
                        ulong id = CommonManager.Create64Key(info.serviceInfo.ONID, info.serviceInfo.TSID, info.serviceInfo.SID);
                        if (serviceEventList.ContainsKey(id) == false)
                        {
                            //過去番組情報のみ存在
                            serviceEventList.Add(id, new EpgServiceAllEventInfo(info.serviceInfo, new List<EpgEventInfo>(), info.eventList));
                        }
                    }
                    updateEpgData = false;
                }
                if (EpgDataChanged != null)
                {
                    EpgDataChanged();
                }
            }
            return ret;
        }

        /// <summary>
        /// baseTimeから1週間分(EventBaseTimeをしきい値とし、このとき上限なし)のEPGデータを読み込む
        /// </summary>
        public ErrCode LoadWeeklyEpgData(DateTime baseTime, out Dictionary<ulong, EpgServiceAllEventInfo> list)
        {
            list = null;
            List<EpgServiceEventInfo> arcList = null;
            ErrCode ret = ReloadEpgData();
            if (ret == ErrCode.CMD_SUCCESS)
            {
                baseTime = baseTime > EventBaseTime ? EventBaseTime : baseTime;
                if (baseTime < EventBaseTime)
                {
                    ret = ErrCode.CMD_ERR;
                    arcList = new List<EpgServiceEventInfo>();
                    try
                    {
                        //1週間分の過去番組情報
                        CommonManager.CreateSrvCtrl().SendEnumPgArc(
                            new List<long> { 0xFFFFFFFFFFFF, 0xFFFFFFFFFFFF, baseTime.ToFileTime(), baseTime.AddDays(7).ToFileTime() }, ref arcList);
                        ret = ErrCode.CMD_SUCCESS;
                    }
                    catch { }
                }
            }
            if (ret == ErrCode.CMD_SUCCESS)
            {
                list = new Dictionary<ulong, EpgServiceAllEventInfo>();
                foreach (var info in serviceEventList)
                {
                    if (arcList != null)
                    {
                        //対応する過去番組情報があれば付加する
                        int i = arcList.FindIndex(a => info.Key == CommonManager.Create64Key(a.serviceInfo.ONID, a.serviceInfo.TSID, a.serviceInfo.SID));
                        list.Add(info.Key, new EpgServiceAllEventInfo(info.Value.serviceInfo,
                            info.Value.eventList.Where(a => a.StartTimeFlag != 0 && a.start_time >= baseTime && a.start_time < baseTime.AddDays(7)).ToList(),
                            i < 0 ? new List<EpgEventInfo>() : arcList[i].eventList));
                    }
                    else
                    {
                        list.Add(info.Key, new EpgServiceAllEventInfo(info.Value.serviceInfo,
                            info.Value.eventList.Where(a => a.StartTimeFlag == 0 || a.start_time >= baseTime).ToList(),
                            info.Value.eventArcList));
                    }
                }
                if (arcList != null)
                {
                    foreach (EpgServiceEventInfo info in arcList)
                    {
                        ulong id = CommonManager.Create64Key(info.serviceInfo.ONID, info.serviceInfo.TSID, info.serviceInfo.SID);
                        if (list.ContainsKey(id) == false)
                        {
                            //過去番組情報のみ存在
                            list.Add(id, new EpgServiceAllEventInfo(info.serviceInfo, new List<EpgEventInfo>(), info.eventList));
                        }
                    }
                }
            }
            return ret;
        }

        private class EpgEventInfoComparer : IComparer<EpgEventInfo>
        {
            public int Compare(EpgEventInfo a, EpgEventInfo b)
            {
                return a.event_id - b.event_id;
            }
        }

        /// <summary>
        /// 指定IDの番組情報を取得する。できるだけServiceEventListを利用する
        /// </summary>
        public EpgEventInfo GetPgInfo(ushort onid, ushort tsid, ushort sid, ushort eventID, bool cacheOnly)
        {
            var eventInfo = new EpgEventInfo();
            EpgServiceAllEventInfo allInfo;
            if (serviceEventList.TryGetValue(CommonManager.Create64Key(onid, tsid, sid), out allInfo))
            {
                //過去でない番組情報は必ずID順になっている
                eventInfo.event_id = eventID;
                int index = allInfo.eventList.BinarySearch(eventInfo, new EpgEventInfoComparer());
                if (index >= 0)
                {
                    return allInfo.eventList[index];
                }
            }
            try
            {
                if (cacheOnly == false &&
                    CommonManager.CreateSrvCtrl().SendGetPgInfo(CommonManager.Create64PgKey(onid, tsid, sid, eventID), ref eventInfo) == ErrCode.CMD_SUCCESS)
                {
                    return eventInfo;
                }
            }
            catch { }
            return null;
        }

        /// <summary>
        /// 結果はCtrlCmdUtil.SendSearchPg()と同じだが、ServiceEventListを利用する
        /// </summary>
        public ErrCode SearchPg(List<EpgSearchKeyInfo> key, out List<EpgEventInfo> list)
        {
            list = null;
            List<EpgEventInfoMinimum> minList = null;
            ErrCode ret = ReloadEpgData();
            if (ret == ErrCode.CMD_SUCCESS)
            {
                ret = ErrCode.CMD_ERR;
                minList = new List<EpgEventInfoMinimum>();
                try
                {
                    ret = CommonManager.CreateSrvCtrl().SendSearchPgMinimum(key, ref minList);
                }
                catch { }
            }
            if (ret == ErrCode.CMD_SUCCESS)
            {
                list = new List<EpgEventInfo>(minList.Count);
                var eventInfo = new EpgEventInfo();
                var eventComparer = new EpgEventInfoComparer();
                foreach (EpgEventInfoMinimum info in minList)
                {
                    //番組情報はserviceEventListに存在するはず
                    EpgServiceAllEventInfo allInfo;
                    if (serviceEventList.TryGetValue(CommonManager.Create64Key(info.original_network_id, info.transport_stream_id, info.service_id), out allInfo))
                    {
                        //過去でない番組情報は必ずID順になっている
                        eventInfo.event_id = info.event_id;
                        int index = allInfo.eventList.BinarySearch(eventInfo, eventComparer);
                        if (index >= 0)
                        {
                            list.Add(allInfo.eventList[index]);
                        }
                    }
                }
            }
            return ret;
        }

        /// <summary>
        /// baseTimeから1週間分(EventBaseTimeをしきい値とし、このとき上限なし)のEPGデータを検索する
        /// </summary>
        public ErrCode SearchWeeklyEpgData(DateTime baseTime, EpgSearchKeyInfo key, out Dictionary<ulong, EpgServiceAllEventInfo> list)
        {
            list = null;
            List<EpgEventInfo> eventList;
            List<EpgEventInfo> arcList = null;
            ErrCode ret = SearchPg(new List<EpgSearchKeyInfo>() { key }, out eventList);
            if (ret == ErrCode.CMD_SUCCESS)
            {
                ret = ErrCode.CMD_ERR;
                baseTime = baseTime > EventBaseTime ? EventBaseTime : baseTime;
                arcList = new List<EpgEventInfo>();
                //1週間分の過去番組情報
                var param = new SearchPgParam();
                param.keyList = new List<EpgSearchKeyInfo>() { key };
                param.enumStart = baseTime.ToFileTime();
                param.enumEnd = baseTime.AddDays(baseTime < EventBaseTime ? 7 : 14).ToFileTime();
                try
                {
                    CommonManager.CreateSrvCtrl().SendSearchPgArc(param, ref arcList);
                    ret = ErrCode.CMD_SUCCESS;
                }
                catch { }
            }
            if (ret == ErrCode.CMD_SUCCESS)
            {
                list = new Dictionary<ulong, EpgServiceAllEventInfo>();
                //サービス毎のリストに変換
                foreach (EpgEventInfo info in eventList)
                {
                    if (baseTime < EventBaseTime ? (info.StartTimeFlag != 0 && info.start_time >= baseTime && info.start_time < baseTime.AddDays(7))
                                                 : (info.StartTimeFlag == 0 || info.start_time >= baseTime))
                    {
                        ulong id = CommonManager.Create64Key(info.original_network_id, info.transport_stream_id, info.service_id);
                        EpgServiceAllEventInfo allInfo;
                        if (list.TryGetValue(id, out allInfo) == false)
                        {
                            if (ChSet5.Instance.ChList.ContainsKey(id) == false)
                            {
                                //サービス情報ないので無効
                                continue;
                            }
                            allInfo = new EpgServiceAllEventInfo(CommonManager.ConvertChSet5To(ChSet5.Instance.ChList[id]));
                            if (serviceEventList.ContainsKey(id))
                            {
                                //リモコンキー情報を補完
                                allInfo.serviceInfo.remote_control_key_id = serviceEventList[id].serviceInfo.remote_control_key_id;
                            }
                            list.Add(id, allInfo);
                        }
                        allInfo.eventList.Add(info);
                    }
                }
                foreach (EpgEventInfo info in arcList)
                {
                    ulong id = CommonManager.Create64Key(info.original_network_id, info.transport_stream_id, info.service_id);
                    EpgServiceAllEventInfo allInfo;
                    if (list.TryGetValue(id, out allInfo) == false)
                    {
                        if (ChSet5.Instance.ChList.ContainsKey(id) == false)
                        {
                            //サービス情報ないので無効
                            continue;
                        }
                        allInfo = new EpgServiceAllEventInfo(CommonManager.ConvertChSet5To(ChSet5.Instance.ChList[id]));
                        if (serviceEventList.ContainsKey(id))
                        {
                            //リモコンキー情報を補完
                            allInfo.serviceInfo.remote_control_key_id = serviceEventList[id].serviceInfo.remote_control_key_id;
                        }
                        list.Add(id, allInfo);
                    }
                    allInfo.eventArcList.Add(info);
                }
            }
            return ret;
        }

        /// <summary>
        /// デフォルト表示のために番組情報のサービスを選択する
        /// </summary>
        public static IEnumerable<EpgServiceAllEventInfo> SelectServiceEventList(IEnumerable<EpgServiceAllEventInfo> sel)
        {
            if (Settings.Instance.ShowEpgCapServiceOnly)
            {
                sel = sel.Where(info =>
                    ChSet5.Instance.ChList.ContainsKey(CommonManager.Create64Key(info.serviceInfo.ONID, info.serviceInfo.TSID, info.serviceInfo.SID)) &&
                    ChSet5.Instance.ChList[CommonManager.Create64Key(info.serviceInfo.ONID, info.serviceInfo.TSID, info.serviceInfo.SID)].EpgCapFlag);
            }
            //リモコンキー優先のID順ソート。BSはなるべくSID順
            var bsmin = new Dictionary<ushort, ushort>();
            foreach (EpgServiceAllEventInfo info in sel)
            {
                if (ChSet5.IsBS(info.serviceInfo.ONID) &&
                    (bsmin.ContainsKey(info.serviceInfo.TSID) == false || bsmin[info.serviceInfo.TSID] > info.serviceInfo.SID))
                {
                    bsmin[info.serviceInfo.TSID] = info.serviceInfo.SID;
                }
            }
            sel = sel.OrderBy(info =>
                (ulong)(ChSet5.IsDttv(info.serviceInfo.ONID) ? (info.serviceInfo.remote_control_key_id + 255) % 256 : 0) << 48 |
                CommonManager.Create64Key(info.serviceInfo.ONID,
                                          (ChSet5.IsBS(info.serviceInfo.ONID) ? bsmin[info.serviceInfo.TSID] : info.serviceInfo.TSID),
                                          info.serviceInfo.SID));
            return sel;
        }

        /// <summary>
        /// 予約情報の更新があれば再読み込みする
        /// </summary>
        /// <returns></returns>
        public ErrCode ReloadReserveInfo()
        {
            ErrCode ret = ErrCode.CMD_SUCCESS;
            if (updateReserveInfo)
            {
                reserveList = new Dictionary<uint, ReserveData>();
                tunerReserveList = new Dictionary<uint, TunerReserveInfo>();
                DefaultRecSetting = null;
                var list = new List<ReserveData>();
                var list2 = new List<TunerReserveInfo>();
                try
                {
                    ret = ErrCode.CMD_ERR;
                    ret = CommonManager.CreateSrvCtrl().SendEnumReserve(ref list);
                    if (ret == ErrCode.CMD_SUCCESS)
                    {
                        ret = ErrCode.CMD_ERR;
                        ret = CommonManager.CreateSrvCtrl().SendEnumTunerReserve(ref list2);
                        if (ret == ErrCode.CMD_SUCCESS)
                        {
                            //デフォルト値の情報を取得する
                            var info = new ReserveData();
                            if (CommonManager.CreateSrvCtrl().SendGetReserve(0x7FFFFFFF, ref info) == ErrCode.CMD_SUCCESS)
                            {
                                DefaultRecSetting = info.RecSetting;
                            }
                        }
                    }
                }
                catch { }
                if (ret == ErrCode.CMD_SUCCESS)
                {
                    foreach (ReserveData info in list)
                    {
                        reserveList.Add(info.ReserveID, info);
                    }
                    foreach (TunerReserveInfo info in list2)
                    {
                        tunerReserveList.Add(info.tunerID, info);
                    }
                    updateReserveInfo = false;
                }
                if (ReserveInfoChanged != null)
                {
                    ReserveInfoChanged();
                }
            }
            return ret;
        }

        /// <summary>
        /// 予約情報の録画予定ファイル名のみ再読み込みする
        /// </summary>
        /// <returns></returns>
        public ErrCode ReloadReserveRecFileNameList()
        {
            ErrCode ret = ErrCode.CMD_SUCCESS;
            if (reserveList.Count > 0)
            {
                var list = new List<ReserveData>();
                try
                {
                    ret = ErrCode.CMD_ERR;
                    ret = CommonManager.CreateSrvCtrl().SendEnumReserve(ref list);
                }
                catch { }
                if (ret == ErrCode.CMD_SUCCESS)
                {
                    bool changed = false;
                    foreach (ReserveData info in list)
                    {
                        if (reserveList.ContainsKey(info.ReserveID))
                        {
                            if (reserveList[info.ReserveID].RecFileNameList.Count != info.RecFileNameList.Count)
                            {
                                reserveList[info.ReserveID].RecFileNameList = info.RecFileNameList;
                                changed = true;
                            }
                            else
                            {
                                for (int i = 0; i < info.RecFileNameList.Count; i++)
                                {
                                    if (reserveList[info.ReserveID].RecFileNameList[i] != info.RecFileNameList[i])
                                    {
                                        reserveList[info.ReserveID].RecFileNameList = info.RecFileNameList;
                                        changed = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (changed && ReserveInfoChanged != null)
                    {
                        ReserveInfoChanged();
                    }
                }
            }
            return ret;
        }

        /// <summary>
        /// 録画済み情報の更新があれば再読み込みする
        /// </summary>
        /// <returns></returns>
        public ErrCode ReloadrecFileInfo()
        {
            ErrCode ret = ErrCode.CMD_SUCCESS;
            if (updateRecInfo)
            {
                recFileInfo = new Dictionary<uint, RecFileInfo>();
                var list = new List<RecFileInfo>();
                try
                {
                    ret = ErrCode.CMD_ERR;
                    ret = CommonManager.CreateSrvCtrl().SendEnumRecInfoBasic(ref list);
                    if (ret == ErrCode.CMD_NON_SUPPORT)
                    {
                        ret = ErrCode.CMD_ERR;
                        ret = CommonManager.CreateSrvCtrl().SendEnumRecInfo(ref list);
                    }
                }
                catch { }
                if (ret == ErrCode.CMD_SUCCESS)
                {
                    foreach (RecFileInfo info in list)
                    {
                        recFileInfo.Add(info.ID, info);
                    }
                    updateRecInfo = false;
                }
            }
            return ret;
        }

        /// <summary>
        /// EPG自動予約登録情報の更新があれば再読み込みする
        /// </summary>
        /// <returns></returns>
        public ErrCode ReloadEpgAutoAddInfo()
        {
            ErrCode ret = ErrCode.CMD_SUCCESS;
            if (updateAutoAddEpgInfo)
            {
                epgAutoAddList = new Dictionary<uint, EpgAutoAddData>();
                var list = new List<EpgAutoAddData>();
                try
                {
                    ret = ErrCode.CMD_ERR;
                    ret = CommonManager.CreateSrvCtrl().SendEnumEpgAutoAdd(ref list);
                }
                catch { }
                if (ret == ErrCode.CMD_SUCCESS)
                {
                    foreach (EpgAutoAddData info in list)
                    {
                        epgAutoAddList.Add(info.dataID, info);
                    }
                    updateAutoAddEpgInfo = false;
                }
            }
            return ret;
        }


        /// <summary>
        /// 自動予約登録情報の更新があれば再読み込みする
        /// </summary>
        /// <returns></returns>
        public ErrCode ReloadManualAutoAddInfo()
        {
            ErrCode ret = ErrCode.CMD_SUCCESS;
            if (updateAutoAddManualInfo)
            {
                manualAutoAddList = new Dictionary<uint, ManualAutoAddData>();
                var list = new List<ManualAutoAddData>();
                try
                {
                    ret = ErrCode.CMD_ERR;
                    ret = CommonManager.CreateSrvCtrl().SendEnumManualAdd(ref list);
                }
                catch { }
                if (ret == ErrCode.CMD_SUCCESS)
                {
                    foreach (ManualAutoAddData info in list)
                    {
                        manualAutoAddList.Add(info.dataID, info);
                    }
                    updateAutoAddManualInfo = false;
                }
            }
            return ret;
        }

        public ReserveData GetNextReserve()
        {
            DateTime now = DateTime.UtcNow.AddHours(9);
            if (reserveList.Values.Any(a => a.RecSetting.RecMode != 5 && a.StartTime > now))
            {
                return reserveList.Values.Where(a => a.RecSetting.RecMode != 5 && a.StartTime > now)
                                         .Aggregate((a, b) => a.StartTime <= b.StartTime ? a : b);
            }
            return null;
        }
    }
}
