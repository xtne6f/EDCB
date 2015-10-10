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
                    if (0x7880 <= eventInfo.original_network_id && eventInfo.original_network_id <= 0x7FE8)
                    {
                        view = "地デジ";
                    }
                    else if (eventInfo.original_network_id == 0x0004)
                    {
                        view = "BS";
                    }
                    else if (eventInfo.original_network_id == 0x0006)
                    {
                        view = "CS1";
                    }
                    else if (eventInfo.original_network_id == 0x0007)
                    {
                        view = "CS2";
                    }
                    else
                    {
                        view = "その他";
                    }

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
        public String Reserved
        {
            get
            {
                String view = "";
                if (IsReserved == true)
                {
                    view = "予";
                }
                return view;
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
                // ReserveItemクラスからコピペ
                String view = "";
                switch (ReserveInfo.RecSetting.RecMode)
                {
                    case 0:
                        view = "全サービス";
                        break;
                    case 1:
                        view = "指定サービス";
                        break;
                    case 2:
                        view = "全サービス（デコード処理なし）";
                        break;
                    case 3:
                        view = "指定サービス（デコード処理なし）";
                        break;
                    case 4:
                        view = "視聴";
                        break;
                    case 5:
                        view = "無効";
                        break;
                    default:
                        break;
                }
                return view;
            }
        }

        public String JyanruKey
        {
            get
            {
                if (this.EventInfo == null) { return null; }
                //
                String view = "";
                if (eventInfo != null && eventInfo.ContentInfo != null)
                {
                    Dictionary<int, List<int>> nibbleDict1 = new Dictionary<int, List<int>>();  // 小ジャンルを大ジャンルでまとめる
                    foreach (EpgContentData ecd1 in eventInfo.ContentInfo.nibbleList)
                    {
                        if (nibbleDict1.ContainsKey(ecd1.content_nibble_level_1))
                        {
                            nibbleDict1[ecd1.content_nibble_level_1].Add(ecd1.content_nibble_level_2);
                        }
                        else
                        {
                            nibbleDict1.Add(ecd1.content_nibble_level_1, new List<int>() { ecd1.content_nibble_level_2 });
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
                if (this.EventInfo == null || this.EventInfo.DurationFlag == 0) { return new TimeSpan(); }
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
                if (this.EventInfo == null) { return null; }
                //
                return CommonManager.Instance.ConvertProgramText(this.EventInfo, EventInfoTextMode.All);
            }
        }
    }
}
