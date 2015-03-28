using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Media;
using System.Windows.Controls;
using System.Windows;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public class SearchItem : ProgramItemCommon
    {
        //EventInfo、ReserveInfo、JyanruKey、ForeColor、BackColor、BorderBrushはベースクラス

        private MenuUtil mutil = CommonManager.Instance.MUtil;

        public String EventName
        {
            get
            {
                String view = "";
                if (EventInfo != null)
                {
                    view = EventInfo.Title();
                }
                return view;
            }
        }
        public String ServiceName
        {
            get
            {
                String view = "";
                if (EventInfo != null)
                {
                    UInt64 serviceKey = EventInfo.Create64Key();
                    if (ChSet5.Instance.ChList.ContainsKey(serviceKey) == true)
                    {
                        view = ChSet5.Instance.ChList[serviceKey].ServiceName;
                    }
                }
                return view;
            }
        }
        public String NetworkName
        {
            get
            {
                String view = "";
                if (EventInfo != null)
                {
                    view = CommonManager.Instance.ConvertNetworkNameText(EventInfo.original_network_id);
                }
                return view;
            }
        }
        public String StartTime
        {
            get
            {
                String view = "未定";
                if (EventInfo != null)
                {
                    view = EventInfo.start_time.ToString("yyyy/MM/dd(ddd) HH:mm:ss");
                }
                return view;
            }
        }
        public String Status
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
        public SolidColorBrush StatusColor
        {
            get
            {
                SolidColorBrush color = CommonManager.Instance.StatResForeColor;
                if (EventInfo != null)
                {
                    if (EventInfo.IsOnAir() == true)
                    {
                        color = CommonManager.Instance.StatOnAirForeColor;
                    }
                    if (IsReserved == true)
                    {
                        if (ReserveInfo.IsOnRec() == true)
                        {
                            color = CommonManager.Instance.StatRecForeColor;
                        }
                    }
                }
                return color;
            }
        }
        public bool IsReserved
        {
            get
            {
                return (ReserveInfo != null);
            }
        }
        public TextBlock ToolTipView
        {
            get
            {
                if (Settings.Instance.NoToolTip == true) return null;

                String view = "";
                if (EventInfo != null)
                {
                    view = CommonManager.Instance.ConvertProgramText(EventInfo, EventInfoTextMode.All);
                }

                return mutil.GetTooltipBlockStandard(view);
            }
        }

        /// <summary>
        /// 番組放送時間（長さ）
        /// </summary>
        public TimeSpan ProgramDuration
        {
            get
            {
                if (EventInfo == null) { return new TimeSpan(); }
                //
                return TimeSpan.FromSeconds(EventInfo.durationSec);
            }
        }

        /// <summary>
        /// 番組内容
        /// </summary>
        public String ProgramContent
        {
            get
            {
                if (EventInfo == null) { return null; }
                //
                if (Settings.Instance.FixSearchResult)
                {
                    return "省略";
                }
                else
                {
                    return EventInfo.ShortInfo.text_char.Replace("\r\n", " ");
                }
            }
        }
    }

    public static class SearchItemEx
    {
        public static List<EpgEventInfo> EventInfoList(this ICollection<SearchItem> itemlist)
        {
            return itemlist.Where(item => item != null).Select(item => item.EventInfo).ToList();
        }
        public static List<EpgEventInfo> NoReserveInfoList(this ICollection<SearchItem> itemlist)
        {
            return itemlist.Where(item => item == null ? false : item.IsReserved == false).Select(item => item.EventInfo).ToList();
        }
        public static List<ReserveData> ReserveInfoList(this ICollection<SearchItem> itemlist)
        {
            return itemlist.Where(item => item == null ? false : item.IsReserved == true).Select(item => item.ReserveInfo).ToList();
        }
        public static bool HasReserved(this List<SearchItem> list)
        {
            return list.Any(info => info == null ? false : info.IsReserved);
        }
        public static bool HasNoReserved(this List<SearchItem> list)
        {
            return list.Any(info => info == null ? false : !info.IsReserved);
        }

    }
}
