using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.IO;

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
        private bool updateEpgAutoAddAppend = true;
        private bool updateEpgAutoAddAppendReserveInfo = true;

        Dictionary<UInt64, EpgServiceEventInfo> serviceEventList = new Dictionary<UInt64, EpgServiceEventInfo>();
        Dictionary<UInt32, ReserveData> reserveList = new Dictionary<UInt32, ReserveData>();
        Dictionary<UInt32, ReserveDataAppend> reserveAppendList = null;
        Dictionary<UInt32, TunerReserveInfo> tunerReserveList = new Dictionary<UInt32, TunerReserveInfo>();
        Dictionary<UInt32, RecFileInfo> recFileInfo = new Dictionary<UInt32, RecFileInfo>();
        Dictionary<UInt32, RecFileInfoAppend> recFileAppendList = null;
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
            var dict = epgAutoAddAppendList ?? new Dictionary<uint, EpgAutoAddDataAppend>();
            if (updateEpgAutoAddAppend == true)
            {
                List<EpgAutoAddData> srcList = epgAutoAddList.Values.Where(data => dict.ContainsKey(data.dataID) == false).ToList();
                if (srcList.Count != 0)
                {
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
                    catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
                }

                updateEpgAutoAddAppend = false;
                updateEpgAutoAddAppendReserveInfo = true;//現時刻でのSearchList再作成も含む
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
        public void ClearEpgAutoAddDataAppend(Dictionary<UInt32, EpgAutoAddData> oldList = null)
        {
            if (oldList == null) epgAutoAddAppendList = null;
            if (epgAutoAddAppendList == null) return;

            var xs = new System.Xml.Serialization.XmlSerializer(typeof(EpgSearchKeyInfo));
            var SearchKey2String = new Func<EpgAutoAddData, string>(epgdata =>
            {
                var ms = new MemoryStream();
                xs.Serialize(ms, epgdata.searchInfo);
                ms.Seek(0, SeekOrigin.Begin);
                return new StreamReader(ms).ReadToEnd();
            });

            //並べ替えによるID変更もあるので、内容ベースでAppendを再利用する。
            var dicOld = new Dictionary<string, EpgAutoAddDataAppend>();
            foreach (var info in oldList.Values)
            {
                EpgAutoAddDataAppend data;
                if (epgAutoAddAppendList.TryGetValue(info.dataID, out data) == true)
                {
                    string key = SearchKey2String(info);
                    if (dicOld.ContainsKey(key) == false)
                    {
                        dicOld.Add(key, data);
                    }
                }
            }
            var newAppend = new Dictionary<uint, EpgAutoAddDataAppend>();
            foreach (var info in epgAutoAddList.Values)
            {
                string key = SearchKey2String(info);
                EpgAutoAddDataAppend append1;
                if (dicOld.TryGetValue(key, out append1) == true)
                {
                    //同一内容の検索が複数ある場合は同じデータを参照することになる。
                    //特に問題無いはずだが、マズいようなら何か対応する。
                    newAppend.Add(info.dataID, append1);
                }
            }
            epgAutoAddAppendList = newAppend;
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

        public RecFileInfoAppend GetRecFileAppend(RecFileInfo master, bool UpdateDB = false)
        {
            if (master == null) return null;

            if (recFileAppendList == null)
            {
                recFileAppendList = new Dictionary<uint, RecFileInfoAppend>();
            }

            RecFileInfoAppend retv = null;
            if (recFileAppendList.TryGetValue(master.ID, out retv) == false)
            {
                if (UpdateDB == true)
                {
                    var list = recFileInfo.Values.Where(info => info.HasErrPackets == true 
                        && recFileAppendList.ContainsKey(info.ID) == false).ToList();

                    try
                    {
                        var extraDatalist = new List<RecFileInfo>();
                        if (cmd.SendGetRecInfoList(list.Select(info => info.ID).ToList(), ref extraDatalist) == ErrCode.CMD_SUCCESS)
                        {
                            extraDatalist.ForEach(item => recFileAppendList.Add(item.ID, new RecFileInfoAppend(item)));
                        }
                    }
                    catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }

                    //何か問題があった場合でも何度もSendGetRecInfoList()しないよう残りも全て登録してしまう。
                    foreach (var item in list.Where(info => recFileAppendList.ContainsKey(info.ID) == false))
                    {
                        recFileAppendList.Add(item.ID, new RecFileInfoAppend(item, false));
                    }

                    recFileAppendList.TryGetValue(master.ID, out retv);
                }
                else
                {
                    var extraRecInfo = new RecFileInfo();
                    if (cmd.SendGetRecInfo(master.ID, ref extraRecInfo) == ErrCode.CMD_SUCCESS)
                    {
                        retv = new RecFileInfoAppend(extraRecInfo);
                        recFileAppendList.Add(master.ID, retv);
                    }
                }
            }
            return retv ?? new RecFileInfoAppend(master);
        }
        public void ClearRecFileAppend(bool connect = false)
        {
            if (recFileAppendList == null) return;

            if (Settings.Instance.RecInfoExtraDataCache == false ||
                connect == true && Settings.Instance.RecInfoExtraDataCacheKeepConnect == false)
            {
                recFileAppendList = null;
            }
            else if (connect == false && Settings.Instance.RecInfoExtraDataCacheOptimize == true)
            {
                //Appendリストにあるが、有効でないデータ(通信エラーなどで仮登録されたもの)を削除。
                var delList = recFileAppendList.Where(item => item.Value.IsValid == false).Select(item => item.Key).ToList();
                delList.ForEach(key => recFileAppendList.Remove(key));

                //現在の録画情報リストにないデータを削除。
                var delList2 = recFileAppendList.Keys.Where(key => recFileInfo.ContainsKey(key) == false).ToList();
                delList2.ForEach(key => recFileAppendList.Remove(key));
            }
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
            reserveAppendList = null;
            recFileAppendList = null;
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
                    updateEpgAutoAddAppend = true;
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
                    updateEpgAutoAddAppend = true;
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

        /// <summary>EPG更新フラグをオフにする(EpgTimerSrv直接起動時用)。</summary>
        public void ResetUpdateNotifyEpg() { updateEpgData = false; }

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

                    ret = (ErrCode)cmd.SendEnumRecInfoBasic(ref list);
                    if (ret != ErrCode.CMD_SUCCESS) return ret;

                    list.ForEach(info => recFileInfo.Add(info.ID, info));

                    ClearRecFileAppend();
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
                        Dictionary<uint, EpgAutoAddData> oldList = epgAutoAddList;
                        epgAutoAddList = new Dictionary<uint, EpgAutoAddData>();
                        List<EpgAutoAddData> list = new List<EpgAutoAddData>();

                        ret = (ErrCode)cmd.SendEnumEpgAutoAdd(ref list);
                        if (ret != ErrCode.CMD_SUCCESS) return ret;

                        list.ForEach(info => epgAutoAddList.Add(info.dataID, info));

                        ClearEpgAutoAddDataAppend(oldList);
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
