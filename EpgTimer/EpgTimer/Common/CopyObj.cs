using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public static class CopyObjEx
    {
        //シリアライズとかもろもろは使えないので地道にコピーする。
        //ジェネリックは手続きの統一用で、各メソッドは別途定義する。

        private static List<T> Clone<T>(this List<T> src, Action<T, T> CopyData) where T : class, new()
        {
            if (src == null) return null; //普通にnullで呼び出せてしまう
            List<T> copylist = new List<T>();
            foreach (T item in src) copylist.Add(item.Clone(CopyData));
            return copylist;
        }
        private static T Clone<T>(this T src, Action<T, T> CopyData) where T : class, new()
        {
            if (src == null) return null; //普通にnullで呼び出せてしまう
            T copyobj = new T();
            src.CopyTo(copyobj, CopyData);
            return copyobj;
        }
        private static void CopyTo<T>(this T src, T dest, Action<T, T> CopyData)
        {
            if (src == null || dest == null) return;
            CopyData(src, dest);
        }

        public static List<EpgSearchKeyInfo> Clone(this List<EpgSearchKeyInfo> src) { return src.Clone<EpgSearchKeyInfo>(CopyData); }
        public static EpgSearchKeyInfo Clone(this EpgSearchKeyInfo src) { return src.Clone<EpgSearchKeyInfo>(CopyData); }
        public static void CopyTo(this EpgSearchKeyInfo src, EpgSearchKeyInfo dest) { src.CopyTo<EpgSearchKeyInfo>(dest, CopyData); }
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
        }

        public static List<EpgContentData> Clone(this List<EpgContentData> src) { return src.Clone<EpgContentData>(CopyData); }
        public static EpgContentData Clone(this EpgContentData src) { return src.Clone<EpgContentData>(CopyData); }
        public static void CopyTo(this EpgContentData src, EpgContentData dest) { src.CopyTo<EpgContentData>(dest, CopyData); }
        private static void CopyData(EpgContentData src, EpgContentData dest)
        {
            dest.content_nibble_level_1 = src.content_nibble_level_1;
            dest.content_nibble_level_2 = src.content_nibble_level_2;
            dest.user_nibble_1 = src.user_nibble_1;
            dest.user_nibble_2 = src.user_nibble_2;
        }

        public static List<EpgSearchDateInfo> Clone(this List<EpgSearchDateInfo> src) { return src.Clone<EpgSearchDateInfo>(CopyData); }
        public static EpgSearchDateInfo Clone(this EpgSearchDateInfo src) { return src.Clone<EpgSearchDateInfo>(CopyData); }
        public static void CopyTo(this EpgSearchDateInfo src, EpgSearchDateInfo dest) { src.CopyTo<EpgSearchDateInfo>(dest, CopyData); }
        private static void CopyData(EpgSearchDateInfo src, EpgSearchDateInfo dest)
        {
            dest.endDayOfWeek = src.endDayOfWeek;
            dest.endHour = src.endHour;
            dest.endMin = src.endMin;
            dest.startDayOfWeek = src.startDayOfWeek;
            dest.startHour = src.startHour;
            dest.startMin = src.startMin;
        }

    }
}
