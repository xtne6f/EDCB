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

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

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
            set;
        }
        public String EventName
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    view = ReserveInfo.Title;
                }
                return view;
            }
        }
        public String ServiceName
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    view = ReserveInfo.StationName;
                }
                return view;
            }
        }
        public String NetworkName
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    if (0x7880 <= ReserveInfo.OriginalNetworkID && ReserveInfo.OriginalNetworkID <= 0x7FE8)
                    {
                        view = "地デジ";
                    }
                    else if (ReserveInfo.OriginalNetworkID == 0x0004)
                    {
                        view = "BS";
                    }
                    else if (ReserveInfo.OriginalNetworkID == 0x0006)
                    {
                        view = "CS1";
                    }
                    else if (ReserveInfo.OriginalNetworkID == 0x0007)
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
                String view = "";
                if (ReserveInfo != null)
                {
                    view = ReserveInfo.StartTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
                    DateTime endTime = ReserveInfo.StartTime + TimeSpan.FromSeconds(ReserveInfo.DurationSecond);
                    view += endTime.ToString("HH:mm:ss");
                }
                return view;
            }
        }
        public String MarginStart
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    int marginTime;
                    if (ReserveInfo.RecSetting.UseMargineFlag == 1)
                    {
                        marginTime = ReserveInfo.RecSetting.StartMargine;
                    }
                    else
                    {
                        //TODO: ここでデフォルトマージンを確認するがEpgTimerNWでは無意味。根本的にはSendCtrlCmdの拡張が必要
                        marginTime = IniFileHandler.GetPrivateProfileInt("SET", "StartMargin", 0, SettingPath.TimerSrvIniPath);
                    }
                    view = CustomTimeFormat(marginTime * -1);
                }
                return view;
            }
        }
        public String MarginEnd
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    int marginTime;
                    if (ReserveInfo.RecSetting.UseMargineFlag == 1)
                    {
                        marginTime = ReserveInfo.RecSetting.EndMargine;
                    }
                    else
                    {
                        //TODO: ここでデフォルトマージンを確認するがEpgTimerNWでは無意味。根本的にはSendCtrlCmdの拡張が必要
                        marginTime = IniFileHandler.GetPrivateProfileInt("SET", "EndMargin", 0, SettingPath.TimerSrvIniPath);
                    }
                    view = CustomTimeFormat(marginTime);
                }
                return view;
            }
        }
        private String CustomTimeFormat(Int32 span)
        {
            string hours;
            string minutes;
            string seconds = (span % 60).ToString("00;00");
            if (Math.Abs(span) < 3600)
            {
                hours = "";
                minutes = (span / 60).ToString("0;0") + ":";
            }
            else
            {
                hours = (span / 3600).ToString("0;0") + ":";
                minutes = ((span % 3600) / 60).ToString("00;00") + ":";
            }
            return span.ToString("+;-") + hours + minutes + seconds + CustomTimeMark();
        }
        private String CustomTimeMark()
        {
            //EpgtimerNWの場合、デフォルト値不明のため。不明でなくなったら要らない
            String mark = "";
            if (CommonManager.Instance.NWMode == true)
            {
                mark = (ReserveInfo.RecSetting.UseMargineFlag == 1 ? " " : "?");
            }
            return mark;
        }
        public String RecMode
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
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
                }
                return view;
            }
        }
        public String Priority
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    view = ReserveInfo.RecSetting.Priority.ToString();
                }
                return view;
            }
        }
        public String Tuijyu
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    if (ReserveInfo.RecSetting.TuijyuuFlag == 0)
                    {
                        view = "しない";
                    }
                    else if (ReserveInfo.RecSetting.TuijyuuFlag == 1)
                    {
                        view = "する";
                    }
                }
                return view;
            }
        }
        public String Pittari
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    if (ReserveInfo.RecSetting.PittariFlag == 0)
                    {
                        view = "しない";
                    }
                    else if (ReserveInfo.RecSetting.PittariFlag == 1)
                    {
                        view = "する";
                    }
                }
                return view;
            }
        }
        public String Comment
        {
            get
            {
                String view = "";
                if (ReserveInfo != null)
                {
                    view = ReserveInfo.Comment.ToString();
                }
                return view;
            }
        }
        private Boolean IsOnRec
        {
            get
            {
                Boolean retv = false;

                if (ReserveInfo != null)
                {
                    Int32 duration = (Int32)ReserveInfo.DurationSecond;
                    DateTime startTime = ReserveInfo.StartTime;

                    if (ReserveInfo.RecSetting.UseMargineFlag == 1)
                    {
                        startTime = ReserveInfo.StartTime.AddSeconds(ReserveInfo.RecSetting.StartMargine * -1);
                        duration += ReserveInfo.RecSetting.StartMargine;
                        duration += ReserveInfo.RecSetting.EndMargine;
                    }
                    else
                    {
                        //TODO: ここでデフォルトマージンを確認するがEpgTimerNWでは無意味。根本的にはSendCtrlCmdの拡張が必要
                        int defStartMargin = IniFileHandler.GetPrivateProfileInt("SET", "StartMargin", 0, SettingPath.TimerSrvIniPath);
                        int defEndMargin = IniFileHandler.GetPrivateProfileInt("SET", "EndMargin", 0, SettingPath.TimerSrvIniPath);

                        startTime = ReserveInfo.StartTime.AddSeconds(defStartMargin * -1);
                        duration += defStartMargin;
                        duration += defEndMargin;
                    }

                    if (startTime <= System.DateTime.Now)
                    {
                        if (startTime + TimeSpan.FromSeconds(duration) >= System.DateTime.Now)
                        {
                            retv = true;
                        }
                    }
                }

                return retv;
            }
        }
        private Boolean IsOnAir
        {
            get
            {
                Boolean retv = false;

                if (ReserveInfo != null)
                {
                    if (ReserveInfo.StartTime <= System.DateTime.Now)
                    {
                        if (ReserveInfo.StartTime + TimeSpan.FromSeconds(ReserveInfo.DurationSecond) >= System.DateTime.Now)
                        {
                            retv = true;
                        }
                    }
                }

                return retv;
            }
        }
        public String Status
        {
            get
            {
                String[] wiewString = { "", "無", "予+", "無+", "録*", "無*" };
                int index = 0;

                if (IsOnAir == true)
                {
                    index = 2;
                }

                if (IsOnRec == true)//マージンがあるので、IsOnAir==trueとは限らない
                {
                    index = 4;
                }

                if (ReserveInfo.RecSetting.RecMode == 5) //無効の判定
                {
                    index += 1;
                }

                return wiewString[index];
            }
        }
        public SolidColorBrush StatusColor
        {
            get
            {
                SolidColorBrush color = null;

                if (IsOnAir == true)
                {
                    color = CommonManager.Instance.StatOnAirForeColor;
                }

                if (IsOnRec == true)
                {
                    color = CommonManager.Instance.StatRecForeColor;
                }

                if (color == null)
                {
                    color = CommonManager.Instance.StatResForeColor;
                }

                return color;
            }
        }
        public List<String> RecFolder
        {
            get
            {
                List<String> list = new List<string>();
                if (ReserveInfo != null)
                {
                    foreach (RecFileSetInfo recinfo1 in ReserveInfo.RecSetting.RecFolderList)
                    {
                        list.Add(recinfo1.RecFolder);
                    }
                    foreach (RecFileSetInfo recinfo1 in ReserveInfo.RecSetting.PartialRecFolder)
                    {
                        list.Add("(ワンセグ) " + recinfo1.RecFolder);
                    }
                }
                return list;
            }
        }
        public List<String> RecFileName
        {
            get
            {
                List<String> list = new List<string>();
                if (ReserveInfo != null)
                {
                    list = ReserveInfo.RecFileNameList;
                }
                return list;
            }
        }
        public SolidColorBrush ForeColor
        {
            get
            {
                SolidColorBrush color = CommonManager.Instance.ListDefForeColor;
                if (ReserveInfo != null)
                {
                    color = CommonManager.Instance.EventItemForeColor(ReserveInfo.RecSetting.RecMode);
                }
                return color;
            }
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
                String view = "";
                if (ReserveInfo != null)
                {
                    view = CommonManager.Instance.ConvertReserveText(ReserveInfo);
                    /*                    view = ReserveInfo.StartTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
                                        DateTime endTime = ReserveInfo.StartTime + TimeSpan.FromSeconds(ReserveInfo.DurationSecond);
                                        view += endTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss") + "\r\n";

                                        view += ServiceName;
                                        if (0x7880 <= ReserveInfo.OriginalNetworkID && ReserveInfo.OriginalNetworkID <= 0x7FE8)
                                        {
                                            view += " (地デジ)";
                                        }
                                        else if (ReserveInfo.OriginalNetworkID == 0x0004)
                                        {
                                            view += " (BS)";
                                        }
                                        else if (ReserveInfo.OriginalNetworkID == 0x0006)
                                        {
                                            view += " (CS1)";
                                        }
                                        else if (ReserveInfo.OriginalNetworkID == 0x0007)
                                        {
                                            view += " (CS2)";
                                        }
                                        else
                                        {
                                            view += " (その他)";
                                        }
                                        view += "\r\n";

                                        view += EventName + "\r\n\r\n";
                                        view += "録画モード : " + RecMode + "\r\n";
                                        view += "優先度 : " + Priority + "\r\n";
                                        view += "追従 : " + Tuijyu + "\r\n";
                                        view += "ぴったり（？） : " + Pittari + "\r\n";
                                        if ((ReserveInfo.RecSetting.ServiceMode & 0x01) == 0)
                                        {
                                            view += "指定サービス対象データ : デフォルト\r\n";
                                        }
                                        else
                                        {
                                            view += "指定サービス対象データ : ";
                                            if ((ReserveInfo.RecSetting.ServiceMode & 0x10) > 0)
                                            {
                                                view += "字幕含む ";
                                            }
                                            if ((ReserveInfo.RecSetting.ServiceMode & 0x20) > 0)
                                            {
                                                view += "データカルーセル含む";
                                            }
                                            view += "\r\n";
                                        }

                                        view += "録画実行bat : " + ReserveInfo.RecSetting.BatFilePath + "\r\n";

                                        if (ReserveInfo.RecSetting.RecFolderList.Count == 0)
                                        {
                                            view += "録画フォルダ : デフォルト\r\n";
                                        }
                                        else
                                        {
                                            view += "録画フォルダ : \r\n";
                                            foreach (RecFileSetInfo info in ReserveInfo.RecSetting.RecFolderList)
                                            {
                                                view += info.RecFolder + " (WritePlugIn:" + info.WritePlugIn + ")\r\n";
                                            }
                                        }

                                        if (ReserveInfo.RecSetting.UseMargineFlag == 0)
                                        {
                                            view += "録画マージン : デフォルト\r\n";
                                        }
                                        else
                                        {
                                            view += "録画マージン : 開始 " + ReserveInfo.RecSetting.StartMargine.ToString() +
                                                " 終了 " + ReserveInfo.RecSetting.EndMargine.ToString() + "\r\n";
                                        }

                                        if (ReserveInfo.RecSetting.SuspendMode == 0)
                                        {
                                            view += "録画後動作 : デフォルト\r\n";
                                        }
                                        else
                                        {
                                            view += "録画後動作 : ";
                                            switch (ReserveInfo.RecSetting.SuspendMode)
                                            {
                                                case 1:
                                                    view += "スタンバイ";
                                                    break;
                                                case 2:
                                                    view += "休止";
                                                    break;
                                                case 3:
                                                    view += "シャットダウン";
                                                    break;
                                                case 4:
                                                    view += "何もしない";
                                                    break;
                                            }
                                            if (ReserveInfo.RecSetting.RebootFlag == 1)
                                            {
                                                view += " 復帰後再起動する";
                                            }
                                            view += "\r\n";
                                        }
                                        view += "予約状況 : " + ReserveInfo.Comment;
                                        view += "\r\n\r\n";
                                        view += "OriginalNetworkID : " + ReserveInfo.OriginalNetworkID.ToString() + " (0x" + ReserveInfo.OriginalNetworkID.ToString("X4") + ")\r\n";
                                        view += "TransportStreamID : " + ReserveInfo.TransportStreamID.ToString() + " (0x" + ReserveInfo.TransportStreamID.ToString("X4") + ")\r\n";
                                        view += "ServiceID : " + ReserveInfo.ServiceID.ToString() + " (0x" + ReserveInfo.ServiceID.ToString("X4") + ")\r\n";
                                        view += "EventID : " + ReserveInfo.EventID.ToString() + " (0x" + ReserveInfo.EventID.ToString("X4") + ")\r\n";*/
                }


                TextBlock block = new TextBlock();
                block.Text = view;
                block.MaxWidth = 400;
                block.TextWrapping = TextWrapping.Wrap;
                return block;
            }
        }

        public EpgEventInfo EventInfo
        {
            get
            {
                EpgEventInfo eventInfo1 = null;
                UInt64 key1 = CommonManager.Create64Key(ReserveInfo.OriginalNetworkID, ReserveInfo.TransportStreamID, ReserveInfo.ServiceID);
                if (CommonManager.Instance.DB.ServiceEventList.ContainsKey(key1) == true)
                {
                    foreach (EpgEventInfo eventChkInfo1 in CommonManager.Instance.DB.ServiceEventList[key1].eventList)
                    {
                        if (eventChkInfo1.event_id == ReserveInfo.EventID)
                        {
                            eventInfo1 = eventChkInfo1;
                            break;
                        }
                    }
                }
                return eventInfo1;
            }
        }

        public SolidColorBrush BorderBrush
        {
            get
            {
                SolidColorBrush color1 = Brushes.White;
                if (this.EventInfo != null)
                {
                    if (this.EventInfo.ContentInfo != null)
                    {
                        if (this.EventInfo.ContentInfo.nibbleList.Count > 0)
                        {
                            try
                            {
                                foreach (EpgContentData info1 in this.EventInfo.ContentInfo.nibbleList)
                                {
                                    if (info1.content_nibble_level_1 <= 0x0B || info1.content_nibble_level_1 == 0x0F && Settings.Instance.ContentColorList.Count > info1.content_nibble_level_1)
                                    {
                                        color1 = CommonManager.Instance.CustContentColorList[info1.content_nibble_level_1];
                                        break;
                                    }
                                }
                            }
                            catch
                            {
                            }
                        }
                        else
                        {
                            color1 = CommonManager.Instance.CustContentColorList[0x10];
                        }
                    }
                    else
                    {
                        color1 = CommonManager.Instance.CustContentColorList[0x10];
                    }
                }

                return color1;
            }
        }

        /// <summary>
        /// 番組詳細
        /// </summary>
        public string ProgramDetail
        {
            get
            {
                string text1 = "Unavailable (;_;)";
                if (this.EventInfo != null)
                {
                    text1 = CommonManager.Instance.ConvertProgramText(this.EventInfo, EventInfoTextMode.All);
                }
                return text1;
            }
        }
    }
}
