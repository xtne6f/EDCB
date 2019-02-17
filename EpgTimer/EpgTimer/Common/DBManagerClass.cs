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
                {
                    serviceEventList = new Dictionary<ulong, EpgServiceAllEventInfo>();
                    var list = new List<EpgServiceEventInfo>();
                    var list2 = new List<EpgServiceEventInfo>();
                    try
                    {
                        ret = ErrCode.CMD_ERR;
                        ret = CommonManager.CreateSrvCtrl().SendEnumPgAll(ref list);
                        if (ret == ErrCode.CMD_SUCCESS)
                        {
                            ret = ErrCode.CMD_ERR;
                            //過去5日分を取得してみる(仮対応)
                            CommonManager.CreateSrvCtrl().SendEnumPgArc(
                                new List<long> { 0xFFFFFFFFFFFF, 0xFFFFFFFFFFFF, DateTime.UtcNow.AddHours(9).AddDays(-5).ToFileTime(), long.MaxValue }, ref list2);
                            ret = ErrCode.CMD_SUCCESS;
                        }
                    }
                    catch { }
                    if (ret == ErrCode.CMD_SUCCESS)
                    {
                        foreach (EpgServiceEventInfo info in list)
                        {
                            ulong id = CommonManager.Create64Key(
                                info.serviceInfo.ONID,
                                info.serviceInfo.TSID,
                                info.serviceInfo.SID);
                            //対応する過去番組情報があれば付加する
                            int i = list2.FindIndex(info2 => id == CommonManager.Create64Key(info2.serviceInfo.ONID, info2.serviceInfo.TSID, info2.serviceInfo.SID));
                            serviceEventList.Add(id, new EpgServiceAllEventInfo(info.serviceInfo, info.eventList, i < 0 ? new List<EpgEventInfo>() : list2[i].eventList));
                        }
                        updateEpgData = false;
                    }
                }
                if (EpgDataChanged != null)
                {
                    EpgDataChanged();
                }
            }
            return ret;
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
