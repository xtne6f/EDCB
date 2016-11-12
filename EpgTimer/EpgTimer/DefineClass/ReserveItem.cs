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

        public Brush BorderBrush
        {
            get
            {
                Brush color1 = Brushes.White;
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
