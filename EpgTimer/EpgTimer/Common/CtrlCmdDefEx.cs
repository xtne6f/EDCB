using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    static class CtrlCmdDefEx
    {
        //シリアライズとか使わず地道にコピーする。
        //シャローコピー部分はGetType()などを使う方法もあるが、やはり地道にコピーすることにする。

        //CopyObj.csのジェネリックを使って定義している。
        public static List<EpgSearchKeyInfo> Clone(this List<EpgSearchKeyInfo> src) { return CopyObj.Clone(src, CopyData); }
        public static EpgSearchKeyInfo Clone(this EpgSearchKeyInfo src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this EpgSearchKeyInfo src, EpgSearchKeyInfo dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(EpgSearchKeyInfo src, EpgSearchKeyInfo dest)
        {
            dest.aimaiFlag = src.aimaiFlag;
            dest.andKey = src.andKey;
            dest.audioList = src.audioList.ToList();
            dest.contentList = src.contentList.Clone(); //EpgContentData
            dest.dateList = src.dateList.Clone();       //EpgSearchDateInfo
            dest.freeCAFlag = src.freeCAFlag;
            dest.notContetFlag = src.notContetFlag;
            dest.notDateFlag = src.notDateFlag;
            dest.notKey = src.notKey;
            dest.regExpFlag = src.regExpFlag;
            dest.serviceList = src.serviceList.ToList();
            dest.titleOnlyFlag = src.titleOnlyFlag;
            dest.videoList = src.videoList.ToList();
            dest.chkRecEnd = src.chkRecEnd;
            dest.chkRecDay = src.chkRecDay;
            dest.chkRecNoService = src.chkRecNoService;
            dest.chkDurationMin = src.chkDurationMin;
            dest.chkDurationMax = src.chkDurationMax;
            dest.caseFlag = src.caseFlag;
            dest.keyDisabledFlag = src.keyDisabledFlag;
        }

        public static List<EpgContentData> Clone(this List<EpgContentData> src) { return CopyObj.Clone(src, CopyData); }
        public static EpgContentData Clone(this EpgContentData src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this EpgContentData src, EpgContentData dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(EpgContentData src, EpgContentData dest)
        {
            dest.content_nibble_level_1 = src.content_nibble_level_1;
            dest.content_nibble_level_2 = src.content_nibble_level_2;
            dest.user_nibble_1 = src.user_nibble_1;
            dest.user_nibble_2 = src.user_nibble_2;
        }

        public static List<EpgSearchDateInfo> Clone(this List<EpgSearchDateInfo> src) { return CopyObj.Clone(src, CopyData); }
        public static EpgSearchDateInfo Clone(this EpgSearchDateInfo src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this EpgSearchDateInfo src, EpgSearchDateInfo dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(EpgSearchDateInfo src, EpgSearchDateInfo dest)
        {
            dest.endDayOfWeek = src.endDayOfWeek;
            dest.endHour = src.endHour;
            dest.endMin = src.endMin;
            dest.startDayOfWeek = src.startDayOfWeek;
            dest.startHour = src.startHour;
            dest.startMin = src.startMin;
        }

        public static List<RecFileInfo> Clone(this List<RecFileInfo> src) { return CopyObj.Clone(src, CopyData); }
        public static RecFileInfo Clone(this RecFileInfo src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this RecFileInfo src, RecFileInfo dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(RecFileInfo src, RecFileInfo dest)
        {
            dest.Comment = src.Comment;
            dest.Drops = src.Drops;
            dest.DurationSecond = src.DurationSecond;
            dest.ErrInfo = src.ErrInfo;
            dest.EventID = src.EventID;
            dest.ID = src.ID;
            dest.OriginalNetworkID = src.OriginalNetworkID;
            dest.ProgramInfo = src.ProgramInfo;
            dest.ProtectFlag = src.ProtectFlag;
            dest.RecFilePath = src.RecFilePath;
            dest.RecStatus = src.RecStatus;
            dest.Scrambles = src.Scrambles;
            dest.ServiceID = src.ServiceID;
            dest.ServiceName = src.ServiceName;
            dest.StartTime = src.StartTime;
            dest.StartTimeEpg = src.StartTimeEpg;
            dest.Title = src.Title;
            dest.TransportStreamID = src.TransportStreamID;
        }

        public static List<ReserveData> Clone(this List<ReserveData> src) { return CopyObj.Clone(src, CopyData); }
        public static ReserveData Clone(this ReserveData src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this ReserveData src, ReserveData dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(ReserveData src, ReserveData dest)
        {
            dest.Comment = src.Comment;
            dest.DurationSecond = src.DurationSecond;
            dest.EventID = src.EventID;
            dest.OriginalNetworkID = src.OriginalNetworkID;
            dest.OverlapMode = src.OverlapMode;
            dest.RecFileNameList = src.RecFileNameList.ToList();
            dest.RecSetting = src.RecSetting.Clone();               //RecSettingData
            dest.ReserveID = src.ReserveID;
            dest.ReserveStatus = src.ReserveStatus;
            dest.ServiceID = src.ServiceID;
            dest.StartTime = src.StartTime;
            dest.StartTimeEpg = src.StartTimeEpg;
            dest.StationName = src.StationName;
            dest.Title = src.Title;
            dest.TransportStreamID = src.TransportStreamID;
        }

        public static List<RecSettingData> Clone(this List<RecSettingData> src) { return CopyObj.Clone(src, CopyData); }
        public static RecSettingData Clone(this RecSettingData src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this RecSettingData src, RecSettingData dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(RecSettingData src, RecSettingData dest)
        {
            dest.BatFilePath = src.BatFilePath;
            dest.ContinueRecFlag = src.ContinueRecFlag;
            dest.EndMargine = src.EndMargine;
            dest.PartialRecFlag = src.PartialRecFlag;
            dest.PartialRecFolder = src.PartialRecFolder.Clone();   //RecFileSetInfo
            dest.PittariFlag = src.PittariFlag;
            dest.Priority = src.Priority;
            dest.RebootFlag = src.RebootFlag;
            dest.RecFolderList = src.RecFolderList.Clone();         //RecFileSetInfo
            dest.RecMode = src.RecMode;
            dest.ServiceMode = src.ServiceMode;
            dest.StartMargine = src.StartMargine;
            dest.SuspendMode = src.SuspendMode;
            dest.TuijyuuFlag = src.TuijyuuFlag;
            dest.TunerID = src.TunerID;
            dest.UseMargineFlag = src.UseMargineFlag;
        }

        public static bool EqualsTo(this IList<RecSettingData> src, IList<RecSettingData> dest) { return CopyObj.EqualsTo(src, dest, EqualsValue); }
        public static bool EqualsTo(this RecSettingData src, RecSettingData dest) { return CopyObj.EqualsTo(src, dest, EqualsValue); }
        public static bool EqualsValue(RecSettingData src, RecSettingData dest)
        {
            return src.BatFilePath == dest.BatFilePath
                && src.ContinueRecFlag == dest.ContinueRecFlag
                && src.EndMargine == dest.EndMargine
                && src.PartialRecFlag == dest.PartialRecFlag
                && src.PartialRecFolder.EqualsTo(dest.PartialRecFolder) //RecFileSetInfo
                && src.PittariFlag == dest.PittariFlag
                && src.Priority == dest.Priority
                && src.RebootFlag == dest.RebootFlag
                && src.RecFolderList.EqualsTo(dest.RecFolderList)       //RecFileSetInfo
                && src.RecMode == dest.RecMode
                && src.ServiceMode == dest.ServiceMode
                && src.StartMargine == dest.StartMargine
                && src.SuspendMode == dest.SuspendMode
                && src.TuijyuuFlag == dest.TuijyuuFlag
                && src.TunerID == dest.TunerID
                && src.UseMargineFlag == dest.UseMargineFlag;
        }

        public static bool EqualsSettingTo(this RecSettingData src, RecSettingData dest, bool IsManual = false)
        {
            if (src == null || dest == null) return false;
            return src.BatFilePath == dest.BatFilePath
                && src.ContinueRecFlag == dest.ContinueRecFlag
                && (src.EndMargine == dest.EndMargine || src.UseMargineFlag == 0)//マージンデフォルト時
                && src.PartialRecFlag == dest.PartialRecFlag
                && src.PartialRecFolder.EqualsTo(dest.PartialRecFolder)
                && (src.PittariFlag == dest.PittariFlag || IsManual == true)//プログラム予約時
                && src.Priority == dest.Priority
                && (src.RebootFlag == dest.RebootFlag || src.SuspendMode == 0)//動作後設定デフォルト時
                && src.RecFolderList.EqualsTo(dest.RecFolderList)
                && (src.RecMode == dest.RecMode || src.RecMode == 5 || dest.RecMode == 5)
                && (src.ServiceMode == dest.ServiceMode || ((src.ServiceMode | dest.ServiceMode) & 0x0F) == 0)//字幕等データ設定デフォルト時
                && (src.StartMargine == dest.StartMargine || src.UseMargineFlag == 0)//マージンデフォルト時
                && src.SuspendMode == dest.SuspendMode//動作後設定
                && (src.TuijyuuFlag == dest.TuijyuuFlag || IsManual == true)//プログラム予約時
                && src.TunerID == dest.TunerID
                && src.UseMargineFlag == dest.UseMargineFlag;
        }

        public static List<RecFileSetInfo> Clone(this List<RecFileSetInfo> src) { return CopyObj.Clone(src, CopyData); }
        public static RecFileSetInfo Clone(this RecFileSetInfo src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this RecFileSetInfo src, RecFileSetInfo dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(RecFileSetInfo src, RecFileSetInfo dest)
        {
            dest.RecFileName = src.RecFileName;
            dest.RecFolder = src.RecFolder;
            dest.RecNamePlugIn = src.RecNamePlugIn;
            dest.WritePlugIn = src.WritePlugIn;
        }

        public static bool EqualsTo(this IList<RecFileSetInfo> src, IList<RecFileSetInfo> dest) { return CopyObj.EqualsTo(src, dest, EqualsValue); }
        public static bool EqualsTo(this RecFileSetInfo src, RecFileSetInfo dest) { return CopyObj.EqualsTo(src, dest, EqualsValue); }
        public static bool EqualsValue(RecFileSetInfo src, RecFileSetInfo dest)
        {
            return src.RecFileName == dest.RecFileName
                && src.RecFolder == dest.RecFolder
                && src.RecNamePlugIn == dest.RecNamePlugIn
                && src.WritePlugIn == dest.WritePlugIn;
        }

        public static List<EpgAutoAddData> Clone(this List<EpgAutoAddData> src) { return CopyObj.Clone(src, CopyData); }
        public static EpgAutoAddData Clone(this EpgAutoAddData src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this EpgAutoAddData src, EpgAutoAddData dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(EpgAutoAddData src, EpgAutoAddData dest)
        {
            dest.addCount = src.addCount;
            dest.dataID = src.dataID;
            dest.recSetting = src.recSetting.Clone();       //RecSettingData
            dest.searchInfo = src.searchInfo.Clone();       //EpgSearchKeyInfo
        }

        public static List<ManualAutoAddData> Clone(this List<ManualAutoAddData> src) { return CopyObj.Clone(src, CopyData); }
        public static ManualAutoAddData Clone(this ManualAutoAddData src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this ManualAutoAddData src, ManualAutoAddData dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(ManualAutoAddData src, ManualAutoAddData dest)
        {
            dest.dataID = src.dataID;
            dest.dayOfWeekFlag = src.dayOfWeekFlag;
            dest.durationSecond = src.durationSecond;
            dest.originalNetworkID = src.originalNetworkID;
            dest.recSetting = src.recSetting.Clone();       //RecSettingData
            dest.serviceID = src.serviceID;
            dest.startTime = src.startTime;
            dest.stationName = src.stationName;
            dest.title = src.title;
            dest.transportStreamID = src.transportStreamID;
        }

        public static Func<object, ulong> GetKeyFunc(object refobj)
        {
            if (refobj != null)
            {
                string typeName = refobj.GetType().Name;
                if (refobj is ReserveItem)
                {
                    return info => (info as ReserveItem).ReserveInfo.ReserveID;
                }
                else if (refobj is RecInfoItem)
                {
                    return info => (info as RecInfoItem).RecInfo.ID;
                }
                else if (refobj is EpgAutoDataItem)
                {
                    return info => (info as EpgAutoDataItem).EpgAutoAddInfo.dataID;
                }
                else if (refobj is ManualAutoAddDataItem)
                {
                    return info => (info as ManualAutoAddDataItem).ManualAutoAddInfo.dataID;
                }
                else if (refobj is SearchItem)
                {
                    return info => (info as SearchItem).EventInfo.Create64PgKey();
                }
                else if (refobj is NotifySrvInfoItem)
                {
                    return info => (info as NotifySrvInfoItem).NotifyInfo.notifyID;
                }
            }

            //キーにはなっていないが、エラーにしないため一応返す
            return info => (ulong)info.GetHashCode();
        }


        public static List<RecSettingData> RecSettingList(this List<ReserveData> list)
        {
            return list.Where(item => item != null).Select(item => item.RecSetting).ToList();
        }
        public static List<RecSettingData> RecSettingList(this List<EpgAutoAddData> list)
        {
            return list.Where(item => item != null).Select(item => item.recSetting).ToList();
        }
        public static List<RecSettingData> RecSettingList(this List<ManualAutoAddData> list)
        {
            return list.Where(item => item != null).Select(item => item.recSetting).ToList();
        }

        public static List<EpgSearchKeyInfo> RecSearchKeyList(this List<EpgAutoAddData> list)
        {
            return list.Where(item => item != null).Select(item => item.searchInfo).ToList();
        }

        public static UInt64 Create64Key(this EpgServiceInfo obj)
        {
            return CommonManager.Create64Key(obj.ONID, obj.TSID, obj.SID);
        }

        public static UInt64 Create64Key(this RecFileInfo obj)
        {
            return CommonManager.Create64Key(obj.OriginalNetworkID, obj.TransportStreamID, obj.ServiceID);
        }
        public static UInt64 Create64PgKey(this RecFileInfo obj)
        {
            return CommonManager.Create64PgKey(obj.OriginalNetworkID, obj.TransportStreamID, obj.ServiceID, obj.EventID);
        }

        public static UInt64 Create64Key(this ManualAutoAddData obj)
        {
            return CommonManager.Create64Key(obj.originalNetworkID, obj.transportStreamID, obj.serviceID);
        }

        public static UInt64 Create64Key(this EpgEventInfo obj)
        {
            return CommonManager.Create64Key(obj.original_network_id, obj.transport_stream_id, obj.service_id);
        }
        public static UInt64 Create64PgKey(this EpgEventInfo obj)
        {
            return CommonManager.Create64PgKey(obj.original_network_id, obj.transport_stream_id, obj.service_id, obj.event_id);
        }
        public static string Title(this EpgEventInfo info)
        {
            return (info.ShortInfo == null ? "" : info.ShortInfo.event_name);
        }
        
        public static UInt64 Create64Key(this EpgEventData obj)
        {
            return CommonManager.Create64Key(obj.original_network_id, obj.transport_stream_id, obj.service_id);
        }
        public static UInt64 Create64PgKey(this EpgEventData obj)
        {
            return CommonManager.Create64PgKey(obj.original_network_id, obj.transport_stream_id, obj.service_id, obj.event_id);
        }

        public static UInt64 Create64Key(this ReserveData obj)
        {
            return CommonManager.Create64Key(obj.OriginalNetworkID, obj.TransportStreamID, obj.ServiceID);
        }
        public static UInt64 Create64PgKey(this ReserveData obj)
        {
            return CommonManager.Create64PgKey(obj.OriginalNetworkID, obj.TransportStreamID, obj.ServiceID, obj.EventID);
        }

        public static bool IsOnRec(this ReserveData reserveInfo, int MarginMin = 0)
        {
            if (reserveInfo == null) return false;
            //
            int duration = (int)reserveInfo.DurationSecond;
            int StartMargin = CommonManager.Instance.MUtil.GetMargin(reserveInfo.RecSetting, true) + 60 * MarginMin;
            int EndMargin = CommonManager.Instance.MUtil.GetMargin(reserveInfo.RecSetting, false);

            DateTime startTime = reserveInfo.StartTime.AddSeconds(StartMargin * -1);
            duration += StartMargin;
            duration += EndMargin;

            return isOnTime(startTime, duration);
        }

        public static bool IsOnAir(this ReserveData reserveInfo)
        {
            if (reserveInfo == null) return false;
            //
            return isOnTime(reserveInfo.StartTime, (int)reserveInfo.DurationSecond);
        }
        public static bool IsOnAir(this EpgEventInfo eventInfo)
        {
            if (eventInfo == null) return false;
            //
            return isOnTime(eventInfo.start_time, (int)eventInfo.durationSec);
        }

        private static bool isOnTime(DateTime startTime, int duration)
        {
            if (startTime > System.DateTime.Now) return false;
            //
            return (startTime + TimeSpan.FromSeconds(duration) >= System.DateTime.Now);
        }

        public static RecPresetItem LookUpPreset(this RecSettingData data, bool IsManual = false)
        {
            RecPresetItem preset = Settings.Instance.RecPresetList.FirstOrDefault(p1 =>
            {
                var pdata = new RecSettingData();
                Settings.GetDefRecSetting(p1.ID, ref pdata);
                return pdata.EqualsSettingTo(data, IsManual);
            });
            return preset == null ? new RecPresetItem("登録時", 0xFFFFFFFF) : preset;
        }

        public static ReserveData GetNextReserve(this List<ReserveData> resList, bool IsTargetOffRes = false)
        {
            ReserveData ret = null;
            long value = long.MaxValue;

            foreach(ReserveData data in resList)
            {
                if (IsTargetOffRes == true || data.RecSetting.RecMode != 5)
                {
                    if (value > data.StartTime.ToBinary())
                    {
                        ret = data;
                        value = data.StartTime.ToBinary();
                    }
                }
            }

            return ret;
        }

        public static List<ReserveData> GetReserveList(this ManualAutoAddData mdata)
        {
            return CommonManager.Instance.DB.ReserveList.Values.Where(info =>
            {
                //イベントID無し、サービス、開始時刻、長さ一致の予約を拾う。EpgTimerSrv側と同じ内容の判定。
                return info != null && info.EventID == 0xFFFF
                    && mdata.Create64Key() == info.Create64Key()
                    && mdata.startTime == info.StartTime.Hour * 3600 + info.StartTime.Minute * 60 + info.StartTime.Second
                    && mdata.durationSecond == info.DurationSecond
                    && (mdata.dayOfWeekFlag & (byte)(0x01 << (int)info.StartTime.DayOfWeek)) != 0
                    ;
            }).ToList();
        }
        public static List<ReserveData> GetReserveList(this ICollection<ManualAutoAddData> mlist)
        {
            var retList = new List<ReserveData>();
            foreach (ManualAutoAddData info in mlist) retList.AddRange(info.GetReserveList());
            return retList.Distinct().ToList();
        }
        public static List<List<ReserveData>> GetReserveListList(this ICollection<ManualAutoAddData> mlist)
        {
            var rlist_list = new List<List<ReserveData>>();
            foreach (ManualAutoAddData info in mlist) rlist_list.Add(info.GetReserveList());
            return rlist_list;
        }

        public static List<ReserveData> GetReserveList(this ICollection<EpgAutoAddData> mlist)
        {
            var retList = new List<ReserveData>();
            foreach (EpgAutoAddData info in mlist) retList.AddRange(info.GetReserveList());
            return retList.Distinct().ToList();
        }
        public static List<List<ReserveData>> GetReserveListList(this ICollection<EpgAutoAddData> mlist)
        {
            var rlist_list = new List<List<ReserveData>>();
            foreach (EpgAutoAddData info in mlist) rlist_list.Add(info.GetReserveList());
            return rlist_list;
        }

        public static List<RecFileInfo> GetNoProtectedList(this ICollection<RecFileInfo> itemlist)
        {
            return itemlist.Where(item => item == null ? false : item.ProtectFlag == 0).ToList();
        }
        //public static bool HasProtected(this List<RecInfoItem> list)
        //{
        //    return list.Any(info => info == null ? false : info.RecInfo.ProtectFlag == true);
        //}
        public static bool HasNoProtected(this List<RecFileInfo> list)
        {
            return list.Any(info => info == null ? false : info.ProtectFlag == 0);
        }
    }
}
