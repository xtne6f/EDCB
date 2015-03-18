using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Controls;
using System.Windows;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public class SearchItem
    {
        private EpgEventInfo eventInfo = null;
        private ReserveData reserveData = null;

        public EpgEventInfo EventInfo
        {
            get { return eventInfo; }
            set { eventInfo = value; }
        }
        public ReserveData ReserveInfo
        {
            get { return reserveData; }
            set { reserveData = value; }
        }
        public String EventName
        {
            get
            {
                String view = "";
                if (eventInfo != null)
                {
                    if (eventInfo.ShortInfo != null)
                    {
                        view = eventInfo.ShortInfo.event_name;
                    }
                }
                return view;
            }
        }
        public String ServiceName
        {
            get;
            set;
        }
        public String NetworkName
        {
            get
            {
                String view = "";
                if (eventInfo != null)
                {
                    view = CommonManager.Instance.ConvertNetworkNameText(eventInfo.original_network_id);
                }
                return view;
            }
        }
        public String StartTime
        {
            get
            {
                String view = "未定";
                if (eventInfo != null)
                {
                    view = eventInfo.start_time.ToString("yyyy/MM/dd(ddd) HH:mm:ss");
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
                if (eventInfo != null)
                {
                    if (eventInfo.IsOnAir() == true)
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
                if (eventInfo != null)
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
                if (reserveData == null)
                {
                    return false;
                }
                else
                {
                    return true;
                }
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
                SolidColorBrush color = Brushes.White;
                if (ReserveInfo != null)
                {
                    if (ReserveInfo.RecSetting.RecMode == 5)
                    {
                        color = Brushes.DarkGray;
                    }
                    else if (ReserveInfo.OverlapMode == 2)
                    {
                        color = Brushes.Red;
                    }
                    else if (ReserveInfo.OverlapMode == 1)
                    {
                        color = Brushes.Yellow;
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
                if (eventInfo != null)
                {
                    view = CommonManager.Instance.ConvertProgramText(eventInfo, EventInfoTextMode.All);
                }

                TextBlock block = new TextBlock();
                block.Text = view;
                block.MaxWidth = 400;
                block.TextWrapping = TextWrapping.Wrap;
                return block;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public String RecMode
        {
            get
            {
                if (this.ReserveInfo == null) { return null; }
                //
                return CommonManager.Instance.ConvertRecModeText(ReserveInfo.RecSetting.RecMode);
            }
        }

        public String JyanruKey
        {
            get
            {
                if (this.EventInfo == null) { return null; }
                //
                return CommonManager.Instance.ConvertJyanruText(EventInfo);
            }
        }

        /// <summary>
        /// 番組放送時間（長さ）
        /// </summary>
        public TimeSpan ProgramDuration
        {
            get
            {
                if (this.EventInfo == null) { return new TimeSpan(); }
                //
                return TimeSpan.FromSeconds(this.EventInfo.durationSec);
            }
        }

        /// <summary>
        /// 番組内容
        /// </summary>
        public String ProgramContent
        {
            get
            {
                if (this.EventInfo == null) { return null; }
                //
                if (Settings.Instance.FixSearchResult)
                {
                    return "省略";
                }
                else
                {
                    return this.EventInfo.ShortInfo.text_char.Replace("\r\n", " ");
                }
            }
        }

        /// <summary>
        /// 番組詳細
        /// </summary>
        public string ProgramDetail
        {
            get
            {
                if (this.EventInfo == null) { return null; }
                //
                return CommonManager.Instance.ConvertProgramText(this.EventInfo, EventInfoTextMode.All);
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
    }
}
