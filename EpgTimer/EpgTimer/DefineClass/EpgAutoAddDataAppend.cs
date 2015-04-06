using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

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
    }

    public class EpgAutoAddDataAppend
    {
        public EpgAutoAddDataAppend(EpgAutoAddData master1, List<EpgEventInfo> eventlist = null)
        {
            master = master1;
            searchItemList = eventlist;
        }

        private EpgAutoAddData master = null;
        private List<EpgEventInfo> searchItemList = null;
        private uint searchCount = 0;
        private uint onCount = 0;
        private uint offCount = 0;

        //予約情報の更新があったとき、CommonManager.Instance.DB.epgAutoAddAppendList()に入っていればフラグを立ててもらえる。
        public bool updateCounts = true;

        public EpgAutoAddData Master{ get { return master; } }
        public uint dataID          { get { return (master != null ? master.dataID : 0); } }
        public uint SearchCount     { get { RefreshData(); return searchCount; } }
        public uint ReserveCount    { get { RefreshData(); return onCount + offCount; } }
        public uint OnCount         { get { RefreshData(); return onCount; } }
        public uint OffCount        { get { RefreshData(); return offCount; } }

        public List<EpgEventInfo> SearchItemList
        {
            get
            {
                if (searchItemList == null)
                {
                    this.SearchItemList = new List<EpgEventInfo>();
                }
                return searchItemList;
            }
            set
            {
                searchItemList = value;
                updateCounts = true;
            }
        }

        //必要なら情報の更新をする。
        public void RefreshData()
        {
            if (updateCounts == false) return;
            updateCounts = false;

            searchCount = 0;
            onCount = 0;
            offCount = 0;

            if (master == null || searchItemList == null) return;

            var slist = new List<SearchItem>();
            foreach (EpgEventInfo info in searchItemList)
            {
                if (info.start_time.AddSeconds(info.durationSec) > DateTime.Now)
                {
                    slist.Add(new SearchItem(info));
                }
            }
            CommonManager.Instance.MUtil.SetSearchItemReserved(slist);

            var rlist = slist.ReserveInfoList();

            searchCount = (uint)slist.Count;
            onCount = (uint)rlist.Count(info => info.RecSetting.RecMode != 5);
            offCount = (uint)rlist.Count - onCount;
        }

    }
}
