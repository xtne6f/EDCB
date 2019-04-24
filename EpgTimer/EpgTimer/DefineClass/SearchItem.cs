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
        public SearchItem(EpgEventInfo info, bool past, bool filtered)
        {
            EventInfo = info;
            Past = past;
            Filtered = filtered;
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
        public bool Filtered
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
            get { return EventInfo.StartTimeFlag != 0 ? EventInfo.start_time.ToString("yyyy\\/MM\\/dd(ddd) HH\\:mm\\:ss") : "未定"; }
        }
        public bool IsReserved
        {
            get { return ReserveInfo != null; }
        }
        public String Reserved
        {
            get { return ReserveInfo == null ? "" : ReserveInfo.RecSetting.RecMode == 4 ? "視" :  "予"; }
        }
        public SolidColorBrush BackColor
        {
            get
            {
                return ReserveInfo == null ? Settings.BrushCache.ResDefBrush :
                       ReserveInfo.RecSetting.RecMode == 5 ? Settings.BrushCache.ResNoBrush :
                       ReserveInfo.OverlapMode == 2 ? Settings.BrushCache.ResErrBrush :
                       ReserveInfo.OverlapMode == 1 ? Settings.BrushCache.ResWarBrush : Settings.BrushCache.ResDefBrush;
            }
        }
        public double Opacity
        {
            get { return Filtered ? 0.7 : 1.0; }
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
                    // 小ジャンルを大ジャンルでまとめる
                    foreach (EpgContentData ecd1 in EventInfo.ContentInfo.nibbleList)
                    {
                        int nibble1 = ecd1.content_nibble_level_1;
                        int nibble2 = ecd1.content_nibble_level_2;
                        if (nibble1 == 0x0E && nibble2 <= 0x01)
                        {
                            nibble1 = ecd1.user_nibble_1 | (0x60 + nibble2 * 16);
                            nibble2 = ecd1.user_nibble_2;
                        }
                        string name;
                        if (CommonManager.Instance.ContentKindDictionary.TryGetValue((ushort)(nibble1 << 8 | 0xFF), out name) == false)
                        {
                            name = "(0x" + nibble1.ToString("X2") + ")";
                        }
                        string key = "[" + name + " - ";
                        int i = view.IndexOf(key, StringComparison.Ordinal);
                        if (i < 0)
                        {
                            key = "[" + name + "]";
                            i = view.IndexOf(key, StringComparison.Ordinal);
                            if (i < 0)
                            {
                                view += (view.Length > 0 ? ", " : "") + key;
                                i = view.Length - 1;
                            }
                        }
                        // "その他"でなければ
                        if (nibble1 != 0x0F)
                        {
                            if (CommonManager.Instance.ContentKindDictionary.TryGetValue((ushort)(nibble1 << 8 | nibble2), out name) == false)
                            {
                                name = "(0x" + nibble2.ToString("X2") + ")";
                            }
                            view = view.Insert(view.IndexOf(']', i), (key[key.Length - 1] == ']' ? " - " : ", ") + name);
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
                        if (info.content_nibble_level_1 <= 0x0B || info.content_nibble_level_1 == 0x0F)
                        {
                            return Settings.BrushCache.ContentBrushList[info.content_nibble_level_1];
                        }
                    }
                }
                return Settings.BrushCache.ContentBrushList[0x10];
            }
        }
    }
}
