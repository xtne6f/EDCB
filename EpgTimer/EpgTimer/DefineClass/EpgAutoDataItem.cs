using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Media;
using System.Windows.Controls;
using System.Windows;

namespace EpgTimer
{
    //キーワード予約とプログラム自動登録の共通項目
    public class AutoAddDataItem : RecSettingItem
    {
        protected MenuUtil mutil = CommonManager.Instance.MUtil;
        protected ViewUtil vutil = CommonManager.Instance.VUtil;

        protected AutoAddData _data;
        public AutoAddData Data { get { return _data; } set { _data = value; } }

        public AutoAddDataItem() {}
        public AutoAddDataItem(AutoAddData data) { _data = data; }

        public static new string GetValuePropertyName(string key)
        {
            var obj = new AutoAddDataItem();
            if (key == CommonUtil.GetMemberName(() => obj.NextReserve))
            {
                return CommonUtil.GetMemberName(() => obj.NextReserveValue);
            }
            else
            {
                return RecSettingItem.GetValuePropertyName(key);
            }
        }

        public override RecSettingData RecSettingInfo { get { return _data != null ? _data.RecSettingInfo : null; } }

        public String EventName
        {
            get
            {
                if (_data == null) return "";
                //
                return _data.DataTitle;
            }
        }
        public String SearchCount
        {
            get
            {
                if (_data == null) return "";
                //
                return _data.SearchCount.ToString();
            }
        }
        public String ReserveCount
        {
            get
            {
                if (_data == null) return "";
                //
                return _data.ReserveCount.ToString();
            }
        }
        //"ReserveCount"のうち、有効な予約アイテム数
        public String OnCount
        {
            get
            {
                if (_data == null) return "";
                //
                return _data.OnCount.ToString();
            }
        }
        //"ReserveCount"のうち、無効な予約アイテム数
        public String OffCount
        {
            get
            {
                if (_data == null) return "";
                //
                return _data.OffCount.ToString();
            }
        }
        public String NextReserve
        {
            get
            {
                if (_data == null) return "";
                //
                return new ReserveItem(_data.GetNextReserve()).StartTime;
            }
        }
        public long NextReserveValue
        {
            get
            {
                if (_data == null) return long.MinValue;
                //
                return new ReserveItem(_data.GetNextReserve()).StartTimeValue;
            }
        }
        public virtual String NetworkName { get { return ""; } }
        public virtual String ServiceName { get { return ""; } }
        public override string ToString()
        {
            return CommonManager.Instance.ConvertTextSearchString(EventName);
        }
        public virtual TextBlock ToolTipView { get { return null; } }
        public virtual bool KeyEnabled
        {
            set
            {
                EpgCmds.ChgOnOffCheck.Execute(this, null);
            }
            get
            {
                if (_data == null) return false;
                //
                return _data.IsEnabled;
            }
        }
        public SolidColorBrush ForeColor
        {
            get
            {
                if (_data != null)
                {
                    if (_data.IsEnabled == false)
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
                if (_data != null)
                {
                    if (_data.IsEnabled == false)
                    {
                        return CommonManager.Instance.ResNoBackColor;
                    }
                }
                return CommonManager.Instance.ResDefBackColor;
            }
        }
        public virtual Brush BorderBrush { get { return Brushes.White; } }
    }

    //T型との関連付け
    public class AutoAddDataItemT<T> : AutoAddDataItem where T : AutoAddData
    {
        public AutoAddDataItemT() { }
        public AutoAddDataItemT(T item) : base(item) { }
    }

    public static class AutoDataItemEx
    {
        public static List<T> AutoAddInfoList<T>(this IEnumerable<AutoAddDataItemT<T>> itemlist) where T : AutoAddData
        {
            return itemlist.Where(item => item != null).Select(item => (T)item.Data).ToList();
        }
    }

    public class EpgAutoDataItem : AutoAddDataItemT<EpgAutoAddData>
    {
        public EpgAutoDataItem() { }
        public EpgAutoDataItem(EpgAutoAddData item) : base(item) { }

        public EpgAutoAddData EpgAutoAddInfo { get { return (EpgAutoAddData)_data; } set { _data = value; } }

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
                switch (EpgAutoAddInfo.searchInfo.dateList.Count)
                {
                    case 0: return "なし";
                    case 1: return CommonManager.ConvertTimeText(EpgAutoAddInfo.searchInfo.dateList[0]);
                    default: return "複数指定";
                }
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
        /// 地デジ、BS、CS
        /// </summary>
        public override String NetworkName
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
        /// <summary>
        /// NHK総合１・東京、NHKBS1
        /// </summary>
        public override String ServiceName
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
        public override TextBlock ToolTipView
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
                    view += "Andキーワード：" + EventName + "\r\n";
                    view += "Notキーワード：" + NotKey + "\r\n";
                    view += "正規表現モード：" + RegExp + "\r\n";
                    view += "番組名のみ検索対象：" + TitleOnly + "\r\n";
                    view += "ジャンル絞り込み：" + JyanruKey + "\r\n";
                    view += "時間絞り込み：" + DateKey + "\r\n";
                    view += "検索対象サービス：" + ServiceName + "\r\n";

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
        public override Brush BorderBrush
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

}
