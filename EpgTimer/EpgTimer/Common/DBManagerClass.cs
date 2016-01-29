using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;

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
        private bool updateEpgAutoAddAppendReserveInfo = true;

        Dictionary<UInt64, EpgServiceEventInfo> serviceEventList = new Dictionary<UInt64, EpgServiceEventInfo>();
        Dictionary<UInt32, ReserveData> reserveList = new Dictionary<UInt32, ReserveData>();
        Dictionary<UInt32, ReserveDataAppend> reserveAppendList = null;
        Dictionary<UInt32, TunerReserveInfo> tunerReserveList = new Dictionary<UInt32, TunerReserveInfo>();
        Dictionary<UInt32, RecFileInfo> recFileInfo = new Dictionary<UInt32, RecFileInfo>();
        Dictionary<Int32, String> writePlugInList = new Dictionary<Int32, String>();
        Dictionary<Int32, String> recNamePlugInList = new Dictionary<Int32, String>();
        Dictionary<UInt32, ManualAutoAddData> manualAutoAddList = new Dictionary<UInt32, ManualAutoAddData>();
        Dictionary<UInt32, AutoAddDataAppend> manualAutoAddAppendList = null;
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
        public AutoAddDataAppend GetManualAutoAddDataAppend(ManualAutoAddData master)
        {
            if (master == null) return null;

            //データ更新は必要になったときにまとめて行う
            //未使用か、ManualAutoAddData更新により古いデータ廃棄済みでデータが無い場合
            Dictionary<uint, AutoAddDataAppend> dict = manualAutoAddAppendList;
            if (dict == null)
            {
                ReloadReserveInfo();//notify残ってれば更新

                dict = manualAutoAddList.Values.ToDictionary(item => item.dataID, item => new AutoAddDataAppend(
                    reserveList.Values.Where(info => info != null && info.IsEpgReserve == false && item.CheckPgHit(info)).ToList()));

                foreach (AutoAddDataAppend item in dict.Values) item.UpdateCounts();

                manualAutoAddAppendList = dict;
            }

            AutoAddDataAppend retv;
            if (dict.TryGetValue(master.dataID, out retv) == false)
            {
                retv = new AutoAddDataAppend();
            }
            return retv;
        }
        public EpgAutoAddDataAppend GetEpgAutoAddDataAppend(EpgAutoAddData master)
        {
            if (master == null) return null;

            //データ更新は必要になったときにまとめて行う
            //未使用か、EpgAutoAddData更新により古いデータ廃棄済みでデータが無い場合
            Dictionary<uint, EpgAutoAddDataAppend> dict = epgAutoAddAppendList;
            if (dict == null)
            {
                dict = new Dictionary<uint, EpgAutoAddDataAppend>();
                List<EpgAutoAddData> srcList = epgAutoAddList.Values.ToList();

                List<EpgSearchKeyInfo> keyList = srcList.RecSearchKeyList().Clone();
                keyList.ForEach(key => key.keyDisabledFlag = 0); //無効解除

                try
                {
                    var list_list = new List<List<EpgEventInfo>>();
                    cmd.SendSearchPgByKey(keyList, ref list_list);

                    //通常あり得ないが、コマンド成功にもかかわらず何か問題があった場合は飛ばす
                    if (srcList.Count == list_list.Count)
                    {
                        int i = 0;
                        foreach (EpgAutoAddData item in srcList)
                        {
                            dict.Add(item.dataID, new EpgAutoAddDataAppend(list_list[i++]));
                        }
                    }

                    epgAutoAddAppendList = dict;
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }

                updateEpgAutoAddAppendReserveInfo = true;
            }

            //予約情報との突き合わせが古い場合
            if (updateEpgAutoAddAppendReserveInfo == true)
            {
                ReloadReserveInfo();//notify残ってれば更新
                foreach (EpgAutoAddDataAppend item in dict.Values) item.UpdateCounts();
                updateEpgAutoAddAppendReserveInfo = false;
            }

            //SendSearchPgByKeyに失敗した場合などは引っかかる。
            EpgAutoAddDataAppend retv;
            if (dict.TryGetValue(master.dataID, out retv) == false)
            {
                retv = new EpgAutoAddDataAppend();
            }
            return retv;
        }
        public ReserveDataAppend GetReserveDataAppend(ReserveData master)
        {
            if (master == null) return null;

            Dictionary<uint, ReserveDataAppend> dict = reserveAppendList;
            if (dict == null)
            {
                dict = reserveList.ToDictionary(data => data.Key, data => new ReserveDataAppend());
                reserveAppendList = dict;

                ReloadEpgAutoAddInfo();//notify残ってれば更新
                foreach (EpgAutoAddData item in epgAutoAddList.Values)
                {
                    item.GetReserveList().ForEach(info => dict[info.ReserveID].EpgAutoList.Add(item));
                }

                ReloadManualAutoAddInfo();//notify残ってれば更新
                foreach (ManualAutoAddData item in manualAutoAddList.Values)
                {
                    item.GetReserveList().ForEach(info => dict[info.ReserveID].ManualAutoList.Add(item));
                }

                foreach (ReserveDataAppend data in dict.Values)
                {
                    data.UpdateData();
                }
            }

            ReserveDataAppend retv;
            if (dict.TryGetValue(master.ReserveID, out retv) == false)
            {
                retv = new ReserveDataAppend();
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
            manualAutoAddAppendList = null;
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
                    epgAutoAddAppendList = null;//検索数が変わる。
                    break;
                case UpdateNotifyItem.ReserveInfo:
                    updateReserveInfo = true;
                    manualAutoAddAppendList = null;
                    updateEpgAutoAddAppendReserveInfo = true;
                    reserveAppendList = null;
                    break;
                case UpdateNotifyItem.RecInfo:
                    updateRecInfo = true;
                    break;
                case UpdateNotifyItem.AutoAddEpgInfo:
                    updateAutoAddEpgInfo = true;
                    epgAutoAddAppendList = null;
                    reserveAppendList = null;
                    break;
                case UpdateNotifyItem.AutoAddManualInfo:
                    updateAutoAddManualInfo = true;
                    manualAutoAddAppendList = null;
                    reserveAppendList = null;
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

    }
}
