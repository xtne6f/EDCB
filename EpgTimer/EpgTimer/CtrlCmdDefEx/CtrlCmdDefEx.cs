using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;

namespace EpgTimer
{
    static class CtrlCmdDefEx
    {
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

        public static bool EqualsPg(ReserveData i1, ReserveData i2, bool IdMode = true, bool TimeMode = false)
        {
            if (i1 == null && i2 == null) return true;
            if (i1 == null || i2 == null) return false;
            return (IdMode == false || i1.EventID == i2.EventID)
                    && (TimeMode == false || i1.StartTime == i2.StartTime && i1.DurationSecond == i2.DurationSecond)
                    && i1.Create64Key() == i2.Create64Key();
        }

        //以降の三つは数の多いEpgEventInfo相手に実行されるので、Convert使わずバラしちゃった方がいいのかも
        public static bool EqualsPg(EpgEventInfo i1, EpgEventInfo i2, bool IdMode = true, bool TimeMode = false)
        {
            return EqualsPg(ConvertEpgToReserveData(i1), ConvertEpgToReserveData(i2), IdMode, TimeMode);
        }

        public static bool EqualsPg(EpgEventInfo i1, ReserveData i2, bool IdMode = true, bool TimeMode = false)
        {
            return EqualsPg(ConvertEpgToReserveData(i1), i2, IdMode, TimeMode);
        }

        public static bool EqualsPg(ReserveData i1, EpgEventInfo i2, bool IdMode = true, bool TimeMode = false)
        {
            return EqualsPg(i1, ConvertEpgToReserveData(i2), IdMode, TimeMode);
        }

        public static ReserveData ConvertEpgToReserveData(EpgEventInfo epgInfo)
        {
            if (epgInfo == null) return null;
            ReserveData resInfo = new ReserveData();
            epgInfo.ConvertToReserveData(ref resInfo);
            return resInfo;
        }

        public static bool ConvertToReserveData(this EpgEventInfo epgInfo, ref ReserveData resInfo)
        {
            if (epgInfo == null || resInfo == null) return false;

            resInfo.Title = epgInfo.Title();
            resInfo.StartTime = epgInfo.start_time;
            resInfo.StartTimeEpg = epgInfo.start_time;
            resInfo.DurationSecond = (epgInfo.DurationFlag == 0 ? 10 * 60 : epgInfo.durationSec);

            UInt64 key = epgInfo.Create64Key();
            if (ChSet5.Instance.ChList.ContainsKey(key) == true)
            {
                resInfo.StationName = ChSet5.Instance.ChList[key].ServiceName;
            }
            resInfo.OriginalNetworkID = epgInfo.original_network_id;
            resInfo.TransportStreamID = epgInfo.transport_stream_id;
            resInfo.ServiceID = epgInfo.service_id;
            resInfo.EventID = epgInfo.event_id;

            return true;
        }

        public static Func<object, ulong> GetKeyFunc(Type t)
        {
            if (t == typeof(ReserveItem))
            {
                return info => (info as ReserveItem).ReserveInfo.ReserveID;
            }
            else if (t == typeof(RecInfoItem))
            {
                return info => (info as RecInfoItem).RecInfo.ID;
            }
            else if (t == typeof(EpgAutoDataItem))
            {
                return info => (info as EpgAutoDataItem).EpgAutoAddInfo.dataID;
            }
            else if (t == typeof(ManualAutoAddDataItem))
            {
                return info => (info as ManualAutoAddDataItem).ManualAutoAddInfo.dataID;
            }
            else if (t == typeof(SearchItem))
            {
                return info => (info as SearchItem).EventInfo.Create64PgKey();
            }
            else if (t == typeof(NotifySrvInfoItem))
            {
                return info => (info as NotifySrvInfoItem).NotifyInfo.notifyID;
            }
            else
            {
                //必ずしもキーにはなるとは限らないが、エラーにしないため一応返す。
                return info => (ulong)info.GetHashCode();
            }
        }

        //ソート用の代替プロパティがあればその名前を返す
        public static string GetValuePropertyName(Type t, string key)
        {
            if (t == typeof(ReserveItem))
            {
                return ReserveItem.GetValuePropertyName(key);
            }
            else if (t == typeof(SearchItem))
            {
                return SearchItem.GetValuePropertyName(key);
            }
            else if (t == typeof(RecInfoItem))
            {
                return RecInfoItem.GetValuePropertyName(key);
            }
            else if (t == typeof(EpgAutoDataItem))
            {
                return EpgAutoDataItem.GetValuePropertyName(key);
            }
            else if (t == typeof(ManualAutoAddDataItem))
            {
                return ManualAutoAddDataItem.GetValuePropertyName(key);
            }
            else
            {
                return key;
            }
        }

        public static UInt64 Create64Key(this EpgServiceInfo obj)
        {
            return CommonManager.Create64Key(obj.ONID, obj.TSID, obj.SID);
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

        public static bool IsOnAir(this EpgEventInfo eventInfo)
        {
            if (eventInfo == null) return false;
            //
            return isOnTime(eventInfo.start_time, (int)eventInfo.durationSec);
        }

        public static bool isOnTime(DateTime startTime, int duration)
        {
            if (startTime > System.DateTime.Now) return false;
            //
            return (startTime + TimeSpan.FromSeconds(duration) >= System.DateTime.Now);
        }

    }
}
