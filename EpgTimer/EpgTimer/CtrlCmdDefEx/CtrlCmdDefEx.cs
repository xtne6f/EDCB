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
    }
    public interface IAutoAddTargetData : IBasicPgInfo
    {
        UInt64 Create64PgKey();
        UInt64 CurrentPgUID();
        bool IsSamePg(IAutoAddTargetData data);
        bool IsOnAir();
        List<EpgAutoAddData> SearchEpgAutoAddList(bool? IsEnabled = null, bool ByFazy = false);
        List<ManualAutoAddData> SearchManualAutoAddList(bool? IsEnabled = null);
        List<EpgAutoAddData> GetEpgAutoAddList(bool? IsEnabled = null);
        List<ManualAutoAddData> GetManualAutoAddList(bool? IsEnabled = null);
    }

    public abstract class AutoAddTargetData : IAutoAddTargetData
    {
        public abstract string DataTitle { get; }
        public abstract DateTime PgStartTime { get; }
        public abstract uint PgDurationSecond { get; }
        public virtual UInt64 Create64Key() { return Create64PgKey() >> 16; }
        public abstract UInt64 Create64PgKey();
        public virtual UInt64 CurrentPgUID()
        {
            UInt64 key = Create64PgKey();
            //return (UInt64)(PgStartTime.Ticks) & 0xFFFFFFFF00000000 //上位32bitはこれでも分解能的にはOKだが‥(分解能約7分)
            return ((UInt64)(new TimeSpan(PgStartTime.Ticks).TotalDays)) << 32
                | ((UInt32)CommonManager.Create16Key(key >> 16)) << 16 | (UInt16)key;
        }
        //CurrentPgUID()は同一のEventIDの番組をチェックするが、こちらは放映時刻をチェックする。
        //プログラム予約が絡んでいる場合、結果が変わってくる。
        public virtual bool IsSamePg(IAutoAddTargetData data)
        {
            if (data == null) return false;
            return PgStartTime == data.PgStartTime && PgDurationSecond == data.PgDurationSecond && Create64Key() == data.Create64Key();
        }
        public bool IsOnAir()
        {
            return CtrlCmdDefEx.isOnTime(PgStartTime, (int)PgDurationSecond);
        }

        public virtual List<EpgAutoAddData> SearchEpgAutoAddList(bool? IsEnabled = null, bool ByFazy = false)
        {
            return SearchEpgAutoAddHitList(this, IsEnabled, ByFazy);
        }
        public virtual List<ManualAutoAddData> SearchManualAutoAddList(bool? IsEnabled = null)
        {
            return GetManualAutoAddHitList(this, IsEnabled);
        }
        public virtual List<EpgAutoAddData> GetEpgAutoAddList(bool? IsEnabled = null)
        {
            return GetEpgAutoAddHitList(this, IsEnabled);
        }
        public virtual List<ManualAutoAddData> GetManualAutoAddList(bool? IsEnabled = null)
        {
            return GetManualAutoAddHitList(this, IsEnabled);
        }

        public static List<EpgAutoAddData> SearchEpgAutoAddHitList(IAutoAddTargetData info, bool? IsEnabled = null, bool ByFazy = false)
        {
            if (info == null) return new List<EpgAutoAddData>();
            //
            var list = GetEpgAutoAddHitList(info, IsEnabled);
            if (ByFazy == true)
            {
                list.AddRange(MenuUtil.FazySearchEpgAutoAddData(info.DataTitle, IsEnabled));
                list = list.Distinct().OrderBy(data => data.DataID).ToList();
            }
            return list;
        }
        public static List<EpgAutoAddData> GetEpgAutoAddHitList(IAutoAddTargetData info, bool? IsEnabled = null)
        {
            return CommonManager.Instance.DB.EpgAutoAddList.Values.GetAutoAddList(IsEnabled)
                .FindAll(data => data.CheckPgHit(info) == true);//info==nullでもOK
        }
        public static List<ManualAutoAddData> GetManualAutoAddHitList(IAutoAddTargetData info, bool? IsEnabled = null)
        {
            return CommonManager.Instance.DB.ManualAutoAddList.Values.GetAutoAddList(IsEnabled)
                .FindAll(data => data.CheckPgHit(info) == true);//info==nullでもOK
        }
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

        public static List<EpgServiceEventInfo> CopyTable(this IEnumerable<EpgServiceEventInfo> src) { return CopyObj.Clone(src, CopyTableData); }
        public static EpgServiceEventInfo CopyTable(this EpgServiceEventInfo src) { return CopyObj.Clone(src, CopyTableData); }
        public static void CopyTableTo(this EpgServiceEventInfo src, EpgServiceEventInfo dest) { CopyObj.CopyTo(src, dest, CopyTableData); }
        private static void CopyTableData(EpgServiceEventInfo src, EpgServiceEventInfo dest)
        {
            dest.serviceInfo = src.serviceInfo;
            dest.eventList = src.eventList.ToList();
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
            resInfo.DurationSecond = epgInfo.PgDurationSecond;

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
