using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public static class EpgAutoAddDataEx
    {
        public static uint SearchCount(this EpgAutoAddData master)
        {
            return (uint)CommonManager.Instance.DB.GetEpgAutoAddDataAppend(master).SearchItemList.Count;
        }
        public static uint ReserveCount(this EpgAutoAddData master)
        {
            return (uint)CommonManager.Instance.DB.GetEpgAutoAddDataAppend(master).ReseveItemList.Count;
        }
        public static uint OnCount(this EpgAutoAddData master)
        {
            return CommonManager.Instance.DB.GetEpgAutoAddDataAppend(master).OnCount;
        }
        public static uint OffCount(this EpgAutoAddData master)
        {
            return CommonManager.Instance.DB.GetEpgAutoAddDataAppend(master).OffCount;
        }
        public static List<SearchItem> GetSearchList(this EpgAutoAddData master)
        {
            return CommonManager.Instance.DB.GetEpgAutoAddDataAppend(master).SearchItemList;
        }
        public static List<ReserveData> GetReserveList(this EpgAutoAddData master)
        {
            return CommonManager.Instance.DB.GetEpgAutoAddDataAppend(master).ReseveItemList;
        }
        public static ReserveData GetNextReserve(this EpgAutoAddData master)
        {
            return CommonManager.Instance.DB.GetEpgAutoAddDataAppend(master).NextReserve;
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

        public static List<RecSettingData> RecSettingList(this List<EpgAutoAddData> list)
        {
            return list.Where(item => item != null).Select(item => item.recSetting).ToList();
        }
        public static List<EpgSearchKeyInfo> RecSearchKeyList(this List<EpgAutoAddData> list)
        {
            return list.Where(item => item != null).Select(item => item.searchInfo).ToList();
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
    }

    public class EpgAutoAddDataAppend
    {
        public EpgAutoAddDataAppend(List<EpgEventInfo> eventlist = null)
        {
            EpgEventList = eventlist != null ? eventlist : new List<EpgEventInfo>();
            SearchItemList = new List<SearchItem>();
            ReseveItemList = new List<ReserveData>();
            NextReserve = null;
            OnCount = 0;
            OffCount = 0;
        }

        public List<EpgEventInfo> EpgEventList { get; private set; }
        public List<SearchItem> SearchItemList { get; private set; }
        public List<ReserveData> ReseveItemList { get; private set; }
        public ReserveData NextReserve { get; private set; }
        public uint OnCount { get; private set; }
        public uint OffCount { get; private set; }

        //情報の更新をする。
        public void UpdateCounts()
        {
            SearchItemList = new List<SearchItem>();
            SearchItemList.AddFromEventList(EpgEventList, false, true);
            ReseveItemList = SearchItemList.GetReserveList();
            NextReserve = ReseveItemList.GetNextReserve();
            OnCount = (uint)ReseveItemList.Count(info => info.RecSetting.RecMode != 5);
            OffCount = (uint)ReseveItemList.Count - OnCount;
        }

    }
}
