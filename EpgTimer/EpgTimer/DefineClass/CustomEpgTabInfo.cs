using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

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
        public String TabName
        {
            get;
            set;
        }
        public int ViewMode
        {
            get;
            set;
        }
        public bool NeedTimeOnlyBasic
        {
            get;
            set;
        }
        public bool NeedTimeOnlyWeek
        {
            get;
            set;
        }
        public int StartTimeWeek
        {
            get;
            set;
        }
        public List<UInt64> ViewServiceList
        {
            get;
            set;
        }
        public List<UInt16> ViewContentKindList
        {
            get;
            set;
        }
        public bool SearchMode
        {
            get;
            set;
        }
        public EpgSearchKeyInfo SearchKey
        {
            get;
            set;
        }
        public bool FilterEnded
        {
            get;
            set;
        }

        public void CopyTo(ref CustomEpgTabInfo dest)
        {
            dest.TabName = TabName;
            dest.ViewMode = ViewMode;
            dest.NeedTimeOnlyBasic = NeedTimeOnlyBasic;
            dest.NeedTimeOnlyWeek = NeedTimeOnlyWeek;
            dest.StartTimeWeek = StartTimeWeek;
            dest.ViewServiceList = ViewServiceList.ToList();
            dest.ViewContentKindList = ViewContentKindList.ToList();
            dest.SearchMode = SearchMode;
            dest.FilterEnded = FilterEnded;

            dest.SearchKey.aimaiFlag = SearchKey.aimaiFlag;
            dest.SearchKey.andKey = SearchKey.andKey;
            dest.SearchKey.audioList = SearchKey.audioList.ToList();
            dest.SearchKey.contentList = SearchKey.contentList.ToList();
            dest.SearchKey.dateList = SearchKey.dateList.ToList();
            dest.SearchKey.freeCAFlag = SearchKey.freeCAFlag;
            dest.SearchKey.notContetFlag = SearchKey.notContetFlag;
            dest.SearchKey.notDateFlag = SearchKey.notDateFlag;
            dest.SearchKey.notKey = SearchKey.notKey;
            dest.SearchKey.regExpFlag = SearchKey.regExpFlag;
            dest.SearchKey.serviceList = SearchKey.serviceList.ToList();
            dest.SearchKey.titleOnlyFlag = SearchKey.titleOnlyFlag;
            dest.SearchKey.videoList = SearchKey.videoList.ToList();
        }
        public override string ToString()
        {
            return TabName;
        }
    }
}
