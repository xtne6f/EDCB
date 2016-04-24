using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public abstract class AutoAddData : IRecWorkMainData, IRecSetttingData
    {
        //IRecWorkMainData
        public abstract string DataTitle { get; set; }
        //
        public abstract uint DataID { get; set; }
        public abstract bool IsEnabled { get; set; }
        public abstract RecSettingData RecSettingInfo { get; }
        public virtual bool IsManual { get { return false; } }

        /*
        public bool CheckResHit(ReserveData data)
        {
            if (data == null) return false;
            return this.GetReserveList().FirstOrDefault(info => info.ReserveID == data.ReserveID) != null;
        }*/
        public abstract bool CheckPgHit(IAutoAddTargetData data);

        public static AutoAddData AutoAddList(Type t, uint id)
        {
            if (t == typeof(EpgAutoAddData))
            {
                return CommonManager.Instance.DB.EpgAutoAddList[id];
            }
            else if (t == typeof(ManualAutoAddData))
            {
                return CommonManager.Instance.DB.ManualAutoAddList[id];
            }
            return null;
        }

        //AppendData 関係。ID(元データ)に対して一意の情報なので、データ自体はDB側。
        protected virtual AutoAddDataAppend Append { get { return new AutoAddDataAppend(); } }
        public virtual uint SearchCount { get { return 0; } }
        public uint ReserveCount { get { return (uint)Append.ReseveItemList.Count; } }
        public uint OnCount { get { return Append.OnCount; } }
        public uint OffCount { get { return Append.OffCount; } }
        public virtual List<ReserveData> GetReserveList() { return Append.ReseveItemList; }
        public ReserveData GetNextReserve() { return Append.NextReserve; }
    }

    static class AutoAddDataEx
    {
        public static List<ReserveData> GetReserveList(this IEnumerable<AutoAddData> mlist)
        {
            var retList = new List<ReserveData>();
            foreach (AutoAddData info in mlist) retList.AddRange(info.GetReserveList());
            return retList.Distinct().ToList();
        }
        public static List<T> GetAutoAddList<T>(this IEnumerable<T> mlist, bool? IsEnabled = null) where T : AutoAddData
        {
            return IsEnabled == null ? mlist.ToList() : mlist.Where(data => data.IsEnabled == IsEnabled).ToList();
        }
    }

    public partial class EpgAutoAddData : AutoAddData
    {
        public override string DataTitle { get { return searchInfo.andKey; } set { searchInfo.andKey = value; } }
        public override uint DataID { get { return dataID; } set { dataID = value; } }
        public override bool IsEnabled { get { return searchInfo.keyDisabledFlag == 0; } set { searchInfo.keyDisabledFlag = (byte)(value == true ? 0 : 1); } }
        public override RecSettingData RecSettingInfo { get { return recSetting; } }

        public override bool CheckPgHit(IAutoAddTargetData data)
        {
            if (data == null) return false;
            return this.GetSearchList().FirstOrDefault(info => info.EventInfo.Create64PgKey() == data.Create64PgKey()) != null;
        }

        //EpgAutoAddDataAppend 追加分
        protected override AutoAddDataAppend Append { get { return CommonManager.Instance.DB.GetEpgAutoAddDataAppend(this); } }
        public override uint SearchCount { get { return (uint)(Append as EpgAutoAddDataAppend).SearchItemList.Count; } }
        public List<SearchItem> GetSearchList() { return (Append as EpgAutoAddDataAppend).SearchItemList; }
    }

    public static class EpgAutoAddDataEx
    {
        public static List<EpgAutoAddData> Clone(this IEnumerable<EpgAutoAddData> src) { return CopyObj.Clone(src, CopyData); }
        public static EpgAutoAddData Clone(this EpgAutoAddData src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this EpgAutoAddData src, EpgAutoAddData dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(EpgAutoAddData src, EpgAutoAddData dest)
        {
            dest.addCount = src.addCount;
            dest.dataID = src.dataID;
            dest.recSetting = src.recSetting.Clone();       //RecSettingData
            dest.searchInfo = src.searchInfo.Clone();       //EpgSearchKeyInfo
        }

        public static List<EpgSearchKeyInfo> RecSearchKeyList(this IEnumerable<EpgAutoAddData> list)
        {
            return list.Where(item => item != null).Select(item => item.searchInfo).ToList();
        }
    }

    public partial class ManualAutoAddData : AutoAddData, IBasicPgInfo
    {
        public override string DataTitle { get { return title; } set { title = value; } }
        public DateTime PgStartTime { get { return new DateTime(2000, 1, 1).AddSeconds(startTime); } }
        public uint PgDurationSecond { get { return durationSecond; } }
        public UInt64 Create64Key()
        {
            return CommonManager.Create64Key(originalNetworkID, transportStreamID, serviceID);
        }
        public UInt64 Create64PgKey()
        {
            return CommonManager.Create64PgKey(originalNetworkID, transportStreamID, serviceID, 0xFFFF);
        }

        public override uint DataID { get { return dataID; } set { dataID = value; } }
        public override bool IsEnabled { get { return keyDisabledFlag == 0; } set { keyDisabledFlag = (byte)(value == true ? 0 : 1); } }
        public override RecSettingData RecSettingInfo { get { return recSetting; } }
        public override bool IsManual { get { return true; } }
        public override bool CheckPgHit(IAutoAddTargetData data)
        {
            if (data == null) return false;
            return Create64Key() == data.Create64Key()
                && startTime == data.PgStartTime.Hour * 3600 + data.PgStartTime.Minute * 60 + data.PgStartTime.Second
                && durationSecond == data.PgDurationSecond
                && (dayOfWeekFlag & (byte)(0x01 << (int)data.PgStartTime.DayOfWeek)) != 0;
        }

        public void RegulateData()
        {
            while (startTime >= 24 * 60 * 60) ShiftRecDay(1);
        }
        public void ShiftRecDay(int direction)
        {
            startTime = (uint)((int)startTime + (direction >= 0 ? -1 : 1) * 24 * 60 * 60);
            dayOfWeekFlag = ShiftWeekFlag(dayOfWeekFlag, direction);
        }
        public static byte ShiftWeekFlag(byte flg, int direction)
        {
            if (direction >= 0)
            {
                return (byte)(0x7E & ((int)flg << 1) | ((flg & 0x40) != 0 ? 0x01 : 0x00));
            }
            else
            {
                return (byte)(0x3F & ((int)flg >> 1) | ((flg & 0x01) != 0 ? 0x40 : 0x00));
            }
        }
        
        //AutoAddDataAppend
        protected override AutoAddDataAppend Append { get { return CommonManager.Instance.DB.GetManualAutoAddDataAppend(this); } }
        public override uint SearchCount { get { return (uint)CommonUtil.NumBits(dayOfWeekFlag); } }
    }
    public static class ManualAutoAddDataEx
    {
        public static List<ManualAutoAddData> Clone(this IEnumerable<ManualAutoAddData> src) { return CopyObj.Clone(src, CopyData); }
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
            dest.keyDisabledFlag = src.keyDisabledFlag;
        }
    }

    public class AutoAddDataAppend
    {
        public AutoAddDataAppend(List<ReserveData> reservelist = null)
        {
            ReseveItemList = reservelist != null ? reservelist : new List<ReserveData>();
            NextReserve = null;
            OnCount = 0;
            OffCount = 0;
        }

        public List<ReserveData> ReseveItemList { get; protected set; }
        public ReserveData NextReserve { get; protected set; }
        public uint OnCount { get; protected set; }
        public uint OffCount { get; protected set; }

        //情報の更新をする。
        public virtual void UpdateCounts()
        {
            NextReserve = ReseveItemList.GetNextReserve();
            OnCount = (uint)ReseveItemList.Count(info => info.IsEnabled == true);
            OffCount = (uint)ReseveItemList.Count - OnCount;
        }
    }
    public class EpgAutoAddDataAppend : AutoAddDataAppend
    {
        public EpgAutoAddDataAppend(List<EpgEventInfo> eventlist = null)
            : base()
        {
            EpgEventList = eventlist != null ? eventlist : new List<EpgEventInfo>();
            SearchItemList = new List<SearchItem>();
        }

        public List<EpgEventInfo> EpgEventList { get; protected set; }
        public List<SearchItem> SearchItemList { get; protected set; }

        //情報の更新をする。
        public override void UpdateCounts()
        {
            SearchItemList = new List<SearchItem>();
            SearchItemList.AddFromEventList(EpgEventList, false, true);
            ReseveItemList = SearchItemList.GetReserveList();
            base.UpdateCounts();
        }
    }
}
