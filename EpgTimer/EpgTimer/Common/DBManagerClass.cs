using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    class DBManager
    {
        private CtrlCmdUtil cmd = null;
        private bool updateEpgData = true;
        private bool updateReserveInfo = true;
        private bool updateRecInfo = true;
        private bool updateAutoAddEpgInfo = true;
        private bool updateAutoAddManualInfo = true;
        private bool updatePlugInFile = true;
        private bool noAutoReloadEpg = false;
        private bool oneTimeReloadEpg = false;
        private bool updateAutoAddAppendReserveInfo = true;

        Dictionary<UInt64, EpgServiceEventInfo> serviceEventList = new Dictionary<UInt64, EpgServiceEventInfo>();
        Dictionary<UInt32, ReserveData> reserveList = new Dictionary<UInt32, ReserveData>();
        Dictionary<UInt32, TunerReserveInfo> tunerReserveList = new Dictionary<UInt32, TunerReserveInfo>();
        Dictionary<UInt32, RecFileInfo> recFileInfo = new Dictionary<UInt32, RecFileInfo>();
        Dictionary<Int32, String> writePlugInList = new Dictionary<Int32, String>();
        Dictionary<Int32, String> recNamePlugInList = new Dictionary<Int32, String>();
        Dictionary<UInt32, ManualAutoAddData> manualAutoAddList = new Dictionary<UInt32, ManualAutoAddData>();
        Dictionary<UInt32, EpgAutoAddData> epgAutoAddList = new Dictionary<UInt32, EpgAutoAddData>();
        Dictionary<UInt32, EpgAutoAddDataAppend> epgAutoAddAppendList = null;

        public Dictionary<UInt64, EpgServiceEventInfo> ServiceEventList
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
        public Dictionary<UInt32, EpgAutoAddDataAppend> EpgAutoAddAppendList
        {
            get { return epgAutoAddAppendList; }
        }
        public EpgAutoAddDataAppend GetEpgAutoAddDataAppend(EpgAutoAddData master)
        {
            if (master == null) return null;

            //データ更新は必要になったときにまとめて行う
            //未使用か、EpgAutoAddData更新により古いデータ廃棄済みでデータが無い場合
            if (epgAutoAddAppendList == null)
            {
                epgAutoAddAppendList = new Dictionary<uint, EpgAutoAddDataAppend>();
                List<EpgSearchKeyInfo> keyList = new List<EpgSearchKeyInfo>();

                foreach (EpgAutoAddData item in EpgAutoAddList.Values)
                {
                    //「検索無効」の対応のため、andKeyをコピーする。
                    EpgSearchKeyInfo key = item.searchInfo.Clone();
                    key.keyDisabledFlag = 0; //無効解除
                    keyList.Add(key);
                }

                try
                {
                    List<List<EpgEventInfo>> list_list = new List<List<EpgEventInfo>>();
                    CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
                    cmd.SendSearchPgByKey(keyList, ref list_list);

                    //通常あり得ないが、コマンド成功にもかかわらず何か問題があった場合は飛ばす
                    if (EpgAutoAddList.Count == list_list.Count)
                    {
                        int i = 0;
                        foreach (EpgAutoAddData item in EpgAutoAddList.Values)
                        {
                            epgAutoAddAppendList.Add(item.dataID, new EpgAutoAddDataAppend(item, list_list[i++]));
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }
            }

            //予約情報との突き合わせが古い場合
            if (updateAutoAddAppendReserveInfo == true)
            {
                foreach (EpgAutoAddDataAppend item in epgAutoAddAppendList.Values)
                {
                    item.updateCounts = true;
                }
                updateAutoAddAppendReserveInfo = false;
            }

            //SendSearchPgByKeyに失敗した場合などは引っかかる。
            EpgAutoAddDataAppend retv;
            if (epgAutoAddAppendList.TryGetValue(master.dataID, out retv) == false)
            {
                retv = new EpgAutoAddDataAppend(master);
                epgAutoAddAppendList.Add(master.dataID, retv);
            }
            return retv;
        }

        public DBManager(CtrlCmdUtil ctrlCmd)
        {
            cmd = ctrlCmd;
        }

        public void ClearAllDB()
        {
            serviceEventList = new Dictionary<ulong, EpgServiceEventInfo>();
            reserveList = new Dictionary<uint, ReserveData>();
            tunerReserveList = new Dictionary<uint, TunerReserveInfo>();
            recFileInfo = new Dictionary<uint, RecFileInfo>();
            writePlugInList = new Dictionary<int, string>();
            recNamePlugInList = new Dictionary<int, string>();
            manualAutoAddList = new Dictionary<uint, ManualAutoAddData>();
            epgAutoAddList = new Dictionary<uint, EpgAutoAddData>();
            epgAutoAddAppendList = null;
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
            switch ((UpdateNotifyItem)updateInfo)
            {
                case UpdateNotifyItem.EpgData:
                    updateEpgData = true;
                    break;
                case UpdateNotifyItem.ReserveInfo:
                    updateReserveInfo = true;
                    break;
                case UpdateNotifyItem.RecInfo:
                    updateRecInfo = true;
                    break;
                case UpdateNotifyItem.AutoAddEpgInfo:
                    updateAutoAddEpgInfo = true;
                    break;
                case UpdateNotifyItem.AutoAddManualInfo:
                    updateAutoAddManualInfo = true;
                    break;
                case UpdateNotifyItem.PlugInFile:
                    updatePlugInFile = true;
                    break;
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
                if (updateEpgData == true && (noAutoReloadEpg == false || oneTimeReloadEpg == true))
                {
                    if (cmd == null) return ErrCode.CMD_ERR;

                    serviceEventList = new Dictionary<ulong, EpgServiceEventInfo>();
                    var list = new List<EpgServiceEventInfo>();

                    ret = (ErrCode)cmd.SendEnumPgAll(ref list);
                    if (ret != ErrCode.CMD_SUCCESS) return ret;

                    list.ForEach(info => serviceEventList.Add(info.serviceInfo.Create64Key(), info));

                    updateEpgData = false;
                    oneTimeReloadEpg = false;
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
                    if (cmd == null) return ErrCode.CMD_ERR;

                    reserveList = new Dictionary<uint, ReserveData>();
                    tunerReserveList = new Dictionary<uint, TunerReserveInfo>();
                    var list = new List<ReserveData>();
                    var list2 = new List<TunerReserveInfo>();

                    ret = (ErrCode)cmd.SendEnumReserve(ref list);
                    if (ret != ErrCode.CMD_SUCCESS) return ret;

                    ret = (ErrCode)cmd.SendEnumTunerReserve(ref list2);
                    if (ret != ErrCode.CMD_SUCCESS) return ret;

                    list.ForEach(info => reserveList.Add(info.ReserveID, info));
                    list2.ForEach(info => tunerReserveList.Add(info.tunerID, info));

                    updateReserveInfo = false;
                    updateAutoAddAppendReserveInfo = true;
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
                    if (cmd == null) return ErrCode.CMD_ERR;

                    recFileInfo = new Dictionary<uint, RecFileInfo>();
                    var list = new List<RecFileInfo>();

                    ret = (ErrCode)cmd.SendEnumRecInfo(ref list);
                    if (ret != ErrCode.CMD_SUCCESS) return ret;

                    list.ForEach(info => recFileInfo.Add(info.ID, info));

                    updateRecInfo = false;
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
                    if (cmd == null) return ErrCode.CMD_ERR;

                    writePlugInList = new Dictionary<int, string>();
                    recNamePlugInList = new Dictionary<int, string>();
                    var writeList = new List<string>();
                    var recNameList = new List<string>();

                    ret = (ErrCode)cmd.SendEnumPlugIn(2, ref writeList);
                    if (ret != ErrCode.CMD_SUCCESS) return ret;

                    ret = (ErrCode)cmd.SendEnumPlugIn(1, ref recNameList);
                    if (ret != ErrCode.CMD_SUCCESS) return ret;

                    //dictionary使ってる意味無い‥
                    writeList.ForEach(info => writePlugInList.Add(writePlugInList.Count, info));
                    recNameList.ForEach(info => recNamePlugInList.Add(recNamePlugInList.Count, info));

                    updatePlugInFile = false;
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
                    if (cmd == null) return ErrCode.CMD_ERR;

                    {
                        epgAutoAddList = new Dictionary<uint, EpgAutoAddData>();
                        List<EpgAutoAddData> list = new List<EpgAutoAddData>();

                        ret = (ErrCode)cmd.SendEnumEpgAutoAdd(ref list);
                        if (ret != ErrCode.CMD_SUCCESS) return ret;

                        list.ForEach(info => epgAutoAddList.Add(info.dataID, info));

                        updateAutoAddEpgInfo = false;
                        epgAutoAddAppendList = null;
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
                    if (cmd == null) return ErrCode.CMD_ERR;

                    {
                        manualAutoAddList = new Dictionary<uint, ManualAutoAddData>();
                        List<ManualAutoAddData> list = new List<ManualAutoAddData>();

                        ret = (ErrCode)cmd.SendEnumManualAdd(ref list);
                        if (ret != ErrCode.CMD_SUCCESS) return ret;

                        list.ForEach(info => manualAutoAddList.Add(info.dataID, info));

                        updateAutoAddManualInfo = false;
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
            var sortList = new SortedList<String, ReserveData>();
            foreach (ReserveData resInfo in reserveList.Values)
            {
                if (resInfo.RecSetting.RecMode != 5 && resInfo.StartTime > DateTime.Now)
                {
                    String key = resInfo.StartTime.ToString("yyyyMMddHHmmss");
                    key += resInfo.ReserveID.ToString("X8");
                    sortList.Add(key, resInfo);
                }
            }

            if (sortList.Count != 0)
            {
                info = sortList.Values[0];
                return true;
            }

            return false;
        }
    }
}
