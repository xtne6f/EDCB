﻿using System;
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

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public class RecInfoItem
    {
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        
        public RecInfoItem(RecFileInfo item)
        {
            this.RecInfo = item;
        }
        public RecFileInfo RecInfo
        {
            get;
            set;
        }
        public bool IsProtect
        {
            set
            {
                if (RecInfo != null)
                {
                    if (this.RecInfo.ProtectFlag != value)
                    {
                        mutil.RecinfoChgProtect(mutil.ToList(this.RecInfo));
                    }
                }
            }
            get
            {
                bool chk = false;
                if (RecInfo != null)
                {
                    chk = RecInfo.ProtectFlag;
                }
                return chk;
            }
        }
        public bool IsProtect_InfoView
        {
            set
            {
                //1回の通信で実施する。
                MainWindow mainWindow = (MainWindow)Application.Current.MainWindow;
                if (mainWindow.recInfoView.ChgProtectRecInfoForMark(this) == true) return;
                IsProtect = value;
            }
            get
            {
                return IsProtect;
            }
        }
        public String EventName
        {
            get
            {
                String view = "";
                if (RecInfo != null)
                {
                    view = RecInfo.Title;
                }
                return view;
            }
        }
        public String ServiceName
        {
            get
            {
                String view = "";
                if (RecInfo != null)
                {
                    view = RecInfo.ServiceName;
                }
                return view;
            }
        }
        public TimeSpan ProgramDuration
        {
            get
            {
                TimeSpan view = new TimeSpan();
                if (RecInfo != null)
                {
                    view = TimeSpan.FromSeconds(RecInfo.DurationSecond);
                }
                return view;
            }
        }
        public String StartTime
        {
            get
            {
                String view = "";
                if (RecInfo != null)
                {
                    view = RecInfo.StartTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
                    DateTime endTime = RecInfo.StartTime + TimeSpan.FromSeconds(RecInfo.DurationSecond);
                    view += endTime.ToString("HH:mm:ss");
                }
                return view;
            }
        }
        public String Drops
        {
            get
            {
                String view = "";
                if (RecInfo != null)
                {
                    view = RecInfo.Drops.ToString();
                }
                return view;
            }
        }
        public String Scrambles
        {
            get
            {
                String view = "";
                if (RecInfo != null)
                {
                    view = RecInfo.Scrambles.ToString();
                }
                return view;
            }
        }
        public String Result
        {
            get
            {
                String view = "";
                if (RecInfo != null)
                {
                    view = RecInfo.Comment;
                }
                return view;
            }
        }
        public String NetworkName
        {
            get
            {
                String view = "";
                if (RecInfo != null)
                {
                    view = CommonManager.Instance.ConvertNetworkNameText(RecInfo.OriginalNetworkID);
                }
                return view;
            }
        }
        public String RecFilePath
        {
            get
            {
                String view = "";
                if (RecInfo != null)
                {
                    view = RecInfo.RecFilePath;
                }
                return view;
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
                SolidColorBrush color = CommonManager.Instance.RecEndDefBackColor;
                if (RecInfo != null)
                {
                    if (RecInfo.Scrambles > 0)
                    {
                        color = CommonManager.Instance.RecEndWarBackColor;
                    }
                    if (RecInfo.Drops > 0)
                    {
                        color = CommonManager.Instance.RecEndErrBackColor;
                    }
                }
                return color;
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
                    view = RecInfo.StartTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
                    DateTime endTime = RecInfo.StartTime + TimeSpan.FromSeconds(RecInfo.DurationSecond);
                    view += endTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss") + "\r\n";

                    view += ServiceName;
                    if (0x7880 <= RecInfo.OriginalNetworkID && RecInfo.OriginalNetworkID <= 0x7FE8)
                    {
                        view += " (地デジ)";
                    }
                    else if (RecInfo.OriginalNetworkID == 0x0004)
                    {
                        view += " (BS)";
                    }
                    else if (RecInfo.OriginalNetworkID == 0x0006)
                    {
                        view += " (CS1)";
                    }
                    else if (RecInfo.OriginalNetworkID == 0x0007)
                    {
                        view += " (CS2)";
                    }
                    else
                    {
                        view += " (その他)";
                    }
                    view += "\r\n";
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
