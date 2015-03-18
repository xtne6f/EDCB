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
    public class ReserveItem : ProgramItemCommon
    {
        //ReserveInfo、JyanruKey、ForeColor、BackColor、BorderBrushはベースクラス

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

        public String EventName
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    view = ReserveInfo.Title;
                }
                return view;
            }
        }
        public String ServiceName
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    view = ReserveInfo.StationName;
                }
                return view;
            }
        }
        public String NetworkName
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    view = CommonManager.Instance.ConvertNetworkNameText(ReserveInfo.OriginalNetworkID);
                }
                return view;
            }
        }
        public TimeSpan ProgramDuration
        {
            get
            {
                TimeSpan view = new TimeSpan();
                if (ReserveInfo != null)
                {
                    view = TimeSpan.FromSeconds(ReserveInfo.DurationSecond);
                }
                return view;
            }
        }
        public String ProgramContent
        {
            get
            {
                String view = "";
                if (EventInfo != null)
                {
                    view = EventInfo.ShortInfo.text_char.Replace("\r\n", " ");
                }
                return view;
            }
        }
        public String StartTime
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    view = ReserveInfo.StartTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
                    DateTime endTime = ReserveInfo.StartTime + TimeSpan.FromSeconds(ReserveInfo.DurationSecond);
                    view += endTime.ToString("HH:mm:ss");
                }
                return view;
            }
        }
        public String MarginStart
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    view = CommonManager.Instance.MUtil.MarginStartText(ReserveInfo.RecSetting);
                }
                return view;
            }
        }
        public String MarginEnd
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    view = CommonManager.Instance.MUtil.MarginEndText(ReserveInfo.RecSetting);
                }
                return view;
            }
        }
        public String RecMode
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    view = CommonManager.Instance.ConvertRecModeText(ReserveInfo.RecSetting.RecMode);
                }
                return view;
            }
        }
        public String Priority
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    view = ReserveInfo.RecSetting.Priority.ToString();
                }
                return view;
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
                String view = "";
                if (ReserveInfo != null)
                {
                    view = ReserveInfo.Comment.ToString();
                }
                return view;
            }
        }
        public String Status
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
        public SolidColorBrush StatusColor
        {
            get
            {
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
        public List<String> RecFolder
        {
            get
            {
                List<String> list = new List<string>();
                if (ReserveInfo != null)
                {
                    foreach (RecFileSetInfo recinfo1 in ReserveInfo.RecSetting.RecFolderList)
                    {
                        list.Add(recinfo1.RecFolder);
                    }
                    foreach (RecFileSetInfo recinfo1 in ReserveInfo.RecSetting.PartialRecFolder)
                    {
                        list.Add("(ワンセグ) " + recinfo1.RecFolder);
                    }
                }
                return list;
            }
        }
        public List<String> RecFileName
        {
            get
            {
                List<String> list = new List<string>();
                if (ReserveInfo != null)
                {
                    list = ReserveInfo.RecFileNameList;
                }
                return list;
            }
        }
        public TextBlock ToolTipView
        {
            get
            {
                if (Settings.Instance.NoToolTip == true)
                {
                    return null;
                }
                String view = "";
                if (ReserveInfo != null)
                {
                    view = CommonManager.Instance.ConvertReserveText(ReserveInfo);
                }

                TextBlock block = new TextBlock();
                block.Text = view;
                block.MaxWidth = 400;
                block.TextWrapping = TextWrapping.Wrap;
                return block;
            }
        }

    }

    public static class ReserveItemEx
    {
        public static List<ReserveData> ReserveInfoList(this ICollection<ReserveItem> itemlist)
        {
            try
            {
                return itemlist.Select(item => item.ReserveInfo).ToList();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return new List<ReserveData>();
            }
        }
    }
}
