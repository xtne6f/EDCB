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
            ViewMode = 0;
            NeedTimeOnlyBasic = false;
            NeedTimeOnlyWeek = false;
            StartTimeWeek = 4;
            SearchMode = false;
            SearchKey = new EpgSearchKeyInfo();
            SearchServiceFromView = false;
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
        public bool SearchMode { get; set; }
        public EpgSearchKeyInfo SearchKey { get; set; }
        public bool SearchServiceFromView { get; set; }
        public bool FilterEnded { get; set; }
        public int ID { get; set; }

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
            dest.SearchMode = src.SearchMode;
            dest.SearchServiceFromView = src.SearchServiceFromView;
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
