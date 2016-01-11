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
            return CommonManager.Instance.DB.GetEpgAutoAddDataAppend(master).SearchCount;
        }
        public static uint ReserveCount(this EpgAutoAddData master)
        {
            return CommonManager.Instance.DB.GetEpgAutoAddDataAppend(master).ReserveCount;
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
        public EpgAutoAddDataAppend(EpgAutoAddData master1, List<EpgEventInfo> eventlist = null)
        {
            master = master1;
            SetEpgData(eventlist);
        }

        private EpgAutoAddData master;
        private List<EpgEventInfo> epgEventList;
        private List<SearchItem> searchItemList;
        private List<ReserveData> reseveItemList;
        private ReserveData nextReserve;
        private uint searchCount;
        private uint onCount;
        private uint offCount;

        //予約情報の更新があったとき、CommonManager.Instance.DB.epgAutoAddAppendList()に入っていればフラグを立ててもらえる。
        public bool updateCounts;

        public EpgAutoAddData Master            { get { return master; } }
        public uint dataID                      { get { return (master != null ? master.dataID : 0); } }
        public List<EpgEventInfo> EpgEventList  { get { return epgEventList; } }
        public List<SearchItem> SearchItemList  { get { RefreshData(); return searchItemList; } }
        public List<ReserveData> ReseveItemList { get { RefreshData(); return reseveItemList; } }
        public ReserveData NextReserve          { get { RefreshData(); return nextReserve; } }
        public uint SearchCount                 { get { RefreshData(); return searchCount; } }
        public uint ReserveCount                { get { RefreshData(); return onCount + offCount; } }
        public uint OnCount                     { get { RefreshData(); return onCount; } }
        public uint OffCount                    { get { RefreshData(); return offCount; } }

        public void SetEpgData(List<EpgEventInfo> eventlist)
        {
            epgEventList = eventlist != null ? eventlist : new List<EpgEventInfo>();
            searchItemList = new List<SearchItem>();
            reseveItemList = new List<ReserveData>();
            nextReserve = null;
            searchCount = 0;
            onCount = 0;
            offCount = 0;
            updateCounts = true;
        }

        //必要なら情報の更新をする。
        public void RefreshData()
        {
            if (updateCounts == false) return;
            updateCounts = false;

            searchItemList = new List<SearchItem>();
            searchItemList.AddFromEventList(epgEventList, false, true);

            reseveItemList = searchItemList.GetReserveList();

            searchCount = (uint)searchItemList.Count;
            onCount = (uint)reseveItemList.Count(info => info.RecSetting.RecMode != 5);
            offCount = (uint)reseveItemList.Count - onCount;
            nextReserve = reseveItemList.GetNextReserve();
        }

    }
}
