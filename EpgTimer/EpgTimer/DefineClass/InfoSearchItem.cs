using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Media;
using System.Windows.Controls;
using System.Windows;
using System.Windows.Input;

namespace EpgTimer
{
    public class InfoSearchItem : DataListItemBase, IRecWorkMainData
    {
        public InfoSearchItem() { tIdx = dTypes.Count - 1; }
        public InfoSearchItem(IRecWorkMainData data)
        {
            tIdx = dTypes.IndexOf(data.GetType());
            tIdx = tIdx < 0 ? dTypes.Count - 1 : tIdx;

            this.Data = data;
            ViewItem = Activator.CreateInstance(vTypes[tIdx], data) as DataListItemBase;
        }
        private int tIdx;
        public IRecWorkMainData Data { get; private set; }
        public DataListItemBase ViewItem { get; private set; }

        public static IEnumerable<InfoSearchItem> Items(IRecWorkMainData d) { return new List<InfoSearchItem> { new InfoSearchItem(d) }; }
        public static IEnumerable<InfoSearchItem> Items(IEnumerable<IRecWorkMainData> list) { return list.Select(d => new InfoSearchItem(d)); }

        class DummyType { public DummyType(IRecWorkMainData data) { } }
        private static List<Type> dTypes = new List<Type> { typeof(ReserveData), typeof(RecFileInfo), typeof(EpgAutoAddData), typeof(ManualAutoAddData), typeof(DummyType) };
        private static List<Type> vTypes = new List<Type> { typeof(ReserveItem), typeof(RecInfoItem), typeof(EpgAutoDataItem), typeof(ManualAutoAddDataItem), typeof(DummyType) };

        private static List<String> viewItemNames = new List<String> { "予約", "録画済み", "キーワード予約", "プログラム予約", "" };
        public static List<String> ViewTypeNameList() { return viewItemNames.Take(viewItemNames.Count - 1).ToList(); }
        public String ViewItemName { get { return viewItemNames[tIdx]; } }

        private static List<ulong> keyIDOffset = new List<ulong> { 0x01UL << 60, 0x02UL << 60, 0x03UL << 60, 0x04UL << 60, 0 };
        public override ulong KeyID { get { return keyIDOffset[tIdx] | ViewItem.KeyID; } }

