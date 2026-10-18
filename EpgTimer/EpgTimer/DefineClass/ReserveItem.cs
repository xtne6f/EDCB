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
    class ReserveItem
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
        public string EventName
        {
            get { return ReserveInfo.Title; }
        }
        public string ServiceName
        {
            get { return ReserveInfo.StationName; }
        }
        public string NetworkName
        {
            get { return CommonManager.ConvertNetworkNameText(ReserveInfo.OriginalNetworkID); }
        }
        public CommonManager.TimeDuration StartTime
        {
            get { return new CommonManager.TimeDuration(true, ReserveInfo.StartTime, true, ReserveInfo.DurationSecond); }
        }
        public CommonManager.TimeDuration StartTimeNoDuration
        {
            get { return new CommonManager.TimeDuration(true, ReserveInfo.StartTime, true, double.NaN); }
        }
        public TimeSpan Duration
        {
            get { return TimeSpan.FromSeconds(ReserveInfo.DurationSecond); }
        }
        public string RecEnabled
        {
            get { return ReserveInfo.RecSetting.IsNoRec() ? "いいえ" : "はい"; }
        }
        public string RecMode
        {
            get { return CommonManager.Instance.RecModeList[ReserveInfo.RecSetting.GetRecMode()]; }
        }
        public byte Priority
        {
            get { return ReserveInfo.RecSetting.Priority; }
        }
        public string Tuijyu
        {
            get { return ReserveInfo.RecSetting.TuijyuuFlag == 1 ? "する" : "しない"; }
        }
        public string Pittari
        {
            get { return ReserveInfo.RecSetting.PittariFlag == 1 ? "する" : "しない"; }
        }
        public string Comment
        {
            get { return ReserveInfo.Comment; }
        }
        public string RecFileName
        {
            get { return ReserveInfo.RecFileNameList.FirstOrDefault() ?? ""; }
        }
        public List<string> RecFileNameList
        {
            get { return ReserveInfo.RecFileNameList; }
        }
        public string IsProgram
        {
            get { return ReserveInfo.EventID == 0xFFFF ? "はい" : "いいえ"; }
        }
        public string TunerID
        {
            get { return ReserveInfo.RecSetting.TunerID == 0 ? "自動" : "ID:" + ReserveInfo.RecSetting.TunerID.ToString("X8"); }
        }
        public string BatFilePath
        {
            get
            {
                int i = ReserveInfo.RecSetting.BatFilePath.IndexOf('*');
                return i < 0 ? ReserveInfo.RecSetting.BatFilePath : ReserveInfo.RecSetting.BatFilePath.Remove(i);
            }
        }
        public string BatFileTag
        {
            get
            {
                int i = ReserveInfo.RecSetting.BatFilePath.IndexOf('*');
                return i < 0 ? "" : ReserveInfo.RecSetting.BatFilePath.Substring(i + 1);
            }
        }
        public uint ID
        {
            get { return ReserveInfo.ReserveID; }
        }

        private string _estimatedRecSize;
        public string EstimatedRecSize
        {
            get
            {
                if (_estimatedRecSize == null)
                {
                    _estimatedRecSize = "";
                    if (ReserveInfo.RecSetting.GetRecMode() != 4)
                    {
                        int bitrate = 0;
                        for (int i = 0; bitrate <= 0; i++)
                        {
                            string key = CommonManager.Create64Key((ushort)(i > 2 ? 0xFFFF : ReserveInfo.OriginalNetworkID),
                                                                   (ushort)(i > 1 ? 0xFFFF : ReserveInfo.TransportStreamID),
                                                                   (ushort)(i > 0 ? 0xFFFF : ReserveInfo.ServiceID)).ToString("X12");
                            // NWModeではファイルが配置されないかもしれないが特別扱いはしない
                            bitrate = IniFileHandler.GetPrivateProfileInt("BITRATE", key, 0, SettingPath.BitrateIniPath);
                            bitrate = bitrate <= 0 && i == 3 ? 19456 : bitrate;
                        }
                        long margin = ReserveInfo.RecSetting.UseMargineFlag != 0 ? ReserveInfo.RecSetting.StartMargine + ReserveInfo.RecSetting.EndMargine :
                                      CommonManager.Instance.DB.DefaultRecSetting != null ?
                                          CommonManager.Instance.DB.DefaultRecSetting.StartMargine + CommonManager.Instance.DB.DefaultRecSetting.EndMargine : 0;
                        _estimatedRecSize = ((double)Math.Max(bitrate / 8 * 1000 * (margin + ReserveInfo.DurationSecond), 0) / 1024 / 1024 / 1024).ToString("0.0GB").PadLeft(6);
                    }
                }
                return _estimatedRecSize;
            }
        }

        public SolidColorBrush BackColor
        {
            get { return Settings.Instance.ResColorPosition == 0 ? ResBackColor : null; }
        }
        public SolidColorBrush AlternationBackColor
        {
            get { return (Settings.Instance.ResColorPosition == 0 ? ResBackColor : null) ?? Settings.BrushCache.ResDefBrush; }
        }
        public SolidColorBrush StartTimeBackColor
        {
            get { return Settings.Instance.ResColorPosition == 1 ? ResBackColor : null; }
        }
        public SolidColorBrush EventNameBackColor
        {
            get { return Settings.Instance.ResColorPosition == 2 ? ResBackColor : null; }
        }
        private SolidColorBrush ResBackColor
        {
            get
            {
                return ReserveInfo.RecSetting.IsNoRec() ? Settings.BrushCache.ResNoBrush :
                       ReserveInfo.OverlapMode == 2 ? Settings.BrushCache.ResErrBrush :
                       ReserveInfo.OverlapMode == 1 ? Settings.BrushCache.ResWarBrush : null;
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
                            if (info.content_nibble_level_1 <= 0x0B || info.content_nibble_level_1 == 0x0F)
                            {
                                return Settings.BrushCache.ContentBrushList[info.content_nibble_level_1];
                            }
                        }
                    }
                    return Settings.BrushCache.ContentBrushList[0x10];
                }
                return null;
            }
        }
    }
}
