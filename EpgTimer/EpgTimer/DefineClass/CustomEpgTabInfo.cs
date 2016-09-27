using System;
using System.Collections.Generic;
using System.Linq;

namespace EpgTimer
{
    public class CustomEpgTabInfo
    {
        public CustomEpgTabInfo()
        {
            ViewServiceList = new List<UInt64>();
            ViewContentKindList = new List<UInt16>();
            ViewNotContentFlag = false;
            ViewMode = 0;
            NeedTimeOnlyBasic = false;
            NeedTimeOnlyWeek = false;
            StartTimeWeek = 4;
            SearchMode = false;
            SearchKey = new EpgSearchKeyInfo();
            SearchGenreNoSyncView = false;
            FilterEnded = false;
            ID = -1;
        }
        public String TabName { get; set; }
        public int ViewMode { get; set; }
        public bool NeedTimeOnlyBasic { get; set; }
        public bool NeedTimeOnlyWeek { get; set; }
        public int StartTimeWeek { get; set; }
        public List<UInt64> ViewServiceList { get; set; }
        public List<UInt16> ViewContentKindList { get; set; }
        public bool ViewNotContentFlag { get; set; }
        public bool SearchMode { get; set; }
        public EpgSearchKeyInfo SearchKey { get; set; }
        public bool SearchGenreNoSyncView { get; set; }
        public bool FilterEnded { get; set; }
        public int ID { get; set; }

        public EpgSearchKeyInfo GetSearchKeyReloadEpg()
        {
            EpgSearchKeyInfo key = SearchKey.Clone();
            key.serviceList = ViewServiceList.Select(id => (long)id).ToList();
            if (SearchGenreNoSyncView == false)
            {
                key.contentList = ViewContentKindList.Select(id => new EpgContentData { content_nibble_level_1 = (byte)(id >> 8), content_nibble_level_2 = (byte)id }).ToList();
                key.notContetFlag = (byte)(ViewNotContentFlag == true ? 1 : 0);
            }
            return key;
        }

        public static List<CustomEpgTabInfo> Clone(IEnumerable<CustomEpgTabInfo> src) { return CopyObj.Clone(src, CopyData); }
        public CustomEpgTabInfo Clone() { return CopyObj.Clone(this, CopyData); }
        protected static void CopyData(CustomEpgTabInfo src, CustomEpgTabInfo dest)
        {
            dest.TabName = src.TabName;
            dest.ViewMode = src.ViewMode;
            dest.NeedTimeOnlyBasic = src.NeedTimeOnlyBasic;
            dest.NeedTimeOnlyWeek = src.NeedTimeOnlyWeek;
            dest.StartTimeWeek = src.StartTimeWeek;
            dest.ViewServiceList = src.ViewServiceList.ToList();
            dest.ViewContentKindList = src.ViewContentKindList.ToList();
            dest.ViewNotContentFlag = src.ViewNotContentFlag;
            dest.SearchMode = src.SearchMode;
            dest.SearchGenreNoSyncView = src.SearchGenreNoSyncView;
            dest.FilterEnded = src.FilterEnded;
            dest.SearchKey = src.SearchKey.Clone();
            dest.ID = src.ID;
        }

        public override string ToString()
        {
            return TabName;
        }
    }

    public static class CustomEpgTabInfoEx
    {
        public static List<CustomEpgTabInfo> Clone(this IEnumerable<CustomEpgTabInfo> src) { return CustomEpgTabInfo.Clone(src); }
    }
}
