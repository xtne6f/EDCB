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
    public class ReserveItem : SearchItem
    {
        //EventInfo、ReserveInfo、JyanruKey、ForeColor、BackColor、BorderBrush -> SearchItem.cs

        public ReserveItem() { }
        public ReserveItem(ReserveData item)
        {
            base.ReserveInfo = item;
        }

        private EpgEventInfo eventInfo = null;
        public override EpgEventInfo EventInfo
        {
            get
            {
                if (eventInfo == null)
                {
                    if (ReserveInfo != null)
                    {
                        eventInfo = CommonManager.Instance.GetEpgEventInfoFromReserveData(ReserveInfo, false);
                    }
                }
                return eventInfo;
            }

            set { eventInfo = value; }
        }

        public override String EventName
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                return ReserveInfo.Title;
            }
        }
        public override String ServiceName
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                return ReserveInfo.StationName;
            }
        }
        public override String NetworkName
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                return CommonManager.Instance.ConvertNetworkNameText(ReserveInfo.OriginalNetworkID);
            }
        }
        public override String StartTime
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                DateTime endTime = ReserveInfo.StartTime + TimeSpan.FromSeconds(ReserveInfo.DurationSecond);
                return ReserveInfo.StartTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ") + endTime.ToString("HH:mm:ss");
            }
        }
        public override TimeSpan ProgramDuration
        {
            get
            {
                if (ReserveInfo == null) { return new TimeSpan(); }
                //
                return TimeSpan.FromSeconds(ReserveInfo.DurationSecond);
            }
        }
        public String MarginStart
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                return mutil.MarginStartText(ReserveInfo.RecSetting);
            }
        }
        public String MarginEnd
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                return mutil.MarginEndText(ReserveInfo.RecSetting);
            }
        }
        public override String ProgramContent
        {
            get
            {
                if (EventInfo == null) return "";
                //
                return EventInfo.ShortInfo.text_char.Replace("\r\n", " ");
            }
        }
        //public String JyanruKey -> SearchItem.cs
        public String RecMode
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                return CommonManager.Instance.ConvertRecModeText(ReserveInfo.RecSetting.RecMode);
            }
        }
        public String Priority
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                return ReserveInfo.RecSetting.Priority.ToString();
            }
        }
        public String Tuijyu
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    if (ReserveInfo.RecSetting.TuijyuuFlag == 0)
                    {
                        view = "しない";
                    }
                    else if (ReserveInfo.RecSetting.TuijyuuFlag == 1)
                    {
                        view = "する";
                    }
                }
                return view;
            }
        }
        public String Pittari
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    if (ReserveInfo.RecSetting.PittariFlag == 0)
                    {
                        view = "しない";
                    }
                    else if (ReserveInfo.RecSetting.PittariFlag == 1)
                    {
                        view = "する";
                    }
                }
                return view;
            }
        }
        public String Comment
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                String view = ReserveInfo.Comment.ToString();
                if (view == "")
                {
                    view = "個別予約(" + (ReserveInfo.EventID == 0xFFFF ? "プログラム" : "EPG") + ")";
                }
                return view;
            }
        }
        public List<String> RecFolder
        {
            get
            {
                List<String> list = new List<string>();
                if (ReserveInfo != null)
                {
                    ReserveInfo.RecSetting.RecFolderList.ForEach(info => list.Add(info.RecFolder));
                    ReserveInfo.RecSetting.PartialRecFolder.ForEach(info => list.Add("(ワンセグ) " + info.RecFolder));
                }
                return list;
            }
        }
        public List<String> RecFileName
        {
            get
            {
                if (ReserveInfo == null) return new List<string>();
                //
                return ReserveInfo.RecFileNameList;
            }
        }
        public override TextBlock ToolTipView
        {
            get
            {
                if (Settings.Instance.NoToolTip == true) return null;

                String view = "";
                if (ReserveInfo != null)
                {
                    view = CommonManager.Instance.ConvertReserveText(ReserveInfo);
                }

                return mutil.GetTooltipBlockStandard(view);
            }
        }
        public override String Status
        {
            get
            {
                String[] wiewString = { "", "無", "予+", "無+", "録*", "無*" };
                int index = 0;
                if (ReserveInfo != null)
                {
                    if (ReserveInfo.IsOnAir() == true)
                    {
                        index = 2;
                    }
                    if (ReserveInfo.IsOnRec() == true)//マージンがあるので、IsOnAir==trueとは限らない
                    {
                        index = 4;
                    }
                    if (ReserveInfo.RecSetting.RecMode == 5) //無効の判定
                    {
                        index += 1;
                    }
                }
                return wiewString[index];
            }
        }
        public override SolidColorBrush StatusColor
        {
            get
            {
                List<ReserveData> test = new List<ReserveData>();

                SolidColorBrush color = CommonManager.Instance.StatResForeColor;
                if (ReserveInfo != null)
                {
                    if (ReserveInfo.IsOnAir() == true)
                    {
                        color = CommonManager.Instance.StatOnAirForeColor;
                    }
                    if (ReserveInfo.IsOnRec() == true)
                    {
                        color = CommonManager.Instance.StatRecForeColor;
                    }
                }
                return color;
            }
        }

    }

    public static class ReserveItemEx
    {
        public static List<ReserveData> ReserveInfoList(this ICollection<ReserveItem> itemlist)
        {
            return itemlist.Where(item => item != null).Select(item => item.ReserveInfo).ToList();
        }
    }
}
