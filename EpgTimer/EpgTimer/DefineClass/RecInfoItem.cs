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
using System.Windows.Shapes;

namespace EpgTimer
{
    public class RecInfoItem
    {
        public RecInfoItem() { }
        public RecInfoItem(RecFileInfo item)
        {
            this.RecInfo = item;
        }

        public RecFileInfo RecInfo { get; set; }

        public static string GetValuePropertyName(string key)
        {
            var obj = new RecInfoItem();
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

        public bool IsProtect
        {
            set
            {
                EpgCmds.ChgOnOffCheck.Execute(this, null);
            }
            get
            {
                if (RecInfo == null) return false;
                //
                return  RecInfo.ProtectFlag != 0;
            }
        }
        public String EventName
        {
            get
            {
                if (RecInfo == null) return "";
                //
                return RecInfo.Title;
            }
        }
        public String ServiceName
        {
            get
            {
                if (RecInfo == null) return "";
                //
                return RecInfo.ServiceName;
            }
        }
        public String ProgramDuration
        {
            get
            {
                if (RecInfo == null) return "";
                //
                return CommonManager.ConvertDurationText(RecInfo.DurationSecond, Settings.Instance.RecInfoNoDurSecond);
            }
        }
        public UInt32 ProgramDurationValue
        {
            get
            {
                if (RecInfo == null) return UInt32.MinValue;
                //
                return RecInfo.DurationSecond;
            }
        }
        public String StartTime
        {
            get
            {
                if (RecInfo == null) return "";
                //
                return CommonManager.ConvertTimeText(RecInfo.StartTime, RecInfo.DurationSecond, Settings.Instance.RecInfoNoYear, Settings.Instance.RecInfoNoSecond);
            }
        }
        public long StartTimeValue
        {
            get
            {
                if (RecInfo == null) return long.MinValue;
                //
                return RecInfo.StartTime.Ticks;
            }
        }
        public String Drops
        {
            get
            {
                if (RecInfo == null) return "";
                //
                return RecInfo.Drops.ToString();
            }
        }
        public String DropsSerious
        {
            get
            {
                if (RecInfo == null) return "";
                //
                return RecInfo.DropsCritical.ToString();
            }
        }
        public String Scrambles
        {
            get
            {
                if (RecInfo == null) return "";
                //
                return RecInfo.Scrambles.ToString();
            }
        }
        public String ScramblesSerious
        {
            get
            {
                if (RecInfo == null) return "";
                //
                return RecInfo.ScramblesCritical.ToString();
            }
        }
        public String Result
        {
            get
            {
                if (RecInfo == null) return "";
                //
                return RecInfo.Comment;
            }
        }
        public String NetworkName
        {
            get
            {
                if (RecInfo == null) return "";
                //
                return CommonManager.ConvertNetworkNameText(RecInfo.OriginalNetworkID);
            }
        }
        public String RecFilePath
        {
            get
            {
                if (RecInfo == null) return "";
                //
                return RecInfo.RecFilePath;
            }
        }
        public Brush ForeColor
        {
            get
            {
                return CommonManager.Instance.ListDefForeColor;
            }
        }
        public Brush BackColor
        {
            get
            {
                if (RecInfo != null)
                {
                    long drops = Settings.Instance.RecinfoErrCriticalDrops == false ? RecInfo.Drops : RecInfo.DropsCritical;
                    long scrambles = Settings.Instance.RecinfoErrCriticalDrops == false ? RecInfo.Scrambles : RecInfo.ScramblesCritical;

                    if (Settings.Instance.RecInfoDropErrIgnore >= 0 && drops > Settings.Instance.RecInfoDropErrIgnore
                        || RecInfo.RecStatusBasic == RecEndStatusBasic.ERR)
                    {
                        return CommonManager.Instance.RecEndErrBackColor;
                    }
                    if (Settings.Instance.RecInfoDropWrnIgnore >= 0 && drops > Settings.Instance.RecInfoDropWrnIgnore
                        || Settings.Instance.RecInfoScrambleIgnore >= 0 && scrambles > Settings.Instance.RecInfoScrambleIgnore
                        || RecInfo.RecStatusBasic == RecEndStatusBasic.WARN)
                    {
                        return CommonManager.Instance.RecEndWarBackColor;
                    }
                }
                return CommonManager.Instance.RecEndDefBackColor;
            }
        }
        public TextBlock ToolTipView
        {
            get
            {
                if (Settings.Instance.NoToolTip == true) return null;
                //
                return MenuUtil.GetTooltipBlockStandard(RecInfoText);
            }
        }
        public override string ToString()
        {
            return CommonManager.Instance.ConvertTextSearchString(this.EventName);
        }
        public string RecInfoText
        {
            get
            {
                String view = "";
                if (RecInfo != null)
                {
                    view = CommonManager.ConvertTimeText(RecInfo.StartTime, RecInfo.DurationSecond, false, false, false) + "\r\n";

                    view += ServiceName;
                    view += "(" + CommonManager.ConvertNetworkNameText(RecInfo.OriginalNetworkID) + ")" + "\r\n";
                    view += EventName + "\r\n";
                    view += "\r\n";
                    view += "録画結果 : " + RecInfo.Comment + "\r\n";
                    view += "録画ファイルパス : " + RecInfo.RecFilePath + "\r\n";
                    view += "\r\n";

                    view += "OriginalNetworkID : " + RecInfo.OriginalNetworkID.ToString() + " (0x" + RecInfo.OriginalNetworkID.ToString("X4") + ")\r\n";
                    view += "TransportStreamID : " + RecInfo.TransportStreamID.ToString() + " (0x" + RecInfo.TransportStreamID.ToString("X4") + ")\r\n";
                    view += "ServiceID : " + RecInfo.ServiceID.ToString() + " (0x" + RecInfo.ServiceID.ToString("X4") + ")\r\n";
                    view += "EventID : " + RecInfo.EventID.ToString() + " (0x" + RecInfo.EventID.ToString("X4") + ")\r\n";
                    view += "\r\n";
                    view += "Drops : " + RecInfo.Drops.ToString() + "\r\n";
                    view += "Scrambles : " + RecInfo.Scrambles.ToString() + "\r\n";
                }
                return view;
            }
        }
    }

    public static class RecInfoItemEx
    {
        public static List<RecFileInfo> RecInfoList(this ICollection<RecInfoItem> itemlist)
        {
            return itemlist.Where(item => item != null).Select(item => item.RecInfo).ToList();
        }
    }
}
