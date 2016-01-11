using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public static class ManualAutoAddDataEx
    {
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

        public static List<RecSettingData> RecSettingList(this List<ManualAutoAddData> list)
        {
            return list.Where(item => item != null).Select(item => item.recSetting).ToList();
        }

        public static UInt64 Create64Key(this ManualAutoAddData obj)
        {
            return CommonManager.Create64Key(obj.originalNetworkID, obj.transportStreamID, obj.serviceID);
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

    }
}
