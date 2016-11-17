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
    public class RecInfoItem : DataListItemBase
    {
        public RecInfoItem() { }
        public RecInfoItem(RecFileInfo item)
        {
            this.RecInfo = item;
        }

        public RecFileInfo RecInfo { get; set; }
        public override ulong KeyID { get { return RecInfo == null ? 0 : RecInfo.ID; } }

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
        public String EventNameValue
        {
            get
            {
                return Settings.Instance.TrimSortTitle == true ? MenuUtil.TrimKeyword(EventName) : EventName;
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
        public override Brush BackColor
        {
            get
            {
                //番組表へジャンプ時の強調表示
                if (NowJumpingTable != 0) return base.BackColor;

                //通常表示
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
        public override string ConvertInfoText()
        {
            if (RecInfo == null) return "";
            //
            String view = CommonManager.ConvertTimeText(RecInfo.StartTime, RecInfo.DurationSecond, false, false, false) + "\r\n";
            view += ServiceName + "(" + NetworkName + ")" + "\r\n";
            view += EventName + "\r\n\r\n";

            view += "録画結果 : " + Result + "\r\n";
            view += "録画ファイルパス : " + RecFilePath + "\r\n";
            view += ConvertDropText() + "\r\n";
            view += ConvertScrambleText()+ "\r\n\r\n";

            view += CommonManager.Convert64PGKeyString(RecInfo.Create64PgKey());

            return view;
        }
        public string DropInfoText
        {
            get
            {
                if (RecInfo == null) return "";
                //
                return ConvertDropText("D:") + " " + ConvertScrambleText("S:");
            }
        }
        private string ConvertDropText(string title = "Drops : ")
        {
            if (Settings.Instance.RecinfoErrCriticalDrops == true)
            {
                return "*" + title + RecInfo.DropsCritical.ToString();
            }
            else
            {
                return title + RecInfo.Drops.ToString();
            }
        }
        private string ConvertScrambleText(string title = "Scrambles : ")
        {
            if (Settings.Instance.RecinfoErrCriticalDrops == true)
            {
                return "*" + title + RecInfo.ScramblesCritical.ToString();
            }
            else
            {
                return title + RecInfo.Scrambles.ToString();
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
