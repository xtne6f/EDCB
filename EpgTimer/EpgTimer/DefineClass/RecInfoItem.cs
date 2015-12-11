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
        private MenuUtil mutil = CommonManager.Instance.MUtil;

        public RecFileInfo RecInfo { get; set; }
        public RecInfoItem(RecFileInfo item)
        {
            this.RecInfo = item;
        }

        public static string GetValuePropertyName(string key)
        {
            switch (key)
            {
                case "StartTime": return "StartTimeValue";
                case "ProgramDuration": return "ProgramDurationValue";
                default: return key;
            }
        }

        public bool IsProtect
        {
            set
            {
                //選択されている場合、複数選択時に1回の通信で処理するため、ウインドウ側に処理を渡す。
                MainWindow mainWindow = (MainWindow)Application.Current.MainWindow;
                mainWindow.recInfoView.ChgProtectRecInfoFromCheckbox(this);
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
        public DateTime StartTimeValue
        {
            get
            {
                if (RecInfo == null) return new DateTime();
                //
                return RecInfo.StartTime;
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
        public SolidColorBrush ForeColor
        {
            get
            {
                return CommonManager.Instance.ListDefForeColor;
            }
        }
        public SolidColorBrush BackColor
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
                return mutil.GetTooltipBlockStandard(RecInfoText);
            }
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
