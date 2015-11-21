using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Media;
using System.Windows.Controls;
using System.Windows;

namespace EpgTimer
{
    public class EpgAutoDataItem
    {
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private ViewUtil vutil = CommonManager.Instance.VUtil;

        public EpgAutoDataItem() { }
        public EpgAutoDataItem(EpgAutoAddData item)
        {
            this.EpgAutoAddInfo = item;
        }

        public EpgAutoAddData EpgAutoAddInfo { get; set; }

        public String AndKey
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return EpgAutoAddInfo.searchInfo.andKey;
            }
        }
        public String NotKey
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return EpgAutoAddInfo.searchInfo.notKey;
            }
        }
        public String RegExp
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return EpgAutoAddInfo.searchInfo.regExpFlag == 1 ? "○" : "×";
            }
        }
        public String TitleOnly
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return EpgAutoAddInfo.searchInfo.titleOnlyFlag == 1 ? "○" : "×";
            }
        }
        public String DateKey
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                if (EpgAutoAddInfo.searchInfo.dateList.Count == 1)
                {
                    EpgSearchDateInfo info = EpgAutoAddInfo.searchInfo.dateList[0];
                    return CommonManager.Instance.DayOfWeekDictionary[info.startDayOfWeek] + " " + info.startHour.ToString("00") + ":" + info.startMin.ToString("00") +
                        " ～ " + CommonManager.Instance.DayOfWeekDictionary[info.endDayOfWeek] + " " + info.endHour.ToString("00") + ":" + info.endMin.ToString("00");
                }
                if (EpgAutoAddInfo.searchInfo.dateList.Count > 1)
                {
                    return "複数指定";
                }
                return "なし";
            }
        }
        public String RecMode
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return CommonManager.Instance.ConvertRecModeText(EpgAutoAddInfo.recSetting.RecMode);
            }
        }
        public String Priority
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return EpgAutoAddInfo.recSetting.Priority.ToString();
            }
        }
        public String Tuijyu
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return CommonManager.Instance.YesNoDictionary[EpgAutoAddInfo.recSetting.TuijyuuFlag].DisplayName;
            }
        }
        public String Pittari
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return CommonManager.Instance.YesNoDictionary[EpgAutoAddInfo.recSetting.PittariFlag].DisplayName;
            }
        }
        public String AddCount
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return EpgAutoAddInfo.addCount.ToString();
            }
        }
        //詳細ウィンドウを開いたときの項目数と同じもの。
        //無効時でも有効時のAddCountと同じ数字が入る。
        public String SearchCount 
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return EpgAutoAddInfo.SearchCount().ToString();
            }
        }
        //"SearchCount"のうち、予約アイテム数
        //検索の無効・有効によってAddCountやSearchCountと異なる値になる。
        public String ReserveCount 
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return EpgAutoAddInfo.ReserveCount().ToString();
            }
        }
        //"ReserveCount"のうち、有効な予約アイテム数
        public String OnCount
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return EpgAutoAddInfo.OnCount().ToString();
            }
        }
        //"ReserveCount"のうち、無効な予約アイテム数
        public String OffCount
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return EpgAutoAddInfo.OffCount().ToString();
            }
        }
        public String NextReserve
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return new ReserveItem(EpgAutoAddInfo.GetNextReserve()).StartTime;
            }
        }
        public String JyanruKey
        {
            get
            {
                if (EpgAutoAddInfo == null || EpgAutoAddInfo.searchInfo == null) return "";
                //
                String view = CommonManager.Instance.ConvertJyanruText(EpgAutoAddInfo.searchInfo);
                if (view != "" && EpgAutoAddInfo.searchInfo.notContetFlag == 1)
                {
                    view = "NOT " + view;
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
                if (EpgAutoAddInfo == null) return "";
                //
                String view = "";
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
                if (EpgAutoAddInfo == null) return "";
                //
                String view1 = "";
                List<string> networkKeyList1 = new List<string>();
                foreach (ulong service1 in this.EpgAutoAddInfo.searchInfo.serviceList)
                {
                    string network1 = "(x_x)";
                    ChSet5Item chSet5Item1;
                    if (ChSet5.Instance.ChList.TryGetValue(service1, out chSet5Item1) == true)
                    {
                        network1 = CommonManager.ConvertNetworkNameText(chSet5Item1.ONID, true);
                    }

                    if (networkKeyList1.Contains(network1) == false)
                    {
                        networkKeyList1.Add(network1);
                        view1 += network1 + ",";
                    }
                }
                if (view1 == "")
                {
                    view1 = "なし";
                }
                return view1.TrimEnd(',');;
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
                if (EpgAutoAddInfo == null) return "";
                //
                return mutil.MarginText(EpgAutoAddInfo.recSetting, true);
            }
        }
        public String MarginEnd
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return mutil.MarginText(EpgAutoAddInfo.recSetting, false);
            }
        }
        public String Preset
        {
            get
            {
                if (EpgAutoAddInfo == null) return "";
                //
                return EpgAutoAddInfo.recSetting.LookUpPreset().DisplayName;
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
                //選択されている場合、複数選択時に1回の通信で処理するため、ウインドウ側に処理を渡す。
                MainWindow mainWindow = (MainWindow)Application.Current.MainWindow;
                mainWindow.autoAddView.epgAutoAddView.ChgKeyEnabledFromCheckbox(this);
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
                if (EpgAutoAddInfo != null)
                {
                    if (EpgAutoAddInfo.searchInfo.keyDisabledFlag == 1)
                    {
                        return CommonManager.Instance.RecModeForeColor[5];
                    }
                }
                return CommonManager.Instance.ListDefForeColor;
            }
        }
        public SolidColorBrush BackColor
        {
            get
            {
                if (EpgAutoAddInfo != null)
                {
                    if (EpgAutoAddInfo.searchInfo.keyDisabledFlag == 1)
                    {
                        return CommonManager.Instance.ResNoBackColor;
                    }
                }
                return CommonManager.Instance.ResDefBackColor;
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
                if (EpgAutoAddInfo == null) return "";
                //
                String view = "";
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
                return view;
            }
        }
        public Brush BorderBrush
        {
            get
            {
                if (EpgAutoAddInfo == null) return Brushes.White;
                //
                if (EpgAutoAddInfo.searchInfo.contentList.Count == 0 || EpgAutoAddInfo.searchInfo.notContetFlag != 0)
                {
                    return Brushes.Gainsboro;
                }
                return vutil.EpgDataContentBrush(EpgAutoAddInfo.searchInfo.contentList);
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
