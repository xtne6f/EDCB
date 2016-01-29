using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Media;
using System.Windows.Controls;
using System.Windows;

namespace EpgTimer
{
    public class SearchItem
    {
        protected MenuUtil mutil = CommonManager.Instance.MUtil;
        protected ViewUtil vutil = CommonManager.Instance.VUtil;

        public virtual EpgEventInfo EventInfo { get; set; }
        public ReserveData ReserveInfo { get; set; }

        public SearchItem() { }
        public SearchItem(EpgEventInfo item)
        {
            EventInfo = item;
        }

        public static string GetValuePropertyName(string key)
        {
            var obj = new SearchItem();
            if (key == CommonUtil.GetMemberName(() => obj.StartTime))
            {
                return CommonUtil.GetMemberName(() => obj.StartTimeValue);
            }
            else if (key == CommonUtil.GetMemberName(() => obj.ProgramDuration))
            {
                return CommonUtil.GetMemberName(() => obj.ProgramDurationValue);
            }
            else
            {
                return key;
            }
        }

        public bool IsReserved
        {
            get
            {
                return (ReserveInfo != null);
            }
        }
        public virtual String EventName
        {
            get
            {
                if (EventInfo == null) return "";
                //
                return EventInfo.DataTitle;
            }
        }
        public virtual String ServiceName
        {
            get
            {
                if (EventInfo != null)
                {
                    UInt64 serviceKey = EventInfo.Create64Key();
                    if (ChSet5.Instance.ChList.ContainsKey(serviceKey) == true)
                    {
                        return ChSet5.Instance.ChList[serviceKey].ServiceName;
                    }
                }
                return "";
            }
        }
        public virtual String NetworkName
        {
            get
            {
                if (EventInfo == null) return "";
                //
                return CommonManager.ConvertNetworkNameText(EventInfo.original_network_id);
            }
        }
        public virtual String StartTime
        {
            get
            {
                if (EventInfo == null) return "";
                if (EventInfo.StartTimeFlag == 0) return "未定";
                //
                return CommonManager.ConvertTimeText(EventInfo.start_time, Settings.Instance.ResInfoNoYear, Settings.Instance.ResInfoNoSecond);
            }
        }
        public virtual DateTime StartTimeValue
        {
            get
            {
                if (EventInfo == null || EventInfo.StartTimeFlag == 0) return new DateTime();
                //
                return EventInfo.start_time;
            }
        }
        /// <summary>
        /// 番組放送時間(長さ)
        /// </summary>
        public virtual String ProgramDuration
        {
            get
            {
                if (EventInfo == null) return "";
                if (EventInfo.DurationFlag == 0) return "不明";
                //
                return CommonManager.ConvertDurationText(EventInfo.durationSec, Settings.Instance.ResInfoNoDurSecond);
            }
        }
        public virtual UInt32 ProgramDurationValue
        {
            get
            {
                if (EventInfo == null || EventInfo.DurationFlag == 0) return UInt32.MinValue;
                //
                return EventInfo.durationSec;
            }
        }
        /// <summary>
        /// 番組内容
        /// </summary>
        public virtual String ProgramContent
        {
            get
            {
                if (EventInfo == null || EventInfo.ShortInfo == null) return "";
                //
                return EventInfo.ShortInfo.text_char.Replace("\r\n", " ");
            }
        }
        public String JyanruKey
        {
            get
            {
                if (EventInfo == null) return "";
                //
                return CommonManager.Instance.ConvertJyanruText(EventInfo);
            }
        }
        public virtual TextBlock ToolTipView
        {
            get
            {
                if (Settings.Instance.NoToolTip == true) return null;
                if (EventInfo == null) return mutil.GetTooltipBlockStandard("");
                //
                return mutil.GetTooltipBlockStandard(CommonManager.Instance.ConvertProgramText(EventInfo, EventInfoTextMode.All));
            }
        }
        public override string ToString()
        {
            return CommonManager.Instance.ConvertTextSearchString(this.EventName);
        }
        public virtual String Status
        {
            get
            {
                String[] wiewString = { "", "予", "無", "放", "予+", "無+", "録*", "無*" };
                int index = 0;
                if (EventInfo != null)
                {
                    if (EventInfo.IsOnAir() == true)
                    {
                        index = 3;
                    }
                    if (IsReserved == true)
                    {
                        if (ReserveInfo.IsOnRec() == true)//マージンがあるので、IsOnAir==trueとは限らない
                        {
                            index = 5;
                        }
                        if (ReserveInfo.RecSetting.RecMode == 5) //無効の判定
                        {
                            index += 2;
                        }
                        else
                        {
                            index += 1;
                        }
                    }
                }
                return wiewString[index];
            }
        }
        public virtual SolidColorBrush StatusColor
        {
            get
            {
                if (EventInfo != null)
                {
                    if (IsReserved == true)
                    {
                        if (ReserveInfo.IsOnRec() == true)
                        {
                            return CommonManager.Instance.StatRecForeColor;
                        }
                    }
                    if (EventInfo.IsOnAir() == true)
                    {
                        return CommonManager.Instance.StatOnAirForeColor;
                    }
                }
                return CommonManager.Instance.StatResForeColor;
            }
        }
        public int NowJumpingTable { set; get; }
        public SolidColorBrush ForeColor
        {
            get
            {
                //番組表へジャンプ時の強調表示
                switch(NowJumpingTable)
                {
                    case 1: return Brushes.Red;
                    case 2: return CommonManager.Instance.ListDefForeColor;
                }

                //通常表示
                if (ReserveInfo == null) return CommonManager.Instance.ListDefForeColor;
                //
                return CommonManager.Instance.RecModeForeColor[ReserveInfo.RecSetting.RecMode];
            }
        }
        public SolidColorBrush BackColor
        {
            get
            {
                //番組表へジャンプ時の強調表示
                switch (NowJumpingTable)
                {
                    case 1: return CommonManager.Instance.ResDefBackColor;
                    case 2: return Brushes.Red;
                }

                //通常表示
                return vutil.ReserveErrBrush(ReserveInfo);
            }
        }
        public Brush BorderBrush
        {
            get
            {
                return vutil.EpgDataContentBrush(EventInfo);
            }
        }
    }

