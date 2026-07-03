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
    class RecInfoItem
    {
        public RecInfoItem(RecFileInfo item)
        {
            this.RecInfo = item;
        }
        public RecFileInfo RecInfo
        {
            get;
            private set;
        }
        public bool IsProtect
        {
            set
            {
                RecInfo.ProtectFlag = Convert.ToByte(value);
                CommonManager.CreateSrvCtrl().SendChgProtectRecInfo(new List<RecFileInfo>() { RecInfo });
            }
            get { return RecInfo.ProtectFlag != 0; }
        }
        public string EventName
        {
            get { return RecInfo.Title; }
        }
        public string ServiceName
        {
            get { return RecInfo.ServiceName; }
        }
        public CommonManager.TimeDuration StartTime
        {
            get { return new CommonManager.TimeDuration(true, RecInfo.StartTime, true, RecInfo.DurationSecond); }
        }
        public CommonManager.TimeDuration StartTimeNoDuration
        {
            get { return new CommonManager.TimeDuration(true, RecInfo.StartTime, true, double.NaN); }
        }
        public TimeSpan Duration
        {
            get { return TimeSpan.FromSeconds(RecInfo.DurationSecond); }
        }
        public long Drops
        {
            get { return RecInfo.Drops; }
        }
        public long Scrambles
        {
            get { return RecInfo.Scrambles; }
        }
        public string Result
        {
            get { return RecInfo.Comment; }
        }
        public string NetworkName
        {
            get { return CommonManager.ConvertNetworkNameText(RecInfo.OriginalNetworkID); }
        }
        public string RecFilePath
        {
            get { return RecInfo.RecFilePath; }
        }
        public uint ID
        {
            get { return RecInfo.ID; }
        }
        public SolidColorBrush BackColor
        {
            get { return Settings.Instance.RecEndColorPosition == 0 ? DropScrambleBackColor : null; }
        }
        public SolidColorBrush AlternationBackColor
        {
            get { return (Settings.Instance.RecEndColorPosition == 0 ? DropScrambleBackColor : null) ?? Settings.BrushCache.RecEndDefBrush; }
        }
        public SolidColorBrush StartTimeBackColor
        {
            get { return Settings.Instance.RecEndColorPosition == 1 ? DropScrambleBackColor : null; }
        }
        public SolidColorBrush EventNameBackColor
        {
            get { return Settings.Instance.RecEndColorPosition == 2 ? DropScrambleBackColor : null; }
        }
        private SolidColorBrush DropScrambleBackColor
        {
            get
            {
                return RecInfo.Drops > 0 ? Settings.BrushCache.RecEndErrBrush :
                       RecInfo.Scrambles > 0 ? Settings.BrushCache.RecEndWarBrush : null;
            }
        }
        public SolidColorBrush ResultBackColor
        {
            get
            {
                return RecInfo.RecStatus == (uint)RecEndStatus.NORMAL ||
                       RecInfo.RecStatus == (uint)RecEndStatus.CHG_TIME ||
                       RecInfo.RecStatus == (uint)RecEndStatus.NEXT_START_END ? null :
                       RecInfo.RecStatus == (uint)RecEndStatus.ERR_END ||
                       RecInfo.RecStatus == (uint)RecEndStatus.END_SUBREC ||
                       RecInfo.RecStatus == (uint)RecEndStatus.NOT_START_HEAD ? Settings.BrushCache.RecEndWarBrush : Settings.BrushCache.RecEndErrBrush;
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
                string view = "";
                {
                    view = StartTime + "\r\n";

                    view += ServiceName;
                    view += " (" + NetworkName + ")" + "\r\n";
                    view += EventName + "\r\n";
                    view += "\r\n";
                    view += "結果 : " + RecInfo.Comment + "\r\n";
                    view += "録画ファイル : " + RecInfo.RecFilePath + "\r\n";
                    view += "\r\n";

                    view += "OriginalNetworkID : " + RecInfo.OriginalNetworkID.ToString() + " (0x" + RecInfo.OriginalNetworkID.ToString("X4") + ")\r\n";
                    view += "TransportStreamID : " + RecInfo.TransportStreamID.ToString() + " (0x" + RecInfo.TransportStreamID.ToString("X4") + ")\r\n";
                    view += "ServiceID : " + RecInfo.ServiceID.ToString() + " (0x" + RecInfo.ServiceID.ToString("X4") + ")\r\n";
                    view += "EventID : " + RecInfo.EventID.ToString() + " (0x" + RecInfo.EventID.ToString("X4") + ")\r\n";
                    view += "\r\n";
                    view += "Drop : " + Drops + "\r\n";
                    view += "Scramble : " + Scrambles;
                }


                TextBlock block = new TextBlock();
                block.Text = view;
                block.MaxWidth = 400;
                block.TextWrapping = TextWrapping.Wrap;
                return block;
            }
        }
    }
}
