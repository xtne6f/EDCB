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
                if (_data.GetNextReserve() == null) return long.MaxValue;
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
        public virtual TextBlock ToolTipView
        {
            get
            {
                if (Settings.Instance.NoToolTip == true) return null;
                //
                return mutil.GetTooltipBlockStandard(ConvertInfoText());
            }
        }
        public virtual TextBlock ToolTipViewAutoAddSearch
        {
            get { return mutil.GetTooltipBlockStandard(ConvertInfoText()); }
        }
        public virtual String ConvertInfoText() { return ""; }
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

    public static class AutoAddDataItemEx
    {
        public static AutoAddDataItem CreateIncetance(AutoAddData data)
        {
            if (data is EpgAutoAddData)
            {
                return new EpgAutoDataItem(data as EpgAutoAddData);
            }
            else if (data is ManualAutoAddData)
            {
                return new ManualAutoAddDataItem(data as ManualAutoAddData);
            }
            else
            {
                return new AutoAddDataItem(data);
            }
        }

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
                    string network1 = "?";
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
                return view1.TrimEnd(','); ;
            }
        }        /// <summary>
        /// NHK総合１・東京、NHKBS1
        /// </summary>
        public override String ServiceName
        {
            get { return _ServiceName(2); }
        }
        private String _ServiceName(int count = -1, bool withNetwork = false)
        {
            if (EpgAutoAddInfo == null) return "";
            //
            String view = "";
            int countAll = EpgAutoAddInfo.searchInfo.serviceList.Count;
            foreach (ulong service1 in EpgAutoAddInfo.searchInfo.serviceList.Take(count == -1 ? countAll : count))
            {
                if (view != "") { view += ", "; }
                ChSet5Item chSet5Item1;
                if (ChSet5.Instance.ChList.TryGetValue(service1, out chSet5Item1) == true)
                {
                    view += chSet5Item1.ServiceName + (withNetwork == true ? "(" + CommonManager.ConvertNetworkNameText(chSet5Item1.ONID) + ")" : "");
                }
                else
                {
                    view += "?" + (withNetwork == true ? "(?)" : "");
                }
            }
            if (count != -1 && count < countAll)
            {
                view += (view == "" ? "" : ", ") + "他" + (countAll - count);
            }
            if (view == "")
            {
                view = "なし";
            }
            return view;
        }
        public override String ConvertInfoText()
        {
            if (EpgAutoAddInfo == null) return "";
            //
            String view = "";
            view += "Andキーワード : " + EventName + "\r\n";
            view += "Notキーワード : " + NotKey + "\r\n";
            view += "正規表現モード : " + RegExp + "\r\n";
            view += "番組名のみ検索対象 : " + TitleOnly + "\r\n";
            view += "自動登録 : " + (KeyEnabled == true ? "有効" : "無効") + "\r\n";
            view += "ジャンル絞り込み : " + JyanruKey + "\r\n";
            view += "時間絞り込み : " + DateKey + "\r\n";
            view += "検索対象サービス : " + _ServiceName(10, true);

            view += "\r\n\r\n" + ConvertRecSettingText();
            return view;
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
                return ViewUtil.EpgDataContentBrush(EpgAutoAddInfo.searchInfo.contentList);
            }
        }

    }

}
