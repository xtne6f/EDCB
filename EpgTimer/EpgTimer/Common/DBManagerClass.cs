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
        private bool updatePlugInFile = true;
        private bool noAutoReloadEpg = false;
        private bool oneTimeReloadEpg = false;

        Dictionary<UInt64, EpgServiceAllEventInfo> serviceEventList = new Dictionary<UInt64, EpgServiceAllEventInfo>();
        Dictionary<UInt32, ReserveData> reserveList = new Dictionary<UInt32, ReserveData>();
        Dictionary<UInt32, TunerReserveInfo> tunerReserveList = new Dictionary<UInt32, TunerReserveInfo>();
        Dictionary<UInt32, RecFileInfo> recFileInfo = new Dictionary<UInt32, RecFileInfo>();
        Dictionary<Int32, String> writePlugInList = new Dictionary<Int32, String>();
        Dictionary<Int32, String> recNamePlugInList = new Dictionary<Int32, String>();
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
        public Dictionary<UInt32, RecFileInfo> RecFileInfo
        {
            get { return recFileInfo; }
        }
        public Dictionary<Int32, String> WritePlugInList
        {
            get { return writePlugInList; }
        }
        public Dictionary<Int32, String> RecNamePlugInList
        {
            get { return recNamePlugInList; }
        }
        public Dictionary<UInt32, ManualAutoAddData> ManualAutoAddList
        {
            get { return manualAutoAddList; }
        }
        public Dictionary<UInt32, EpgAutoAddData> EpgAutoAddList
        {
            get { return epgAutoAddList; }
        }

        public void ClearAllDB()
        {
            serviceEventList.Clear();
            reserveList.Clear();
            tunerReserveList.Clear();
            recFileInfo.Clear();
            writePlugInList.Clear();
            recNamePlugInList.Clear();
            manualAutoAddList.Clear();
            epgAutoAddList.Clear();

            serviceEventList = null;
            serviceEventList = new Dictionary<ulong, EpgServiceAllEventInfo>();
            reserveList = null;
            reserveList = new Dictionary<uint, ReserveData>();
            tunerReserveList = null;
            tunerReserveList = new Dictionary<uint, TunerReserveInfo>();
            recFileInfo = null;
            recFileInfo = new Dictionary<uint, RecFileInfo>();
            writePlugInList = null;
            writePlugInList = new Dictionary<int, string>();
            recNamePlugInList = null;
            recNamePlugInList = new Dictionary<int, string>();
            manualAutoAddList = null;
            manualAutoAddList = new Dictionary<uint, ManualAutoAddData>();
            epgAutoAddList = null;
            epgAutoAddList = new Dictionary<uint, EpgAutoAddData>();
        }

        /// <summary>
        /// EPGデータの自動取得を行うかどうか（NW用）
        /// </summary>
        /// <param name="noReload"></param>
        public void SetNoAutoReloadEPG(bool noReload)
        {
            noAutoReloadEpg = noReload;
        }

        public void SetOneTimeReloadEpg()
        {
            oneTimeReloadEpg = true;
        }

        /// <summary>
        /// データの更新があったことを通知
        /// </summary>
        /// <param name="updateInfo">[IN]更新のあったデータのフラグ</param>
        public void SetUpdateNotify(UInt32 updateInfo)
        {
            if (updateInfo == (UInt32)UpdateNotifyItem.EpgData)
            {
                updateEpgData = true;
            }
            if (updateInfo == (UInt32)UpdateNotifyItem.ReserveInfo)
            {
                updateReserveInfo = true;
            }
            if (updateInfo == (UInt32)UpdateNotifyItem.RecInfo)
            {
                updateRecInfo = true;
            }
            if (updateInfo == (UInt32)UpdateNotifyItem.AutoAddEpgInfo)
            {
                updateAutoAddEpgInfo = true;
            }
            if (updateInfo == (UInt32)UpdateNotifyItem.AutoAddManualInfo)
            {
                updateAutoAddManualInfo = true;
            }
            if (updateInfo == (UInt32)UpdateNotifyItem.PlugInFile)
            {
                updatePlugInFile = true;
            }
        }

        /// <summary>
        /// EPGデータの更新があれば再読み込みする
        /// </summary>
        /// <returns></returns>
        public ErrCode ReloadEpgData()
        {
            ErrCode ret = ErrCode.CMD_SUCCESS;
            try
            {
                if (updateEpgData)
                {
                    if (noAutoReloadEpg == false || oneTimeReloadEpg)
                    {
                        {
                            serviceEventList.Clear();
                            serviceEventList = null;
                            serviceEventList = new Dictionary<ulong, EpgServiceAllEventInfo>();
                            List<EpgServiceEventInfo> list = new List<EpgServiceEventInfo>();
                            ret = CommonManager.CreateSrvCtrl().SendEnumPgAll(ref list);
                            if (ret == ErrCode.CMD_SUCCESS)
                            {
                                var list2 = new List<EpgServiceEventInfo>();
                                CommonManager.CreateSrvCtrl().SendEnumPgArcAll(ref list2);
                                foreach (EpgServiceEventInfo info in list)
                                {
                                    UInt64 id = CommonManager.Create64Key(
                                        info.serviceInfo.ONID,
                                        info.serviceInfo.TSID,
                                        info.serviceInfo.SID);
                                    //対応する過去番組情報があれば付加する
                                    int i = list2.FindIndex(info2 => id == CommonManager.Create64Key(info2.serviceInfo.ONID, info2.serviceInfo.TSID, info2.serviceInfo.SID));
                                    serviceEventList.Add(id, new EpgServiceAllEventInfo(info.serviceInfo, info.eventList, i < 0 ? new List<EpgEventInfo>() : list2[i].eventList));
                                }
                                updateEpgData = false;
                                oneTimeReloadEpg = false;
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
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
            try
            {
                if (updateReserveInfo == true)
                {
                    {
                        reserveList.Clear();
                        reserveList = null;
                        reserveList = new Dictionary<uint, ReserveData>();
                        tunerReserveList.Clear();
                        tunerReserveList = null;
                        tunerReserveList = new Dictionary<uint, TunerReserveInfo>();
                        List<ReserveData> list = new List<ReserveData>();
                        List<TunerReserveInfo> list2 = new List<TunerReserveInfo>();
                        ret = CommonManager.CreateSrvCtrl().SendEnumReserve(ref list);
                        if (ret == ErrCode.CMD_SUCCESS)
                        {
                            ret = CommonManager.CreateSrvCtrl().SendEnumTunerReserve(ref list2);
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
                        }
                        list.Clear();
                        list2.Clear();
                        list = null;
                        list2 = null;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
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
            try
            {
                if (updateRecInfo == true)
                {
                    {
                        recFileInfo.Clear();
                        recFileInfo = null;
                        recFileInfo = new Dictionary<uint, RecFileInfo>();
                        List<RecFileInfo> list = new List<RecFileInfo>();
                        ret = CommonManager.CreateSrvCtrl().SendEnumRecInfoBasic(ref list);
                        if (ret == ErrCode.CMD_NON_SUPPORT)
                        {
                            ret = CommonManager.CreateSrvCtrl().SendEnumRecInfo(ref list);
                        }
                        if (ret == ErrCode.CMD_SUCCESS)
                        {
                            foreach (RecFileInfo info in list)
                            {
                                recFileInfo.Add(info.ID, info);
                            }
                            updateRecInfo = false;
                        }
                        list.Clear();
                        list = null;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return ret;
        }

        /// <summary>
        /// PlugInFileの再読み込み指定があればする
        /// </summary>
        /// <returns></returns>
        public ErrCode ReloadPlugInFile()
        {
            ErrCode ret = ErrCode.CMD_SUCCESS;
            try
            {
                if (updatePlugInFile == true)
                {
                    {
                        writePlugInList.Clear();
                        writePlugInList = null;
                        writePlugInList = new Dictionary<int, string>();
                        recNamePlugInList.Clear();
                        recNamePlugInList = null;
                        recNamePlugInList = new Dictionary<int, string>();
                        List<String> writeList = new List<string>();
                        List<String> recNameList = new List<string>();
                        ret = CommonManager.CreateSrvCtrl().SendEnumPlugIn(2, ref writeList);
                        if (ret == ErrCode.CMD_SUCCESS)
                        {
                            ret = CommonManager.CreateSrvCtrl().SendEnumPlugIn(1, ref recNameList);
                            if (ret == ErrCode.CMD_SUCCESS)
                            {
                                foreach (String info in writeList)
                                {
                                    writePlugInList.Add(writePlugInList.Count, info);
                                }
                                foreach (String info in recNameList)
                                {
                                    recNamePlugInList.Add(recNamePlugInList.Count, info);
                                }
                                updatePlugInFile = false;
                            }
                        }
                        writeList.Clear();
                        recNameList.Clear();
                        writeList = null;
                        recNameList = null;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
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
            try
            {
                if (updateAutoAddEpgInfo == true)
                {
                    {
                        epgAutoAddList.Clear();
                        epgAutoAddList = null;
                        epgAutoAddList = new Dictionary<uint, EpgAutoAddData>();
                        List<EpgAutoAddData> list = new List<EpgAutoAddData>();
                        ret = CommonManager.CreateSrvCtrl().SendEnumEpgAutoAdd(ref list);
                        if (ret == ErrCode.CMD_SUCCESS)
                        {
                            foreach (EpgAutoAddData info in list)
                            {
                                epgAutoAddList.Add(info.dataID, info);
                            }
                            updateAutoAddEpgInfo = false;
                        }
                        list.Clear();
                        list = null;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
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
            try
            {
                if (updateAutoAddManualInfo == true)
                {
                    {
                        manualAutoAddList.Clear();
                        manualAutoAddList = null;
                        manualAutoAddList = new Dictionary<uint, ManualAutoAddData>();
                        List<ManualAutoAddData> list = new List<ManualAutoAddData>();
                        ret = CommonManager.CreateSrvCtrl().SendEnumManualAdd(ref list);
                        if (ret == ErrCode.CMD_SUCCESS)
                        {
                            foreach (ManualAutoAddData info in list)
                            {
                                manualAutoAddList.Add(info.dataID, info);
                            }
                            updateAutoAddManualInfo = false;
                        }
                        list.Clear();
                        list = null;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return ret;
        }

        public bool GetNextReserve(ref ReserveData info)
        {
            bool ret = false;

            SortedList<String, ReserveData> sortList = new SortedList<String, ReserveData>();
            foreach (ReserveData resInfo in reserveList.Values)
            {
                if (resInfo.RecSetting.RecMode != 5)
                {
                    String key = resInfo.StartTime.ToString("yyyyMMddHHmmss");
                    key += resInfo.ReserveID.ToString("X8");
                    sortList.Add(key, resInfo);
                }
            }

            foreach (ReserveData resInfo in sortList.Values)
            {
                if (resInfo.StartTime > DateTime.UtcNow.AddHours(9))
                {
                    info = resInfo;
                    ret = true;
                    break;
                }
            }
            sortList.Clear();
            sortList = null;
            return ret;
        }
    }
}