        public String Status
        {
            get
            {
                if      (ViewItem is ReserveItem)           return ((ReserveItem)ViewItem).Status;
                else if (ViewItem is RecInfoItem)           return ((RecInfoItem)ViewItem).IsProtect == true ? "プロテクト" : "";
                else if (Data is AutoAddData)               return (((AutoAddData)Data).IsEnabled == false ? "無" : "");
                else                                        return "";
            }
        }
        public Brush StatusColor
        {
            get
            {
                if  (ViewItem is ReserveItem)               return ((ReserveItem)ViewItem).StatusColor;
                else                                        return CommonManager.Instance.StatResForeColor;
            }
        }
        public String EventName
        {
            get
            {
                if (Data == null) return "";
                //
                return Data.DataTitle;
            }
        }
        public String DataTitle { get { return EventName; } }
        public String StartTime
        {
            get
            {
                if  (ViewItem is AutoAddDataItem)           return ((AutoAddDataItem)ViewItem).NextReserve;
                else if (Data is IBasicPgInfo)              return ReserveItem.GetTimeStringReserveStyle(((IBasicPgInfo)Data).PgStartTime, ((IBasicPgInfo)Data).PgDurationSecond);
                else                                        return "";
            }
        }
        public long StartTimeValue
        {
            get
            {
                if  (ViewItem is AutoAddDataItem)           return ((AutoAddDataItem)ViewItem).NextReserveValue;
                else if (Data is IBasicPgInfo)              return ((IBasicPgInfo)Data).PgStartTime.Ticks;
                else                                        return long.MaxValue;
            }
        }
        public String ProgramDuration
        {
            get
            {
                if      (Data is EpgAutoAddData)            return new ReserveItem(((EpgAutoAddData)Data).GetNextReserve()).ProgramDuration;
                else if (Data is IBasicPgInfo)              return ReserveItem.GetDurationStringReserveStyle(((IBasicPgInfo)Data).PgDurationSecond);
                else                                        return "";
            }
        }
        public UInt32 ProgramDurationValue
        {
            get
            {
                if      (Data is EpgAutoAddData)            return new ReserveItem (((EpgAutoAddData)Data).GetNextReserve()).ProgramDurationValue;
                else if (Data is IBasicPgInfo)              return ((IBasicPgInfo)Data).PgDurationSecond;
                else                                        return UInt32.MaxValue;
            }
        }
        public String NetworkName
        {
            get
            {
                if      (ViewItem is ReserveItem)           return ((ReserveItem)ViewItem).NetworkName;
                else if (ViewItem is RecInfoItem)           return ((RecInfoItem)ViewItem).NetworkName;
                else if (ViewItem is AutoAddDataItem)       return ((AutoAddDataItem)ViewItem).NetworkName;
                else                                        return "";
            }
        }
        public String ServiceName
        {
            get
            {
                if      (ViewItem is ReserveItem)           return ((ReserveItem)ViewItem).ServiceName;
                else if (ViewItem is RecInfoItem)           return ((RecInfoItem)ViewItem).ServiceName;
                else if (ViewItem is AutoAddDataItem)       return ((AutoAddDataItem)ViewItem).ServiceName;
                else                                        return "";
            }
        }
        public String EtcInfo
        {
            get
            {
                string ret = "";
                if      (ViewItem is ReserveItem)           ret = ((ReserveItem)ViewItem).ProgramContent;
                else if (ViewItem is RecInfoItem)           ret = ((RecInfoItem)ViewItem).DropInfoText + " " + ((RecInfoItem)ViewItem).Result;
                else if (ViewItem is EpgAutoDataItem)       ret = ((EpgAutoDataItem)ViewItem).NotKey;
                else if (ViewItem is ManualAutoAddDataItem) ret = ((ManualAutoAddDataItem)ViewItem).StartTimeShort + " " + ((ManualAutoAddDataItem)ViewItem).DayOfWeek;
                ret = ret.Replace("\r\n", " ");//先に長さを確定
                return ret.Substring(0, Math.Min(50, ret.Length));
            }
        }
        public String GetSearchText(bool TitleOnly)
        {
            if (TitleOnly == false)
            {
                if (ViewItem is ReserveItem)
                {
                    //EPGデータ無しの場合SearchItem.ConvertInfoText()は空白になるので手元の情報との合成にする。
                    return ViewItem.ConvertInfoText() + new SearchItem(((ReserveItem)ViewItem).EventInfo).ConvertInfoText();
                }
                else if (ViewItem is RecInfoItem)
                {
                    return ViewItem.ConvertInfoText() + ((RecInfoItem)ViewItem).RecInfo.ProgramInfo;
                }
                else if (ViewItem is AutoAddDataItem)
                {
                    return ViewItem.ConvertInfoText();
                }
            }
            return DataTitle;
        }

        //ツールチップオプション関係
        private static RoutedCommand CmdGetTooltipOption = new RoutedCommand();
        private class TooltipOptionObj { public Func<bool> FuncTooltipOption;}
        private static Func<bool> GetTooltipOptionFunc(IInputElement target = null)
        {
            var param = new TooltipOptionObj();
            CmdGetTooltipOption.Execute(param, target);
            return param.FuncTooltipOption;
        }
        public static void RegisterTooltipOption(Func<bool> funcTooltipOption, UIElement target)
        {
            target.CommandBindings.Add(new CommandBinding(CmdGetTooltipOption, (sender, e) =>
            {
                (e.Parameter as TooltipOptionObj).FuncTooltipOption = funcTooltipOption;
            }));
        }
        private Func<bool> funcTooltipOption = null;
        public override TextBlock ToolTipView
        {
            get
            {
                if (funcTooltipOption == null)
                {
                    funcTooltipOption = GetTooltipOptionFunc();
                }

                if ((funcTooltipOption == null && Settings.Instance.InfoSearchItemTooltip == false)
                     || (funcTooltipOption != null && funcTooltipOption() == false))
                { return null; }

                return ViewItem.ToolTipViewAlways;
            }
        }
        public override Brush ForeColor
        {
            get
            {
                if (ViewItem == null) return base.ForeColor;
                //
                return ViewItem.ForeColor;
            }
        }
        public override Brush BackColor
        {
            get
            {
                if (ViewItem == null) return base.BackColor;
                //
                return ViewItem.BackColor;
            }
        }
        public override Brush BorderBrush
        {
            get
            {
                if (ViewItem == null) return base.BorderBrush;
                //
                return ViewItem.BorderBrush;
            }
        }

    }
}
