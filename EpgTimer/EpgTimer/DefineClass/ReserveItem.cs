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

namespace EpgTimer
{
    public class ReserveItem
    {
        public ReserveItem(ReserveData item)
        {
            this.ReserveInfo = item;
        }
        public ReserveData ReserveInfo
        {
            get;
            private set;
        }
        public String EventName
        {
            get { return ReserveInfo.Title; }
        }
        public String ServiceName
        {
            get { return ReserveInfo.StationName; }
        }
        public String NetworkName
        {
            get { return CommonManager.ConvertNetworkNameText(ReserveInfo.OriginalNetworkID); }
        }
        public String StartTime
        {
            get { return CommonManager.GetTimeDurationText(true, ReserveInfo.StartTime, true, ReserveInfo.DurationSecond); }
        }
        public String RecMode
        {
            get
            {
                return CommonManager.Instance.RecModeList.Length > ReserveInfo.RecSetting.RecMode ?
                       CommonManager.Instance.RecModeList[ReserveInfo.RecSetting.RecMode] : "";
            }
        }
        public byte Priority
        {
            get { return ReserveInfo.RecSetting.Priority; }
        }
        public String Tuijyu
        {
            get { return ReserveInfo.RecSetting.TuijyuuFlag == 1 ? "する" : "しない"; }
        }
        public String Pittari
        {
            get { return ReserveInfo.RecSetting.PittariFlag == 1 ? "する" : "しない"; }
        }
        public String Comment
        {
            get { return ReserveInfo.Comment; }
        }
        public List<String> RecFileName
        {
            get { return ReserveInfo.RecFileNameList; }
        }
        public String IsProgram
        {
            get { return ReserveInfo.EventID == 0xFFFF ? "はい" : "いいえ"; }
        }
        public String TunerID
        {
            get { return ReserveInfo.RecSetting.TunerID == 0 ? "自動" : "ID:" + ReserveInfo.RecSetting.TunerID.ToString("X8"); }
        }
        public String BatFilePath
        {
            get { return ReserveInfo.RecSetting.BatFilePath; }
        }
        public uint ID
        {
            get { return ReserveInfo.ReserveID; }
        }
        public SolidColorBrush BackColor
        {
            get
            {
                return ReserveInfo.RecSetting.RecMode == 5 ? CommonManager.Instance.ResNoBackColor :
                       ReserveInfo.OverlapMode == 2 ? CommonManager.Instance.ResErrBackColor :
                       ReserveInfo.OverlapMode == 1 ? CommonManager.Instance.ResWarBackColor : CommonManager.Instance.ResDefBackColor;
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
                block.Text = CommonManager.Instance.ConvertReserveText(ReserveInfo);
                block.MaxWidth = 400;
                block.TextWrapping = TextWrapping.Wrap;
                return block;
            }
        }

        public Brush BorderBrush
        {
            get
            {
                EpgEventInfo eventInfo = null;
                if (ReserveInfo.EventID != 0xFFFF)
                {
                    eventInfo = CommonManager.Instance.DB.GetPgInfo(ReserveInfo.OriginalNetworkID, ReserveInfo.TransportStreamID,
                                                                    ReserveInfo.ServiceID, ReserveInfo.EventID, true);
                }
                if (eventInfo != null)
                {
                    if (eventInfo.ContentInfo != null)
                    {
                        foreach (EpgContentData info in eventInfo.ContentInfo.nibbleList)
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
                }
                return null;
            }
        }
    }
}
