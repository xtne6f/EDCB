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
    public class EpgAutoDataItem
    {
        private MenuUtil mutil = CommonManager.Instance.MUtil;

        public EpgAutoDataItem(EpgAutoAddData item)
        {
            this.EpgAutoAddInfo = item;
        }

        public EpgAutoAddData EpgAutoAddInfo
        {
            get;
            set;
        }

        public String AndKey
        {
            get
            {
                String view = "";
                if (EpgAutoAddInfo != null)
                {
                    view = EpgAutoAddInfo.searchInfo.andKey;
                }
                return view;
            }
        }
        public String NotKey
        {
            get
            {
                String view = "";
                if (EpgAutoAddInfo != null)
                {
                    view = EpgAutoAddInfo.searchInfo.notKey;
                }
                return view;
            }
        }
        public String RegExp
        {
            get
            {
                String view = "×";
                if (EpgAutoAddInfo != null)
                {
                    if (EpgAutoAddInfo.searchInfo.regExpFlag == 1)
                    {
                        view = "○";
                    }
                }
                return view;
            }
        }
        public String TitleOnly
        {
            get
            {
                String view = "×";
                if (EpgAutoAddInfo != null)
                {
                    if (EpgAutoAddInfo.searchInfo.titleOnlyFlag == 1)
                    {
                        view = "○";
                    }
                }
                return view;
            }
        }
        public String DateKey
        {
            get
            {
                String view = "なし";
                if (EpgAutoAddInfo != null)
                {
                    if (EpgAutoAddInfo.searchInfo.dateList.Count == 1)
                    {
                        EpgSearchDateInfo info = EpgAutoAddInfo.searchInfo.dateList[0];
                        view = CommonManager.Instance.DayOfWeekDictionary[info.startDayOfWeek] + " " + info.startHour.ToString("00") + ":" + info.startMin.ToString("00") +
                            " ～ " + CommonManager.Instance.DayOfWeekDictionary[info.endDayOfWeek] + " " + info.endHour.ToString("00") + ":" + info.endMin.ToString("00");
                    }
                    else if (EpgAutoAddInfo.searchInfo.dateList.Count > 1)
                    {
                        view = "複数指定";
                    }
                }
                return view;
            }
        }
        public String RecMode
        {
            get
            {
                String view = "";
                if (EpgAutoAddInfo != null)
                {
                    view = CommonManager.Instance.ConvertRecModeText(EpgAutoAddInfo.recSetting.RecMode);
                }
                return view;
            }
        }
        public String Priority
        {
            get
            {
                String view = "";
                if (EpgAutoAddInfo != null)
                {
                    view = EpgAutoAddInfo.recSetting.Priority.ToString();
                }
                return view;
            }
        }
        public String Tuijyu
        {
            get
            {
                String view = "";
                if (EpgAutoAddInfo != null)
                {
                    if (EpgAutoAddInfo.recSetting.TuijyuuFlag == 0)
                    {
                        view = "しない";
                    }
                    else if (EpgAutoAddInfo.recSetting.TuijyuuFlag == 1)
                    {
                        view = "する";
                    }
                }
                return view;
            }
        }
        public String AddCount
        {
            get
            {
                String view = "0";
                if (EpgAutoAddInfo != null)
                {
                    view = EpgAutoAddInfo.addCount.ToString();
                }
                return view;
            }
        }

        //詳細ウィンドウを開いたときの項目数と同じもの。
        //無効時でも有効時のAddCountと同じ数字が入る。
        public String SearchCount 
        {
            get
            {
                String view = "0";
                if (EpgAutoAddInfo != null)
                {
                    view = EpgAutoAddInfo.SearchCount().ToString();
                }
                return view;
            }
        }

        //"SearchCount"のうち、予約アイテム数
        //検索の無効・有効によってAddCountやSearchCountと異なる値になる。
        public String ReserveCount 
        {
            get
            {
                String view = "0";
                if (EpgAutoAddInfo != null)
                {
                    view = EpgAutoAddInfo.ReserveCount().ToString();
                }
                return view;
            }
        }

        //"ReserveCount"のうち、有効な予約アイテム数
        public String OnCount
        {
            get
            {
                String view = "0";
                if (EpgAutoAddInfo != null)
                {
                    view = EpgAutoAddInfo.OnCount().ToString();
                }
                return view;
            }
        }

        //"ReserveCount"のうち、無効な予約アイテム数
        public String OffCount
        {
            get
            {
                String view = "0";
                if (EpgAutoAddInfo != null)
                {
                    view = EpgAutoAddInfo.OffCount().ToString();
                }
                return view;
            }
        }

        public String JyanruKey
        {
            get
            {
                String view = "";
                if (EpgAutoAddInfo != null && EpgAutoAddInfo.searchInfo != null)
                {
                    view = CommonManager.Instance.ConvertJyanruText(EpgAutoAddInfo.searchInfo);
                }
                return view;
            }
        }

        /// <summary>
        /// NHK総合１・東京、NHKBS1
        /// </summary>
        public String ServiceKey
        {
            get
            {
                String view = "";
                if (EpgAutoAddInfo != null)
                {
                    foreach (ulong service1 in EpgAutoAddInfo.searchInfo.serviceList)
                    {
                        try
                        {
                            if (view != "") { view += ", "; }
                            view += ChSet5.Instance.ChList[service1].ServiceName;
                        }
                        catch
                        {
                            view += "(x_x)";
                        }
                    }
                    //if (EpgAutoAddInfo.searchInfo.serviceList.Count == 1)
                    //{
                    //    try
                    //    {
                    //        view = ChSet5.Instance.ChList[(ulong)EpgAutoAddInfo.searchInfo.serviceList[0]].ServiceName;
                    //    }
                    //    catch
                    //    {
                    //        view = "検索エラー";
                    //    }
                    //}
                    //else if (EpgAutoAddInfo.searchInfo.serviceList.Count > 1)
                    //{
                    //    view = "複数指定";
                    //}
                }
                if (view == "")
                {
                    view = "なし";
                }
                return view;
            }
        }

        /// <summary>
        /// 地デジ、BS、CS
        /// </summary>
        public String NetworkKey
        {
            get
            {
                String view1 = "";
                if (this.EpgAutoAddInfo != null)
                {
                    List<string> networkKeyList1 = new List<string>();
                    foreach (ulong service1 in this.EpgAutoAddInfo.searchInfo.serviceList)
                    {
                        string network1 = "";
                        try
                        {
                            ChSet5Item chSet5Item1 = ChSet5.Instance.ChList[service1];
                            // SearchKeyDescViewよりコピペ
                            if ((0x7880 <= chSet5Item1.ONID && chSet5Item1.ONID <= 0x7FE8) &&
                                (chSet5Item1.ServiceType == 0x01 || chSet5Item1.ServiceType == 0xA5))
                            {
                                network1 = "地デジ";
                            }
                            else if (chSet5Item1.ONID == 0x04 &&
                              (chSet5Item1.ServiceType == 0x01 || chSet5Item1.ServiceType == 0xA5))
                            {
                                network1 = "BS";
                            }
                            else if ((chSet5Item1.ONID == 0x06 || chSet5Item1.ONID == 0x07) &&
                              (chSet5Item1.ServiceType == 0x01 || chSet5Item1.ServiceType == 0xA5))
                            {
                                network1 = "CS";
                            }
                            else
                            {
                                network1 = "(?_?)";
                            }
                            //network1 = ChSet5.Instance.ChList[service1].NetworkName;
                        }
                        catch
                        {
                            network1 = "(x_x)";
                        }
                        if (!networkKeyList1.Contains(network1))
                        {
                            networkKeyList1.Add(network1);
                        }
                    }
                    foreach (string network1 in networkKeyList1)
                    {
                        if (view1 != "") { view1 += ", "; }
                        view1 += network1;
                    }
                }
                if (view1 == "")
                {
                    view1 = "なし";
                }
                return view1;
            }
        }

        public String Pittari
        {
            get
            {
                String view = "";
                if (EpgAutoAddInfo != null)
                {
                    if (EpgAutoAddInfo.recSetting.PittariFlag == 0)
                    {
                        view = "しない";
                    }
                    else if (EpgAutoAddInfo.recSetting.PittariFlag == 1)
                    {
                        view = "する";
                    }
                }
                return view;
            }
        }

        public String Tuner
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return CommonManager.Instance.ConvertTunerText(EpgAutoAddInfo.recSetting.TunerID);
            }
        }

        public String MarginStart
        {
            get
            {
                String view = "";
                if (EpgAutoAddInfo != null)
                {
                    view = mutil.MarginText(EpgAutoAddInfo.recSetting, true);
                }
                return view;
            }
        }

        public String MarginEnd
        {
            get
            {
                String view = "";
                if (EpgAutoAddInfo != null)
                {
                    view = mutil.MarginText(EpgAutoAddInfo.recSetting, false);
                }
                return view;
            }
        }

        public List<String> RecFolder
        {
            get
            {
                List<String> list = new List<string>();
                if (EpgAutoAddInfo != null)
                {
                    EpgAutoAddInfo.recSetting.RecFolderList.ForEach(info => list.Add(info.RecFolder));
                    EpgAutoAddInfo.recSetting.PartialRecFolder.ForEach(info => list.Add("(ワンセグ) " + info.RecFolder));
                }
                return list;
            }
        }

        public bool KeyEnabled
        {
            set
            {
                //選択されている場合、複数選択時に1回の通信で処理するため、処理を割り込ませる。
                MainWindow mainWindow = (MainWindow)Application.Current.MainWindow;
                if (mainWindow.autoAddView.epgAutoAddView.ChgKeyEnabledFromCheckbox(this) == true) return;

                //通常(単独)の処理
                if (EpgAutoAddInfo != null)
                {
                    if ((this.EpgAutoAddInfo.searchInfo.keyDisabledFlag != 1) != value)
                    {
                        mutil.EpgAutoAddChangeOnOffKeyEnabled(mutil.ToList(this.EpgAutoAddInfo));
                    }
                }
            }
            get
            {
                if (EpgAutoAddInfo == null) return false;
                //
                return EpgAutoAddInfo.searchInfo.keyDisabledFlag != 1;
            }
        }

        public SolidColorBrush ForeColor
        {
            get
            {
                SolidColorBrush color = CommonManager.Instance.ListDefForeColor;
                if (EpgAutoAddInfo != null)
                {
                    if (EpgAutoAddInfo.searchInfo.keyDisabledFlag == 1)
                    {
                        color = CommonManager.Instance.EventItemForeColor(5);
                    }
                }
                return color;
            }
        }
        
        public SolidColorBrush BackColor
        {
            get
            {
                SolidColorBrush color = CommonManager.Instance.ResDefBackColor;
                if (EpgAutoAddInfo != null)
                {
                    if (EpgAutoAddInfo.searchInfo.keyDisabledFlag == 1)
                    {
                        color = CommonManager.Instance.ResNoBackColor;
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
                return mutil.GetTooltipBlockStandard(SearchInfoText);
            }
        }

        public String SearchInfoText
        {
            get
            {
                String view = "";
                if (EpgAutoAddInfo != null)
                {
                    if (EpgAutoAddInfo.searchInfo != null)
                    {
                        view += "検索条件\r\n";
                        view += "Andキーワード：" + AndKey + "\r\n";
                        view += "Notキーワード：" + NotKey + "\r\n";
                        view += "正規表現モード：" + RegExp + "\r\n";
                        view += "番組名のみ検索対象：" + TitleOnly + "\r\n";
                        view += "ジャンル絞り込み：" + JyanruKey + "\r\n";
                        view += "時間絞り込み：" + DateKey + "\r\n";
                        view += "検索対象サービス：" + ServiceKey + "\r\n";

                        view += "\r\n";
                    }
                    if (EpgAutoAddInfo.recSetting != null)
                    {
                        view += "録画設定\r\n";
                        view += "録画モード：" + RecMode + "\r\n";
                        view += "優先度：" + Priority + "\r\n";
                        view += "追従：" + Tuijyu + "\r\n";

                        if ((EpgAutoAddInfo.recSetting.ServiceMode & 0x01) == 0)
                        {
                            view += "指定サービス対象データ : デフォルト\r\n";
                        }
                        else
                        {
                            view += "指定サービス対象データ : ";
                            if ((EpgAutoAddInfo.recSetting.ServiceMode & 0x10) > 0)
                            {
                                view += "字幕含む ";
                            }
                            if ((EpgAutoAddInfo.recSetting.ServiceMode & 0x20) > 0)
                            {
                                view += "データカルーセル含む";
                            }
                            view += "\r\n";
                        }

                        view += "録画実行bat : " + EpgAutoAddInfo.recSetting.BatFilePath + "\r\n";

                        if (EpgAutoAddInfo.recSetting.RecFolderList.Count == 0)
                        {
                            view += "録画フォルダ : デフォルト\r\n";
                        }
                        else
                        {
                            view += "録画フォルダ : \r\n";
                            foreach (RecFileSetInfo info in EpgAutoAddInfo.recSetting.RecFolderList)
                            {
                                view += info.RecFolder + " (WritePlugIn:" + info.WritePlugIn + ")\r\n";
                            }
                        }

                        if (EpgAutoAddInfo.recSetting.UseMargineFlag == 0)
                        {
                            view += "録画マージン : デフォルト\r\n";
                        }
                        else
                        {
                            view += "録画マージン : 開始 " + EpgAutoAddInfo.recSetting.StartMargine.ToString() +
                                " 終了 " + EpgAutoAddInfo.recSetting.EndMargine.ToString() + "\r\n";
                        }

                        if (EpgAutoAddInfo.recSetting.SuspendMode == 0)
                        {
                            view += "録画後動作 : デフォルト\r\n";
                        }
                        else
                        {
                            view += "録画後動作 : ";
                            switch (EpgAutoAddInfo.recSetting.SuspendMode)
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
                            if (EpgAutoAddInfo.recSetting.RebootFlag == 1)
                            {
                                view += " 復帰後再起動する";
                            }
                            view += "\r\n";
                        }
                    }
                }
                return view;
            }
        }

        public Brush BorderBrush
        {
            get
            {
                Brush color1 = Brushes.Gainsboro;
                if (this.EpgAutoAddInfo.searchInfo.contentList.Count > 0)
                {
                    byte content_nibble_level_1 = this.EpgAutoAddInfo.searchInfo.contentList[0].content_nibble_level_1;
                    if (content_nibble_level_1 <= 0x0B || content_nibble_level_1 == 0x0F && Settings.Instance.ContentColorList.Count > content_nibble_level_1)
                    {
                        try
                        {
                            color1 = CommonManager.Instance.CustContentColorList[content_nibble_level_1];
                        }
                        catch
                        {
                            ;
                        }
                    }
                }

                return color1;
            }
        }

    }

    public static class EpgAutoDataItemEx
    {
        public static List<EpgAutoAddData> EpgAutoAddInfoList(this ICollection<EpgAutoDataItem> itemlist)
        {
            return itemlist.Where(item => item != null).Select(item => item.EpgAutoAddInfo).ToList();
        }
    }

}
