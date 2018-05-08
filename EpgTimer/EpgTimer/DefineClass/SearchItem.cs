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

namespace EpgTimer
{
    public class SearchItem
    {
        public SearchItem(EpgEventInfo info, bool past)
        {
            EventInfo = info;
            Past = past;
        }
        public EpgEventInfo EventInfo
        {
            get;
            private set;
        }
        public ReserveData ReserveInfo
        {
            get;
            set;
        }
        public String EventName
        {
            get { return EventInfo.ShortInfo != null ? EventInfo.ShortInfo.event_name : ""; }
        }
        public bool Past
        {
            get;
            private set;
        }
        public String ServiceName
        {
            get;
            set;
        }
        public String NetworkName
        {
            get { return CommonManager.ConvertNetworkNameText(EventInfo.original_network_id); }
        }
        public String StartTime
        {
            get { return EventInfo.StartTimeFlag != 0 ? EventInfo.start_time.ToString("yyyy/MM/dd(ddd) HH:mm:ss") : "未定"; }
        }
        public bool IsReserved
        {
            get { return ReserveInfo != null; }
        }
        public String Reserved
        {
            get { return IsReserved ? "予" : ""; }
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
                TextBlock block = new TextBlock();
                block.Text = CommonManager.Instance.ConvertProgramText(EventInfo, EventInfoTextMode.All);
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
                return CommonManager.Instance.RecModeList.Length > ReserveInfo.RecSetting.RecMode ?
                       CommonManager.Instance.RecModeList[ReserveInfo.RecSetting.RecMode] : "";
            }
        }

        public String JyanruKey
        {
            get
            {
                //
                String view = "";
                if (EventInfo.ContentInfo != null)
                {
                    Dictionary<int, List<int>> nibbleDict1 = new Dictionary<int, List<int>>();  // 小ジャンルを大ジャンルでまとめる
                    foreach (EpgContentData ecd1 in EventInfo.ContentInfo.nibbleList)
                    {
                        int nibble1 = ecd1.content_nibble_level_1;
                        int nibble2 = ecd1.content_nibble_level_2;
                        if (nibble1 == 0x0E && nibble2 == 0x01)
                        {
                            nibble1 = ecd1.user_nibble_1 | 0x70;
                            nibble2 = ecd1.user_nibble_2;
                        }
                        if (nibbleDict1.ContainsKey(nibble1))
                        {
                            nibbleDict1[nibble1].Add(nibble2);
                        }
                        else
                        {
                            nibbleDict1.Add(nibble1, new List<int>() { nibble2 });
                        }
                    }
                    foreach (KeyValuePair<int, List<int>> kvp1 in nibbleDict1)
                    {
                        int nibble1 = kvp1.Key;
                        UInt16 contentKey1 = (UInt16)(nibble1 << 8 | 0xFF);
                        //
                        string smallCategory1 = "";
                        foreach (int nibble2 in kvp1.Value)
                        {
                            UInt16 contentKey2 = (UInt16)(nibble1 << 8 | nibble2);
                            if (nibble2 != 0xFF)
                            {
                                if (smallCategory1 != "") { smallCategory1 += ", "; }
                                if (CommonManager.Instance.ContentKindDictionary.ContainsKey(contentKey2))
                                {
                                    smallCategory1 += CommonManager.Instance.ContentKindDictionary[contentKey2].ToString().Trim();
                                }
                            }
                        }
                        //
                        if (view != "") { view += ", "; }
                        if (CommonManager.Instance.ContentKindDictionary.ContainsKey(contentKey1))
                        {
                            view += "[" + CommonManager.Instance.ContentKindDictionary[contentKey1].ToString().Trim();
                            if (smallCategory1 != "") { view += " - " + smallCategory1; }
                            view += "]";
                        }
                    }
                }
                return view;
            }
        }

        /// <summary>
        /// 番組放送時間（長さ）
        /// </summary>
        public TimeSpan ProgramDuration
        {
            get
            {
                if (EventInfo.DurationFlag == 0) { return new TimeSpan(); }
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
                if (this.EventInfo.ShortInfo == null) { return null; }
                //
                return this.EventInfo.ShortInfo.text_char.Replace("\r\n", " ");
            }
        }

        /// <summary>
        /// 番組詳細
        /// </summary>
        public string ProgramDetail
        {
            get
            {
                //
                return CommonManager.Instance.ConvertProgramText(this.EventInfo, EventInfoTextMode.All);
            }
        }
        public Brush BorderBrush
        {
            get
            {
                if (EventInfo.ContentInfo != null)
                {
                    foreach (EpgContentData info in EventInfo.ContentInfo.nibbleList)
                    {
                        if ((info.content_nibble_level_1 <= 0x0B || info.content_nibble_level_1 == 0x0F) &&
                            CommonManager.Instance.CustContentColorList.Count > info.content_nibble_level_1)
                        {
                            return CommonManager.Instance.CustContentColorList[info.content_nibble_level_1];
                        }
                    }
                }
                if (CommonManager.Instance.CustContentColorList.Count > 0x10)
                {
                    return CommonManager.Instance.CustContentColorList[0x10];
                }
                return null;
            }
        }
    }
}
