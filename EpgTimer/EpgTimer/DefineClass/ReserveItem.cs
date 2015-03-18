using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public class ReserveItem
    {
        private EpgEventInfo eventInfo = null;

        public ReserveItem(ReserveData item)
        {
            this.ReserveInfo = item;
        }
        public ReserveData ReserveInfo
        {
            get;
            set;
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
        public String JyanruKey
        {
            get
            {
                String view = "";
                if (this.EventInfo != null)
                {
                    view = CommonManager.Instance.ConvertJyanruText(this.EventInfo);
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
        public SolidColorBrush ForeColor
        {
            get
            {
                SolidColorBrush color = CommonManager.Instance.ListDefForeColor;
                if (ReserveInfo != null)
                {
                    color = CommonManager.Instance.EventItemForeColor(ReserveInfo.RecSetting.RecMode);
                }
                return color;
            }
        }
        public SolidColorBrush BackColor
        {
            get
            {
                SolidColorBrush color = CommonManager.Instance.ResDefBackColor;
                if (ReserveInfo != null)
                {
                    if (ReserveInfo.RecSetting.RecMode == 5)
                    {
                        color = CommonManager.Instance.ResNoBackColor;
                    }
                    else if (ReserveInfo.OverlapMode == 2)
                    {
                        color = CommonManager.Instance.ResErrBackColor;
                    }
                    else if (ReserveInfo.OverlapMode == 1)
                    {
                        color = CommonManager.Instance.ResWarBackColor;
                    }
                }
                return color;
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

        public EpgEventInfo EventInfo
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
        }

        public Brush BorderBrush
        {
            get
            {
                Brush color1 = Brushes.White;
                if (this.EventInfo != null)
                {
                    if (this.EventInfo.ContentInfo != null)
                    {
                        if (this.EventInfo.ContentInfo.nibbleList.Count > 0)
                        {
                            try
                            {
                                foreach (EpgContentData info1 in this.EventInfo.ContentInfo.nibbleList)
                                {
                                    if (info1.content_nibble_level_1 <= 0x0B || info1.content_nibble_level_1 == 0x0F && Settings.Instance.ContentColorList.Count > info1.content_nibble_level_1)
                                    {
                                        color1 = CommonManager.Instance.CustContentColorList[info1.content_nibble_level_1];
                                        break;
                                    }
                                }
                            }
                            catch
                            {
                            }
                        }
                        else
                        {
                            color1 = CommonManager.Instance.CustContentColorList[0x10];
                        }
                    }
                    else
                    {
                        color1 = CommonManager.Instance.CustContentColorList[0x10];
                    }
                }

                return color1;
            }
        }

        /// <summary>
        /// 番組詳細
        /// </summary>
        public string ProgramDetail
        {
            get
            {
                string text1 = "Unavailable (;_;)";
                if (this.EventInfo != null)
                {
                    text1 = CommonManager.Instance.ConvertProgramText(this.EventInfo, EventInfoTextMode.All);
                }
                return text1;
            }
        }
    }
}
