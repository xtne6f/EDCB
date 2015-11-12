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
            FilterEnded = false;
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
        public bool FilterEnded { get; set; }

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
            dest.FilterEnded = src.FilterEnded;
            dest.SearchKey = src.SearchKey.Clone();
        }

        public override string ToString()
        {
            return TabName;
        }
    }
}