    public static class SearchItemEx
    {
        public static List<EpgEventInfo> GetEventList(this IEnumerable<SearchItem> list)
        {
            return list.Where(item => item != null).Select(item => item.EventInfo).ToList();
        }
        public static List<EpgEventInfo> GetNoReserveList(this IEnumerable<SearchItem> list)
        {
            return list.Where(item => item != null && item.IsReserved == false).Select(item => item.EventInfo).ToList();
        }
        public static List<ReserveData> GetReserveList(this IEnumerable<SearchItem> list)
        {
            return list.Where(item => item != null && item.IsReserved == true).Select(item => item.ReserveInfo).ToList();
        }
        //public static bool HasReserved(this IEnumerable<SearchItem> list)
        //{
        //    return list.Any(info => item != null && item.IsReserved == false);
        //}
        //public static bool HasNoReserved(this IEnumerable<SearchItem> list)
        //{
        //    return list.Any(info => item != null && item.IsReserved == true);
        //}
        public static void AddFromEventList(this ICollection<SearchItem> itemlist, ICollection<EpgEventInfo> eventList, bool isExceptUnknownStartTime, bool isExceptEnded)
        {
            if (itemlist == null) return;
            //
            DateTime now = DateTime.Now;
            foreach (EpgEventInfo info in eventList.OfType<EpgEventInfo>())
            {
                //開始未定を除外
                if (isExceptUnknownStartTime == true)
                {
                    if (info.StartTimeFlag == 0) continue;
                }
                //時間の過ぎているものを除外
                if (isExceptEnded == true)
                {
                    if (info.start_time.AddSeconds(info.DurationFlag == 0 ? 0 : info.durationSec) < now) continue;
                }

                itemlist.Add(new SearchItem(info));
            }
            itemlist.SetReserveData();
        }

        public static void SetReserveData(this ICollection<SearchItem> list)
        {
            var listKeys = new Dictionary<ulong, SearchItem>();

            foreach (SearchItem listItem1 in list)
            {
                //重複するキーは基本的に無いという前提
                try
                {
                    listKeys.Add(listItem1.EventInfo.Create64PgKey(), listItem1);
                    listItem1.ReserveInfo = null;
                }
                catch { }
            }

            SearchItem setItem;
            foreach (ReserveData data in CommonManager.Instance.DB.ReserveList.Values)
            {
                if (listKeys.TryGetValue(data.Create64PgKey(), out setItem))
                {
                    setItem.ReserveInfo = data;
                }
            }
        }

    }
}
