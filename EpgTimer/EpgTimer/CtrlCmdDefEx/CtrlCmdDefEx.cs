using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;

namespace EpgTimer
{
    public interface IRecWorkMainData
    {
        string DataTitle { get; }
        //ulong DataID { get; }
    }

    public interface IBasicPgInfo : IRecWorkMainData
    {
        //string PgTitle { get; }
        //string PgInfo { get; }
        DateTime PgStartTime { get; }
        uint PgDurationSecond { get; }
        UInt64 Create64Key();
        UInt64 Create64PgKey();
    }

    public interface IAutoAddTargetData : IBasicPgInfo
    {
        List<EpgAutoAddData> SearchEpgAutoAddList(bool? IsEnabled, bool ByFazy);
        List<EpgAutoAddData> GetEpgAutoAddList(bool? IsEnabled = null);
        List<ManualAutoAddData> GetManualAutoAddList(bool? IsEnabled = null);
    }

    static class CtrlCmdDefEx
    {
        //CopyObj.csのジェネリックを使って定義している。
        public static List<EpgSearchKeyInfo> Clone(this IEnumerable<EpgSearchKeyInfo> src) { return CopyObj.Clone(src, CopyData); }
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

        public static List<EpgContentData> Clone(this IEnumerable<EpgContentData> src) { return CopyObj.Clone(src, CopyData); }
        public static EpgContentData Clone(this EpgContentData src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this EpgContentData src, EpgContentData dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(EpgContentData src, EpgContentData dest)
        {
            dest.content_nibble_level_1 = src.content_nibble_level_1;
            dest.content_nibble_level_2 = src.content_nibble_level_2;
            dest.user_nibble_1 = src.user_nibble_1;
            dest.user_nibble_2 = src.user_nibble_2;
        }

        public static List<EpgSearchDateInfo> Clone(this IEnumerable<EpgSearchDateInfo> src) { return CopyObj.Clone(src, CopyData); }
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

        public static List<RecFileSetInfo> Clone(this IEnumerable<RecFileSetInfo> src) { return CopyObj.Clone(src, CopyData); }
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

        private static bool EqualsPg(ushort eID1, DateTime start1, uint duration1, ulong key1,
                                     ushort eID2, DateTime start2, uint duration2, ulong key2, bool IdMode, bool TimeMode)
        {
            return (IdMode == false || eID1 == eID2) &&
                   (TimeMode == false || start1 == start2 && duration1 == duration2) &&
                    key1 == key2;
        }
        public static bool EqualsPg(this ReserveData i1, ReserveData i2, bool IdMode = true, bool TimeMode = false)
        {
            if (i1 == null && i2 == null) return true;
            if (i1 == null || i2 == null) return false;
            return EqualsPg(i1.EventID, i1.StartTime, i1.DurationSecond, i1.Create64Key(),
                            i2.EventID, i2.StartTime, i2.DurationSecond, i2.Create64Key(), IdMode, TimeMode);
        }
        public static bool EqualsPg(this EpgEventInfo i1, EpgEventInfo i2, bool IdMode = true, bool TimeMode = false)
        {
            if (i1 == null && i2 == null) return true;
            if (i1 == null || i2 == null) return false;
            return EqualsPg(i1.event_id, i1.start_time, i1.durationSec, i1.Create64Key(),
                            i2.event_id, i2.start_time, i2.durationSec, i2.Create64Key(), IdMode, TimeMode);
        }
        public static bool EqualsPg(this EpgEventInfo i1, ReserveData i2, bool IdMode = true, bool TimeMode = false)
        {
            if (i1 == null && i2 == null) return true;
            if (i1 == null || i2 == null) return false;
            return EqualsPg(i1.event_id, i1.start_time, i1.durationSec, i1.Create64Key(),
                            i2.EventID, i2.StartTime, i2.DurationSecond, i2.Create64Key(), IdMode, TimeMode);
        }
        public static bool EqualsPg(this ReserveData i1, EpgEventInfo i2, bool IdMode = true, bool TimeMode = false)
        {
            return EqualsPg(i2, i1, IdMode, TimeMode);
        }

        public static ReserveData ConvertEpgToReserveData(EpgEventInfo epgInfo)
        {
            if (epgInfo == null) return null;
            var resInfo = new ReserveData();
            epgInfo.ConvertToReserveData(ref resInfo);
            return resInfo;
        }

        public static bool ConvertToReserveData(this EpgEventInfo epgInfo, ref ReserveData resInfo)
        {
            if (epgInfo == null || resInfo == null) return false;

            resInfo.Title = epgInfo.DataTitle;
            resInfo.StartTime = epgInfo.start_time;
            resInfo.StartTimeEpg = epgInfo.start_time;
            resInfo.DurationSecond = (epgInfo.DurationFlag == 0 ? 10 * 60 : epgInfo.durationSec);

            UInt64 key = epgInfo.Create64Key();
            if (ChSet5.ChList.ContainsKey(key) == true)
            {
                resInfo.StationName = ChSet5.ChList[key].ServiceName;
            }
            resInfo.OriginalNetworkID = epgInfo.original_network_id;
            resInfo.TransportStreamID = epgInfo.transport_stream_id;
            resInfo.ServiceID = epgInfo.service_id;
            resInfo.EventID = epgInfo.event_id;

            return true;
        }


        public static void RegulateData(this EpgSearchDateInfo info)
        {
            //早い終了時間を翌日のものとみなす
            Int32 start = (info.startHour) * 60 + info.startMin;
            Int32 end = (info.endHour) * 60 + info.endMin;
            while (end < start)
            {
                end += 24 * 60;
                info.endDayOfWeek = (byte)((info.endDayOfWeek + 1) % 7);
            }

            //28時間表示対応の処置。実際はシフトは1回で十分ではある。
            while (info.startHour >= 24) ShiftRecDayPart(1, ref info.startHour, ref info.startDayOfWeek);
            while (info.endHour >= 24) ShiftRecDayPart(1, ref info.endHour, ref info.endDayOfWeek);
        }
        private static void ShiftRecDayPart(int direction, ref ushort hour, ref byte weekFlg)
        {
            int shift_day = (direction >= 0 ? 1 : -1);
            hour = (ushort)((int)hour + -1 * shift_day * 24);
            weekFlg = (byte)((weekFlg + 7 + shift_day) % 7);
        }

        public static UInt64 Create64Key(this EpgServiceInfo obj)
        {
            return CommonManager.Create64Key(obj.ONID, obj.TSID, obj.SID);
        }

        public static UInt64 Create64Key(this EpgEventData obj)
        {
            return CommonManager.Create64Key(obj.original_network_id, obj.transport_stream_id, obj.service_id);
        }

        public static bool isOnTime(DateTime startTime, int duration)
        {
            if (startTime > System.DateTime.Now) return false;
            //
            return (startTime + TimeSpan.FromSeconds(duration) >= System.DateTime.Now);
        }

    }
}
