using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Reflection;
using System.Globalization;
using System.Xml.Linq;
using System.Runtime.InteropServices;
using System.ComponentModel;
using System.Windows;

namespace EpgTimer
{
    class IniFileHandler
    {
        [DllImport("KERNEL32.DLL", CharSet = CharSet.Unicode)]
        public static extern uint
          GetPrivateProfileString(string lpAppName,
          string lpKeyName, string lpDefault,
          StringBuilder lpReturnedString, uint nSize,
          string lpFileName);

        [DllImport("KERNEL32.DLL", CharSet = CharSet.Unicode)]
        public static extern int
          GetPrivateProfileInt(string lpAppName,
          string lpKeyName, int nDefault, string lpFileName);

        [DllImport("KERNEL32.DLL", CharSet = CharSet.Unicode)]
        public static extern uint WritePrivateProfileString(
          string lpAppName,
          string lpKeyName,
          string lpString,
          string lpFileName);

        public static string
          GetPrivateProfileString(string lpAppName,
          string lpKeyName, string lpDefault, string lpFileName)
        {
            StringBuilder buff = null;
            for (uint n = 512; n <= 1024 * 1024; n *= 2)
            {
                //セクション名取得などのNUL文字分割された結果は先頭要素のみ格納される
                buff = new StringBuilder((int)n);
                if (GetPrivateProfileString(lpAppName, lpKeyName, lpDefault, buff, n, lpFileName) < n - 2)
                {
                    break;
                }
            }
            return buff.ToString();
        }

        public static void TouchFileAsUnicode(string path)
        {
            try
            {
                using (var fs = new FileStream(path, FileMode.CreateNew))
                {
                    fs.Write(new byte[] { 0xFF, 0xFE }, 0, 2);
                }
            }
            catch { }
        }
    }

    class SettingPath
    {
        public static string TimerSrvIniPath
        {
            get
            {
                return Path.Combine(ModulePath, "EpgTimerSrv.ini");
            }
        }

        public static string ModulePath
        {
            get
            {
                return Path.GetDirectoryName(Assembly.GetEntryAssembly().Location);
            }
        }
    }

    public class Settings
    {
        public bool UseCustomEpgView { get; set; }
        public List<CustomEpgTabInfo> CustomEpgTabList { get; set; }
        public double MinHeight { get; set; }
        public double MinimumHeight { get; set; }
        public double ServiceWidth { get; set; }
        public double ScrollSize { get; set; }
        public string FontName { get; set; }
        public double FontSize { get; set; }
        public string FontNameTitle { get; set; }
        public double FontSizeTitle { get; set; }
        public bool FontBoldTitle { get; set; }
        public bool NoToolTip { get; set; }
        public bool NoBallonTips { get; set; }
        public int ForceHideBalloonTipSec { get; set; }
        public bool PlayDClick { get; set; }
        public bool ConfirmDelRecInfo { get; set; }
        public bool ConfirmDelRecInfoAlways { get; set; }
        public bool ShowEpgCapServiceOnly { get; set; }
        public bool SortServiceList { get; set; }
        public bool ExitAfterProcessingArgs { get; set; }
        public double DragScroll { get; set; }
        public List<string> ContentColorList { get; set; }
        public List<UInt32> ContentCustColorList { get; set; }
        public List<string> TimeColorList { get; set; }
        public List<UInt32> TimeCustColorList { get; set; }
        public string ReserveRectColorNormal { get; set; }
        public string ReserveRectColorNo { get; set; }
        public string ReserveRectColorNoTuner { get; set; }
        public string ReserveRectColorWarning { get; set; }
        public int ReserveRectFillOpacity { get; set; }
        public bool ReserveRectFillWithShadow { get; set; }
        public string TitleColor1 { get; set; }
        public string TitleColor2 { get; set; }
        public UInt32 TitleCustColor1 { get; set; }
        public UInt32 TitleCustColor2 { get; set; }
        public string ServiceColor { get; set; }
        public UInt32 ServiceCustColor { get; set; }
        public bool EpgToolTip { get; set; }
        public double EpgBorderLeftSize { get; set; }
        public double EpgBorderTopSize { get; set; }
        public bool EpgTitleIndent { get; set; }
        public string EpgReplacePattern { get; set; }
        public string EpgReplacePatternTitle { get; set; }
        public bool EpgToolTipNoViewOnly { get; set; }
        public int EpgToolTipViewWait { get; set; }
        public bool EpgPopup { get; set; }
        public bool EpgExtInfoPopup { get; set; }
        public bool EpgGradation { get; set; }
        public bool EpgGradationHeader { get; set; }
        public string ResColumnHead { get; set; }
        public ListSortDirection ResSortDirection { get; set; }
        public bool ResHideButton { get; set; }
        public System.Windows.WindowState LastWindowState { get; set; }
        public double MainWndLeft { get; set; }
        public double MainWndTop { get; set; }
        public double MainWndWidth { get; set; }
        public double MainWndHeight { get; set; }
        public double SearchWndTabsHeight { get; set; }
        public bool CloseMin { get; set; }
        public bool WakeMin { get; set; }
        public bool ViewButtonShowAsTab { get; set; }
        public List<string> ViewButtonList { get; set; }
        public List<string> TaskMenuList { get; set; }
        public string Cust1BtnName { get; set; }
        public string Cust1BtnCmd { get; set; }
        public string Cust1BtnCmdOpt { get; set; }
        public string Cust2BtnName { get; set; }
        public string Cust2BtnCmd { get; set; }
        public string Cust2BtnCmdOpt { get; set; }
        public List<string> AndKeyList { get; set; }
        public List<string> NotKeyList { get; set; }
        public string SearchKeyAndKey { get; set; }
        public string SearchKeyNotKey { get; set; }
        public bool SearchKeyRegExp { get; set; }
        public bool SearchKeyTitleOnly { get; set; }
        public bool SearchKeyAimaiFlag { get; set; }
        public bool SearchKeyNotContent { get; set; }
        public bool SearchKeyNotDate { get; set; }
        public List<ContentKindInfo> SearchKeyContentList { get; set; }
        public List<DateItem> SearchKeyDateItemList { get; set; }
        public List<Int64> SearchKeyServiceList { get; set; }
        public byte SearchKeyFreeCA { get; set; }
        public byte SearchKeyChkRecEnd { get; set; }
        public UInt16 SearchKeyChkRecDay { get; set; }
        public string RecInfoColumnHead { get; set; }
        public ListSortDirection RecInfoSortDirection { get; set; }
        public bool RecInfoHideButton { get; set; }
        public string TvTestExe { get; set; }
        public string TvTestCmd { get; set; }
        public bool NwTvMode { get; set; }
        public bool NwTvModeUDP { get; set; }
        public bool NwTvModeTCP { get; set; }
        public bool FilePlay { get; set; }
        public string FilePlayExe { get; set; }
        public string FilePlayCmd { get; set; }
        public bool FilePlayOnAirWithExe { get; set; }
        public List<IEPGStationInfo> IEpgStationList { get; set; }
        public string NWServerIP { get; set; }
        public UInt32 NWServerPort { get; set; }
        public UInt32 NWWaitPort { get; set; }
        public string NWMacAdd { get; set; }
        public bool WakeReconnectNW { get; set; }
        public bool SuspendCloseNW { get; set; }
        public bool NgAutoEpgLoadNW { get; set; }
        public bool PrebuildEpg { get; set; }
        public Int32 TvTestOpenWait { get; set; }
        public Int32 TvTestChgBonWait { get; set; }
        public byte ResDefColorA { get; set; }
        public byte ResDefColorR { get; set; }
        public byte ResDefColorG { get; set; }
        public byte ResDefColorB { get; set; }
        public byte ResErrColorA { get; set; }
        public byte ResErrColorR { get; set; }
        public byte ResErrColorG { get; set; }
        public byte ResErrColorB { get; set; }
        public byte ResWarColorA { get; set; }
        public byte ResWarColorR { get; set; }
        public byte ResWarColorG { get; set; }
        public byte ResWarColorB { get; set; }
        public byte ResNoColorA { get; set; }
        public byte ResNoColorR { get; set; }
        public byte ResNoColorG { get; set; }
        public byte ResNoColorB { get; set; }
        public byte RecEndDefColorA { get; set; }
        public byte RecEndDefColorR { get; set; }
        public byte RecEndDefColorG { get; set; }
        public byte RecEndDefColorB { get; set; }
        public byte RecEndErrColorA { get; set; }
        public byte RecEndErrColorR { get; set; }
        public byte RecEndErrColorG { get; set; }
        public byte RecEndErrColorB { get; set; }
        public byte RecEndWarColorA { get; set; }
        public byte RecEndWarColorR { get; set; }
        public byte RecEndWarColorG { get; set; }
        public byte RecEndWarColorB { get; set; }
        public byte EpgTipsBackColorR { get; set; }
        public byte EpgTipsBackColorG { get; set; }
        public byte EpgTipsBackColorB { get; set; }
        public byte EpgTipsForeColorR { get; set; }
        public byte EpgTipsForeColorG { get; set; }
        public byte EpgTipsForeColorB { get; set; }
        public byte EpgBackColorR { get; set; }
        public byte EpgBackColorG { get; set; }
        public byte EpgBackColorB { get; set; }
        public bool EpgInfoSingleClick { get; set; }
        public byte EpgInfoOpenMode { get; set; }
        public UInt32 ExecBat { get; set; }
        public UInt32 SuspendChk { get; set; }
        public List<ListColumnInfo> ReserveListColumn { get; set; }
        public List<ListColumnInfo> RecInfoListColumn { get; set; }
        public List<ListColumnInfo> AutoAddEpgColumn { get; set; }
        public List<ListColumnInfo> AutoAddManualColumn { get; set; }
        public double SearchWndWidth { get; set; }
        public double SearchWndHeight { get; set; }
        public int NotifyLogMax { get; set; }
        public bool ShowTray { get; set; }
        public bool MinHide { get; set; }
        public bool MouseScrollAuto { get; set; }
        public int NoStyle { get; set; }
        public int NoSendClose { get; set; }
        public string StartTab { get; set; }

        public Settings()
        {
            //既定値をセットするため
            ConvertXElem(null, false);
        }

        private Settings(XElement x)
        {
            ConvertXElem(x, false);
        }

        private void ConvertXElem(XElement x, bool w)
        {
            //XElementに書き出すとき戻り値は捨てる
            Settings r = w ? new Settings() : this;
            //絶対値がdoubleの有効桁数(52bit)をこえる整数を誤差なく扱いたい場合はConvertXElem()などにオーバーロードの追加が必要
            r.UseCustomEpgView          = ConvertXElem(x, w, "UseCustomEpgView", UseCustomEpgView, false);
            r.CustomEpgTabList          = ConvertXElements(x, w, "CustomEpgTabList", CustomEpgTabList).ToList();
            r.MinHeight                 = ConvertXElem(x, w, "MinHeight", MinHeight, 2);
            r.MinimumHeight             = ConvertXElem(x, w, "MinimumHeight", MinimumHeight, 0.6);
            r.ServiceWidth              = ConvertXElem(x, w, "ServiceWidth", ServiceWidth, 150);
            r.ScrollSize                = ConvertXElem(x, w, "ScrollSize", ScrollSize, 240);
            r.FontName                  = ConvertXElem(x, w, "FontName", FontName, "メイリオ");
            r.FontSize                  = ConvertXElem(x, w, "FontSize", FontSize, 12);
            r.FontNameTitle             = ConvertXElem(x, w, "FontNameTitle", FontNameTitle, "メイリオ");
            r.FontSizeTitle             = ConvertXElem(x, w, "FontSizeTitle", FontSizeTitle, 12);
            r.FontBoldTitle             = ConvertXElem(x, w, "FontBoldTitle", FontBoldTitle, true);
            r.NoToolTip                 = ConvertXElem(x, w, "NoToolTip", NoToolTip, false);
            r.NoBallonTips              = ConvertXElem(x, w, "NoBallonTips", NoBallonTips, false);
            r.ForceHideBalloonTipSec    = (int)ConvertXElem(x, w, "ForceHideBalloonTipSec", ForceHideBalloonTipSec, 0);
            r.PlayDClick                = ConvertXElem(x, w, "PlayDClick", PlayDClick, false);
            r.ConfirmDelRecInfo         = ConvertXElem(x, w, "ConfirmDelRecInfo", ConfirmDelRecInfo, true);
            r.ConfirmDelRecInfoAlways   = ConvertXElem(x, w, "ConfirmDelRecInfoAlways", ConfirmDelRecInfoAlways, false);
            r.ShowEpgCapServiceOnly     = ConvertXElem(x, w, "ShowEpgCapServiceOnly", ShowEpgCapServiceOnly, false);
            r.SortServiceList           = ConvertXElem(x, w, "SortServiceList", SortServiceList, true);
            r.ExitAfterProcessingArgs   = ConvertXElem(x, w, "ExitAfterProcessingArgs", ExitAfterProcessingArgs, false);
            r.DragScroll                = ConvertXElem(x, w, "DragScroll", DragScroll, 1.5);
            r.ContentColorList          = ConvertXElements(x, w, "ContentColorList", ContentColorList).ToList();
            r.ContentCustColorList      = ConvertXElements(x, w, "ContentCustColorList",
                (ContentCustColorList ?? Enumerable.Empty<uint>()).Select(a => (double)a), "unsignedInt").Select(a => (uint)a).ToList();
            r.TimeColorList             = ConvertXElements(x, w, "TimeColorList", TimeColorList).ToList();
            r.TimeCustColorList         = ConvertXElements(x, w, "TimeCustColorList",
                (TimeCustColorList ?? Enumerable.Empty<uint>()).Select(a => (double)a), "unsignedInt").Select(a => (uint)a).ToList();
            r.ReserveRectColorNormal    = ConvertXElem(x, w, "ReserveRectColorNormal", ReserveRectColorNormal, "Lime");
            r.ReserveRectColorNo        = ConvertXElem(x, w, "ReserveRectColorNo", ReserveRectColorNo, "Black");
            r.ReserveRectColorNoTuner   = ConvertXElem(x, w, "ReserveRectColorNoTuner", ReserveRectColorNoTuner, "Red");
            r.ReserveRectColorWarning   = ConvertXElem(x, w, "ReserveRectColorWarning", ReserveRectColorWarning, "Yellow");
            r.ReserveRectFillOpacity    = (int)ConvertXElem(x, w, "ReserveRectFillOpacity", ReserveRectFillOpacity, 0);
            r.ReserveRectFillWithShadow = ConvertXElem(x, w, "ReserveRectFillWithShadow", ReserveRectFillWithShadow, true);
            r.TitleColor1               = ConvertXElem(x, w, "TitleColor1", TitleColor1, "Black");
            r.TitleColor2               = ConvertXElem(x, w, "TitleColor2", TitleColor2, "Black");
            r.TitleCustColor1           = (uint)ConvertXElem(x, w, "TitleCustColor1", TitleCustColor1, 0xFFFFFFFF);
            r.TitleCustColor2           = (uint)ConvertXElem(x, w, "TitleCustColor2", TitleCustColor2, 0xFFFFFFFF);
            r.ServiceColor              = ConvertXElem(x, w, "ServiceColor", ServiceColor, "LightSlateGray");
            r.ServiceCustColor          = (uint)ConvertXElem(x, w, "ServiceCustColor", ServiceCustColor, 0xFFFFFFFF);
            r.EpgToolTip                = ConvertXElem(x, w, "EpgToolTip", EpgToolTip, false);
            r.EpgBorderLeftSize         = ConvertXElem(x, w, "EpgBorderLeftSize", EpgBorderLeftSize, 2);
            r.EpgBorderTopSize          = ConvertXElem(x, w, "EpgBorderTopSize", EpgBorderTopSize, 0.5);
            r.EpgTitleIndent            = ConvertXElem(x, w, "EpgTitleIndent", EpgTitleIndent, true);
            r.EpgReplacePattern         = ConvertXElem(x, w, "EpgReplacePattern", EpgReplacePattern, "");
            r.EpgReplacePatternTitle    = ConvertXElem(x, w, "EpgReplacePatternTitle", EpgReplacePatternTitle, "");
            r.EpgToolTipNoViewOnly      = ConvertXElem(x, w, "EpgToolTipNoViewOnly", EpgToolTipNoViewOnly, true);
            r.EpgToolTipViewWait        = (int)ConvertXElem(x, w, "EpgToolTipViewWait", EpgToolTipViewWait, 1500);
            r.EpgPopup                  = ConvertXElem(x, w, "EpgPopup", EpgPopup, true);
            r.EpgExtInfoPopup           = ConvertXElem(x, w, "EpgExtInfoPopup", EpgExtInfoPopup, true);
            r.EpgGradation              = ConvertXElem(x, w, "EpgGradation", EpgGradation, true);
            r.EpgGradationHeader        = ConvertXElem(x, w, "EpgGradationHeader", EpgGradationHeader, true);
            r.ResColumnHead             = ConvertXElem(x, w, "ResColumnHead", ResColumnHead, "");
            ListSortDirection sd;
            Enum.TryParse(ConvertXElem(x, w, "ResSortDirection", ResSortDirection.ToString(), ""), out sd);
            r.ResSortDirection          = sd;
            r.ResHideButton             = ConvertXElem(x, w, "ResHideButton", ResHideButton, false);
            WindowState ws;
            Enum.TryParse(ConvertXElem(x, w, "LastWindowState", LastWindowState.ToString(), ""), out ws);
            r.LastWindowState           = ws;
            r.MainWndLeft               = ConvertXElem(x, w, "MainWndLeft", MainWndLeft, -100);
            r.MainWndTop                = ConvertXElem(x, w, "MainWndTop", MainWndTop, -100);
            r.MainWndWidth              = ConvertXElem(x, w, "MainWndWidth", MainWndWidth, -100);
            r.MainWndHeight             = ConvertXElem(x, w, "MainWndHeight", MainWndHeight, -100);
            r.SearchWndTabsHeight       = ConvertXElem(x, w, "SearchWndTabsHeight", SearchWndTabsHeight, 0);
            r.CloseMin                  = ConvertXElem(x, w, "CloseMin", CloseMin, false);
            r.WakeMin                   = ConvertXElem(x, w, "WakeMin", WakeMin, false);
            r.ViewButtonShowAsTab       = ConvertXElem(x, w, "ViewButtonShowAsTab", ViewButtonShowAsTab, false);
            r.ViewButtonList            = ConvertXElements(x, w, "ViewButtonList", ViewButtonList).ToList();
            r.TaskMenuList              = ConvertXElements(x, w, "TaskMenuList", TaskMenuList).ToList();
            r.Cust1BtnName              = ConvertXElem(x, w, "Cust1BtnName", Cust1BtnName, "");
            r.Cust1BtnCmd               = ConvertXElem(x, w, "Cust1BtnCmd", Cust1BtnCmd, "");
            r.Cust1BtnCmdOpt            = ConvertXElem(x, w, "Cust1BtnCmdOpt", Cust1BtnCmdOpt, "");
            r.Cust2BtnName              = ConvertXElem(x, w, "Cust2BtnName", Cust2BtnName, "");
            r.Cust2BtnCmd               = ConvertXElem(x, w, "Cust2BtnCmd", Cust2BtnCmd, "");
            r.Cust2BtnCmdOpt            = ConvertXElem(x, w, "Cust2BtnCmdOpt", Cust2BtnCmdOpt, "");
            r.AndKeyList                = ConvertXElements(x, w, "AndKeyList", AndKeyList).ToList();
            r.NotKeyList                = ConvertXElements(x, w, "NotKeyList", NotKeyList).ToList();
            r.SearchKeyAndKey           = ConvertXElem(x, w, "SearchKeyAndKey", SearchKeyAndKey, "");
            r.SearchKeyNotKey           = ConvertXElem(x, w, "SearchKeyNotKey", SearchKeyNotKey, "");
            r.SearchKeyRegExp           = ConvertXElem(x, w, "SearchKeyRegExp", SearchKeyRegExp, false);
            r.SearchKeyTitleOnly        = ConvertXElem(x, w, "SearchKeyTitleOnly", SearchKeyTitleOnly, false);
            r.SearchKeyAimaiFlag        = ConvertXElem(x, w, "SearchKeyAimaiFlag", SearchKeyAimaiFlag, false);
            r.SearchKeyNotContent       = ConvertXElem(x, w, "SearchKeyNotContent", SearchKeyNotContent, false);
            r.SearchKeyNotDate          = ConvertXElem(x, w, "SearchKeyNotDate", SearchKeyNotDate, false);
            r.SearchKeyContentList      = ConvertXElements(x, w, "SearchKeyContentList", SearchKeyContentList).ToList();
            r.SearchKeyDateItemList     = ConvertXElements(x, w, "SearchKeyDateItemList", SearchKeyDateItemList).ToList();
            r.SearchKeyServiceList      = ConvertXElements(x, w, "SearchKeyServiceList",
                (SearchKeyServiceList ?? Enumerable.Empty<long>()).Select(a => (double)a), "long").Select(a => (long)a).ToList();
            r.SearchKeyFreeCA           = (byte)ConvertXElem(x, w, "SearchKeyFreeCA", SearchKeyFreeCA, 0);
            r.SearchKeyChkRecEnd        = (byte)ConvertXElem(x, w, "SearchKeyChkRecEnd", SearchKeyChkRecEnd, 0);
            r.SearchKeyChkRecDay        = (ushort)ConvertXElem(x, w, "SearchKeyChkRecDay", SearchKeyChkRecDay, 6);
            r.RecInfoColumnHead         = ConvertXElem(x, w, "RecInfoColumnHead", RecInfoColumnHead, "");
            Enum.TryParse(ConvertXElem(x, w, "RecInfoSortDirection", RecInfoSortDirection.ToString(), ""), out sd);
            r.RecInfoSortDirection      = sd;
            r.RecInfoHideButton         = ConvertXElem(x, w, "RecInfoHideButton", RecInfoHideButton, false);
            r.TvTestExe                 = ConvertXElem(x, w, "TvTestExe", TvTestExe, "");
            r.TvTestCmd                 = ConvertXElem(x, w, "TvTestCmd", TvTestCmd, "");
            r.NwTvMode                  = ConvertXElem(x, w, "NwTvMode", NwTvMode, false);
            r.NwTvModeUDP               = ConvertXElem(x, w, "NwTvModeUDP", NwTvModeUDP, false);
            r.NwTvModeTCP               = ConvertXElem(x, w, "NwTvModeTCP", NwTvModeTCP, false);
            r.FilePlay                  = ConvertXElem(x, w, "FilePlay", FilePlay, true);
            r.FilePlayExe               = ConvertXElem(x, w, "FilePlayExe", FilePlayExe, "");
            r.FilePlayCmd               = ConvertXElem(x, w, "FilePlayCmd", FilePlayCmd, "\"$FilePath$\"");
            r.FilePlayOnAirWithExe      = ConvertXElem(x, w, "FilePlayOnAirWithExe", FilePlayOnAirWithExe, true);
            r.IEpgStationList           = ConvertXElements(x, w, "IEpgStationList", IEpgStationList).ToList();
            r.NWServerIP                = ConvertXElem(x, w, "NWServerIP", NWServerIP, "");
            r.NWServerPort              = (uint)ConvertXElem(x, w, "NWServerPort", NWServerPort, 4510);
            r.NWWaitPort                = (uint)ConvertXElem(x, w, "NWWaitPort", NWWaitPort, 0);
            r.NWMacAdd                  = ConvertXElem(x, w, "NWMacAdd", NWMacAdd, "");
            r.WakeReconnectNW           = ConvertXElem(x, w, "WakeReconnectNW", WakeReconnectNW, false);
            r.SuspendCloseNW            = ConvertXElem(x, w, "SuspendCloseNW", SuspendCloseNW, false);
            r.NgAutoEpgLoadNW           = ConvertXElem(x, w, "NgAutoEpgLoadNW", NgAutoEpgLoadNW, false);
            r.PrebuildEpg               = ConvertXElem(x, w, "PrebuildEpg", PrebuildEpg, false);
            r.TvTestOpenWait            = (int)ConvertXElem(x, w, "TvTestOpenWait", TvTestOpenWait, 2000);
            r.TvTestChgBonWait          = (int)ConvertXElem(x, w, "TvTestChgBonWait", TvTestChgBonWait, 2000);
            r.ResDefColorA              = (byte)ConvertXElem(x, w, "ResDefColorA", ResDefColorA, 0);
            r.ResDefColorR              = (byte)ConvertXElem(x, w, "ResDefColorR", ResDefColorR, 0xFF);
            r.ResDefColorG              = (byte)ConvertXElem(x, w, "ResDefColorG", ResDefColorG, 0xFF);
            r.ResDefColorB              = (byte)ConvertXElem(x, w, "ResDefColorB", ResDefColorB, 0xFF);
            r.ResErrColorA              = (byte)ConvertXElem(x, w, "ResErrColorA", ResErrColorA, 0x80);
            r.ResErrColorR              = (byte)ConvertXElem(x, w, "ResErrColorR", ResErrColorR, 0xFF);
            r.ResErrColorG              = (byte)ConvertXElem(x, w, "ResErrColorG", ResErrColorG, 0);
            r.ResErrColorB              = (byte)ConvertXElem(x, w, "ResErrColorB", ResErrColorB, 0);
            r.ResWarColorA              = (byte)ConvertXElem(x, w, "ResWarColorA", ResWarColorA, 0x80);
            r.ResWarColorR              = (byte)ConvertXElem(x, w, "ResWarColorR", ResWarColorR, 0xFF);
            r.ResWarColorG              = (byte)ConvertXElem(x, w, "ResWarColorG", ResWarColorG, 0xFF);
            r.ResWarColorB              = (byte)ConvertXElem(x, w, "ResWarColorB", ResWarColorB, 0);
            r.ResNoColorA               = (byte)ConvertXElem(x, w, "ResNoColorA", ResNoColorA, 0x80);
            r.ResNoColorR               = (byte)ConvertXElem(x, w, "ResNoColorR", ResNoColorR, 0xA9);
            r.ResNoColorG               = (byte)ConvertXElem(x, w, "ResNoColorG", ResNoColorG, 0xA9);
            r.ResNoColorB               = (byte)ConvertXElem(x, w, "ResNoColorB", ResNoColorB, 0xA9);
            r.RecEndDefColorA           = (byte)ConvertXElem(x, w, "RecEndDefColorA", RecEndDefColorA, 0);
            r.RecEndDefColorR           = (byte)ConvertXElem(x, w, "RecEndDefColorR", RecEndDefColorR, 0xFF);
            r.RecEndDefColorG           = (byte)ConvertXElem(x, w, "RecEndDefColorG", RecEndDefColorG, 0xFF);
            r.RecEndDefColorB           = (byte)ConvertXElem(x, w, "RecEndDefColorB", RecEndDefColorB, 0xFF);
            r.RecEndErrColorA           = (byte)ConvertXElem(x, w, "RecEndErrColorA", RecEndErrColorA, 0x80);
            r.RecEndErrColorR           = (byte)ConvertXElem(x, w, "RecEndErrColorR", RecEndErrColorR, 0xFF);
            r.RecEndErrColorG           = (byte)ConvertXElem(x, w, "RecEndErrColorG", RecEndErrColorG, 0);
            r.RecEndErrColorB           = (byte)ConvertXElem(x, w, "RecEndErrColorB", RecEndErrColorB, 0);
            r.RecEndWarColorA           = (byte)ConvertXElem(x, w, "RecEndWarColorA", RecEndWarColorA, 0x80);
            r.RecEndWarColorR           = (byte)ConvertXElem(x, w, "RecEndWarColorR", RecEndWarColorR, 0xFF);
            r.RecEndWarColorG           = (byte)ConvertXElem(x, w, "RecEndWarColorG", RecEndWarColorG, 0xFF);
            r.RecEndWarColorB           = (byte)ConvertXElem(x, w, "RecEndWarColorB", RecEndWarColorB, 0);
            r.EpgTipsBackColorR         = (byte)ConvertXElem(x, w, "EpgTipsBackColorR", EpgTipsBackColorR, 0xD3);
            r.EpgTipsBackColorG         = (byte)ConvertXElem(x, w, "EpgTipsBackColorG", EpgTipsBackColorG, 0xD3);
            r.EpgTipsBackColorB         = (byte)ConvertXElem(x, w, "EpgTipsBackColorB", EpgTipsBackColorB, 0xD3);
            r.EpgTipsForeColorR         = (byte)ConvertXElem(x, w, "EpgTipsForeColorR", EpgTipsForeColorR, 0);
            r.EpgTipsForeColorG         = (byte)ConvertXElem(x, w, "EpgTipsForeColorG", EpgTipsForeColorG, 0);
            r.EpgTipsForeColorB         = (byte)ConvertXElem(x, w, "EpgTipsForeColorB", EpgTipsForeColorB, 0);
            r.EpgBackColorR             = (byte)ConvertXElem(x, w, "EpgBackColorR", EpgBackColorR, 0xA9);
            r.EpgBackColorG             = (byte)ConvertXElem(x, w, "EpgBackColorG", EpgBackColorG, 0xA9);
            r.EpgBackColorB             = (byte)ConvertXElem(x, w, "EpgBackColorB", EpgBackColorB, 0xA9);
            r.EpgInfoSingleClick        = ConvertXElem(x, w, "EpgInfoSingleClick", EpgInfoSingleClick, false);
            r.EpgInfoOpenMode           = (byte)ConvertXElem(x, w, "EpgInfoOpenMode", EpgInfoOpenMode, 0);
            r.ExecBat                   = (uint)ConvertXElem(x, w, "ExecBat", ExecBat, 0);
            r.SuspendChk                = (uint)ConvertXElem(x, w, "SuspendChk", SuspendChk, 0);
            r.ReserveListColumn         = ConvertXElements(x, w, "ReserveListColumn", ReserveListColumn).ToList();
            r.RecInfoListColumn         = ConvertXElements(x, w, "RecInfoListColumn", RecInfoListColumn).ToList();
            r.AutoAddEpgColumn          = ConvertXElements(x, w, "AutoAddEpgColumn", AutoAddEpgColumn).ToList();
            r.AutoAddManualColumn       = ConvertXElements(x, w, "AutoAddManualColumn", AutoAddManualColumn).ToList();
            r.SearchWndWidth            = ConvertXElem(x, w, "SearchWndWidth", SearchWndWidth, 0);
            r.SearchWndHeight           = ConvertXElem(x, w, "SearchWndHeight", SearchWndHeight, 0);
            r.NotifyLogMax              = (int)ConvertXElem(x, w, "NotifyLogMax", NotifyLogMax, 100);
            r.ShowTray                  = ConvertXElem(x, w, "ShowTray", ShowTray, true);
            r.MinHide                   = ConvertXElem(x, w, "MinHide", MinHide, true);
            r.MouseScrollAuto           = ConvertXElem(x, w, "MouseScrollAuto", MouseScrollAuto, false);
            r.NoStyle                   = (int)ConvertXElem(x, w, "NoStyle", NoStyle, 1);
            r.NoSendClose               = (int)ConvertXElem(x, w, "NoSendClose", NoSendClose, 0);
            r.StartTab                  = ConvertXElem(x, w, "StartTab", StartTab, "ReserveView");
        }

        public Settings DeepCloneStaticSettings()
        {
            var other = (Settings)MemberwiseClone();
            other.CustomEpgTabList = CustomEpgTabList.Select(a => a.DeepClone()).ToList();
            other.ContentColorList = ContentColorList.ToList();
            other.ContentCustColorList = ContentCustColorList.ToList();
            other.TimeColorList = TimeColorList.ToList();
            other.TimeCustColorList = TimeCustColorList.ToList();
            other.ViewButtonList = ViewButtonList.ToList();
            other.TaskMenuList = TaskMenuList.ToList();
            other.SearchKeyContentList = SearchKeyContentList.Select(a => a.DeepClone()).ToList();
            other.SearchKeyDateItemList = SearchKeyDateItemList.Select(a => a.DeepClone()).ToList();
            other.SearchKeyServiceList = SearchKeyServiceList.ToList();
            other.IEpgStationList = IEpgStationList.Select(a => a.DeepClone()).ToList();
            return other;
        }

        public void ShallowCopyDynamicSettingsTo(Settings dest)
        {
            //設定画面と関係なくその場で動的に更新されるプロパティ
            dest.ResColumnHead = ResColumnHead;
            dest.ResSortDirection = ResSortDirection;
            dest.ResHideButton = ResHideButton;
            dest.LastWindowState = LastWindowState;
            dest.MainWndLeft = MainWndLeft;
            dest.MainWndTop = MainWndTop;
            dest.MainWndWidth = MainWndWidth;
            dest.MainWndHeight = MainWndHeight;
            dest.SearchWndTabsHeight = SearchWndTabsHeight;
            dest.AndKeyList = AndKeyList;
            dest.NotKeyList = NotKeyList;
            dest.RecInfoColumnHead = RecInfoColumnHead;
            dest.RecInfoSortDirection = RecInfoSortDirection;
            dest.RecInfoHideButton = RecInfoHideButton;
            dest.NWServerIP = NWServerIP;
            dest.NWServerPort = NWServerPort;
            dest.NWWaitPort = NWWaitPort;
            dest.NWMacAdd = NWMacAdd;
            dest.ReserveListColumn = ReserveListColumn;
            dest.RecInfoListColumn = RecInfoListColumn;
            dest.AutoAddEpgColumn = AutoAddEpgColumn;
            dest.AutoAddManualColumn = AutoAddManualColumn;
            dest.SearchWndWidth = SearchWndWidth;
            dest.SearchWndHeight = SearchWndHeight;
            dest.NotifyLogMax = NotifyLogMax;
        }

        private static Settings _instance;
        public static Settings Instance
        {
            get
            {
                if (_instance == null)
                {
                    _instance = new Settings();
                    //色設定関係
                    _instance.SetColorSetting();
                }
                return _instance;
            }
            set { _instance = value; }
        }

        /// <summary>
        /// 設定ファイルロード関数
        /// </summary>
        public static void LoadFromXmlFile(bool nwMode)
        {
            for (int retry = 0; ;)
            {
                try
                {
                    using (var fs = new FileStream(GetSettingPath(), FileMode.Open, FileAccess.Read))
                    {
                        //読み込んで逆シリアル化する
                        //var xs = new System.Xml.Serialization.XmlSerializer(typeof(Settings));
                        //Instance = (Settings)xs.Deserialize(fs);
                        //XmlSerializerがとても遅いので自力でやる(処理時間1/3ほど)
                        XElement x = XDocument.Load(fs).Element("Settings");
                        if (x != null)
                        {
                            Instance = new Settings(x);
                        }
                    }
                    break;
                }
                catch (IOException)
                {
                    //FileNotFoundExceptionを含むので注意(File.Replace()の内部でNotFoundになる瞬間がある)
                    if (++retry > 4)
                    {
                        break;
                    }
                    System.Threading.Thread.Sleep(200 * retry);
                }
                catch
                {
                    //内容が異常など
                    break;
                }
            }

            //色設定関係
            Instance.SetColorSetting();

            if (Instance.ViewButtonList.Count == 0)
            {
                if (nwMode)
                {
                    Instance.ViewButtonList = new List<string>
                    {
                        "設定", "（空白）", "再接続", "（空白）", "検索", "（空白）", "EPG取得", "（空白）", "EPG再読み込み", "（空白）", "終了"
                    };
                }
                else
                {
                    Instance.ViewButtonList = new List<string>
                    {
                        "設定", "（空白）", "検索", "（空白）", "スタンバイ", "休止", "（空白）", "EPG取得", "（空白）", "EPG再読み込み", "（空白）", "終了"
                    };
                }
            }
            if (Instance.TaskMenuList.Count == 0)
            {
                if (nwMode)
                {
                    Instance.TaskMenuList = new List<string> { "設定", "（セパレータ）", "再接続", "（セパレータ）", "終了" };
                }
                else
                {
                    Instance.TaskMenuList = new List<string> { "設定", "（セパレータ）", "スタンバイ", "休止", "（セパレータ）", "終了" };
                }
            }
            if (Instance.ReserveListColumn.Count == 0)
            {
                Instance.ReserveListColumn = new List<ListColumnInfo>
                {
                    new ListColumnInfo("StartTime", double.NaN),
                    new ListColumnInfo("NetworkName", double.NaN),
                    new ListColumnInfo("ServiceName", double.NaN),
                    new ListColumnInfo("EventName", double.NaN),
                    new ListColumnInfo("RecMode", double.NaN),
                    new ListColumnInfo("Priority", double.NaN),
                    new ListColumnInfo("Tuijyu", double.NaN),
                    new ListColumnInfo("Comment", double.NaN),
                    new ListColumnInfo("RecFileName", double.NaN)
                };
            }
            if (Instance.RecInfoListColumn.Count == 0)
            {
                Instance.RecInfoListColumn = new List<ListColumnInfo>
                {
                    new ListColumnInfo("IsProtect", double.NaN),
                    new ListColumnInfo("StartTime", double.NaN),
                    new ListColumnInfo("NetworkName", double.NaN),
                    new ListColumnInfo("ServiceName", double.NaN),
                    new ListColumnInfo("EventName", double.NaN),
                    new ListColumnInfo("Drops", double.NaN),
                    new ListColumnInfo("Scrambles", double.NaN),
                    new ListColumnInfo("Result", double.NaN),
                    new ListColumnInfo("RecFilePath", double.NaN)
                };
            }
            if (Instance.AutoAddEpgColumn.Count == 0)
            {
                Instance.AutoAddEpgColumn = new List<ListColumnInfo>
                {
                    new ListColumnInfo("AndKey", double.NaN),
                    new ListColumnInfo("NotKey", double.NaN),
                    new ListColumnInfo("RegExp", double.NaN),
                    new ListColumnInfo("RecMode", double.NaN),
                    new ListColumnInfo("Priority", double.NaN),
                    new ListColumnInfo("Tuijyu", double.NaN)
                };
            }
            if (Instance.AutoAddManualColumn.Count == 0)
            {
                Instance.AutoAddManualColumn = new List<ListColumnInfo>
                {
                    new ListColumnInfo("DayOfWeek", double.NaN),
                    new ListColumnInfo("Time", double.NaN),
                    new ListColumnInfo("Title", double.NaN),
                    new ListColumnInfo("StationName", double.NaN),
                    new ListColumnInfo("RecMode", double.NaN),
                    new ListColumnInfo("Priority", double.NaN)
                };
            }
        }

        /// <summary>
        /// 設定ファイルセーブ関数(プロセス間で排他すること)
        /// </summary>
        public static void SaveToXmlFile(bool notifyException = false)
        {
            try
            {
                string path = GetSettingPath();

                try
                {
                    //所有者などが特殊なときFile.Replace()に失敗することがあるため
                    File.Delete(path + ".back");
                }
                catch { }

                using (var fs = new FileStream(path + ".back", FileMode.Create, FileAccess.Write, FileShare.None))
                {
                    //シリアル化して書き込む
                    //var xs = new System.Xml.Serialization.XmlSerializer(typeof(Settings));
                    //xs.Serialize(fs, Instance);
                    var x = new XElement("Settings");
                    Instance.ConvertXElem(x, true);
                    (new XDocument(x)).Save(fs);
                }
                for (int retry = 0; ;)
                {
                    try
                    {
                        File.Replace(path + ".back", path, null);
                        break;
                    }
                    catch (FileNotFoundException)
                    {
                        File.Move(path + ".back", path);
                        break;
                    }
                    catch (IOException)
                    {
                        if (++retry > 4)
                        {
                            throw;
                        }
                        System.Threading.Thread.Sleep(200 * retry);
                    }
                }
            }
            catch (Exception ex)
            {
                if (notifyException)
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }
            }
        }

        private static string GetSettingPath()
        {
            Assembly myAssembly = Assembly.GetEntryAssembly();
            string path = myAssembly.Location + ".xml";

            return path;
        }

        public static List<RecPresetItem> GetRecPresetList()
        {
            var list = new List<RecPresetItem>();
            list.Add(new RecPresetItem());
            list[0].DisplayName = "デフォルト";
            list[0].ID = 0;
            foreach (string s in IniFileHandler.GetPrivateProfileString("SET", "PresetID", "", SettingPath.TimerSrvIniPath).Split(','))
            {
                uint id;
                uint.TryParse(s, out id);
                if (list.Exists(p => p.ID == id) == false)
                {
                    list.Add(new RecPresetItem());
                    list.Last().DisplayName = IniFileHandler.GetPrivateProfileString("REC_DEF" + id, "SetName", "", SettingPath.TimerSrvIniPath);
                    list.Last().ID = id;
                }
            }
            return list;
        }

        public static void GetDefRecSetting(UInt32 presetID, ref RecSettingData defKey)
        {
            String defName = "REC_DEF";
            String defFolderName = "REC_DEF_FOLDER";
            String defFolder1SegName = "REC_DEF_FOLDER_1SEG";

            if (presetID > 0)
            {
                defName += presetID.ToString();
                defFolderName += presetID.ToString();
                defFolder1SegName += presetID.ToString();
            }

            defKey.RecMode = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "RecMode", 1, SettingPath.TimerSrvIniPath);
            defKey.Priority = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "Priority", 2, SettingPath.TimerSrvIniPath);
            defKey.TuijyuuFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "TuijyuuFlag", 1, SettingPath.TimerSrvIniPath);
            defKey.ServiceMode = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "ServiceMode", 0, SettingPath.TimerSrvIniPath);
            defKey.PittariFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "PittariFlag", 0, SettingPath.TimerSrvIniPath);

            defKey.BatFilePath = IniFileHandler.GetPrivateProfileString(defName, "BatFilePath", "", SettingPath.TimerSrvIniPath);

            int count = IniFileHandler.GetPrivateProfileInt(defFolderName, "Count", 0, SettingPath.TimerSrvIniPath);
            for (int i = 0; i < count; i++)
            {
                RecFileSetInfo folderInfo = new RecFileSetInfo();
                folderInfo.RecFolder = IniFileHandler.GetPrivateProfileString(defFolderName, i.ToString(), "", SettingPath.TimerSrvIniPath);
                folderInfo.WritePlugIn = IniFileHandler.GetPrivateProfileString(defFolderName, "WritePlugIn" + i.ToString(), "Write_Default.dll", SettingPath.TimerSrvIniPath);
                folderInfo.RecNamePlugIn = IniFileHandler.GetPrivateProfileString(defFolderName, "RecNamePlugIn" + i.ToString(), "", SettingPath.TimerSrvIniPath);

                defKey.RecFolderList.Add(folderInfo);
            }

            count = IniFileHandler.GetPrivateProfileInt(defFolder1SegName, "Count", 0, SettingPath.TimerSrvIniPath);
            for (int i = 0; i < count; i++)
            {
                RecFileSetInfo folderInfo = new RecFileSetInfo();
                folderInfo.RecFolder = IniFileHandler.GetPrivateProfileString(defFolder1SegName, i.ToString(), "", SettingPath.TimerSrvIniPath);
                folderInfo.WritePlugIn = IniFileHandler.GetPrivateProfileString(defFolder1SegName, "WritePlugIn" + i.ToString(), "Write_Default.dll", SettingPath.TimerSrvIniPath);
                folderInfo.RecNamePlugIn = IniFileHandler.GetPrivateProfileString(defFolder1SegName, "RecNamePlugIn" + i.ToString(), "", SettingPath.TimerSrvIniPath);

                defKey.PartialRecFolder.Add(folderInfo);
            }

            defKey.SuspendMode = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "SuspendMode", 0, SettingPath.TimerSrvIniPath);
            defKey.RebootFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "RebootFlag", 0, SettingPath.TimerSrvIniPath);
            defKey.UseMargineFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "UseMargineFlag", 0, SettingPath.TimerSrvIniPath);
            defKey.StartMargine = IniFileHandler.GetPrivateProfileInt(defName, "StartMargine", 0, SettingPath.TimerSrvIniPath);
            defKey.EndMargine = IniFileHandler.GetPrivateProfileInt(defName, "EndMargine", 0, SettingPath.TimerSrvIniPath);
            defKey.ContinueRecFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "ContinueRec", 0, SettingPath.TimerSrvIniPath);
            defKey.PartialRecFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "PartialRec", 0, SettingPath.TimerSrvIniPath);
            defKey.TunerID = (UInt32)IniFileHandler.GetPrivateProfileInt(defName, "TunerID", 0, SettingPath.TimerSrvIniPath);

        }

        public void GetDefSearchSetting(EpgSearchKeyInfo defKey)
        {
            defKey.andKey = SearchKeyAndKey;
            defKey.notKey = SearchKeyNotKey;
            defKey.regExpFlag = SearchKeyRegExp ? 1 : 0;
            defKey.aimaiFlag = (byte)(SearchKeyAimaiFlag ? 1 : 0);
            defKey.titleOnlyFlag = SearchKeyTitleOnly ? 1 : 0;
            defKey.notContetFlag = (byte)(SearchKeyNotContent ? 1 : 0);
            defKey.notDateFlag = (byte)(SearchKeyNotDate ? 1 : 0);
            defKey.contentList.Clear();
            foreach (ContentKindInfo info in SearchKeyContentList)
            {
                EpgContentData item = new EpgContentData();
                item.content_nibble_level_1 = info.Nibble1;
                item.content_nibble_level_2 = info.Nibble2;
                defKey.contentList.Add(item);
            }
            defKey.dateList.Clear();
            defKey.dateList.AddRange(SearchKeyDateItemList.Select(a => a.DeepClone().DateInfo));
            defKey.serviceList.Clear();
            defKey.serviceList.AddRange(SearchKeyServiceList);
            defKey.freeCAFlag = SearchKeyFreeCA;
            defKey.chkRecEnd = SearchKeyChkRecEnd;
            defKey.chkRecDay = SearchKeyChkRecDay;
        }

        private void SetColorSetting()
        {
            //番組表の背景色
            if (ContentColorList.Count < 17)
            {
                ContentColorList.AddRange((new string[] {
                    "LightYellow",
                    "Lavender",
                    "LavenderBlush",
                    "MistyRose",
                    "Honeydew",
                    "LightCyan",
                    "PapayaWhip",
                    "Pink",
                    "LightYellow",
                    "PapayaWhip",
                    "AliceBlue",
                    "AliceBlue",
                    "White",
                    "White",
                    "White",
                    "WhiteSmoke",
                    "White" }).Skip(ContentColorList.Count));
            }
            //番組表の背景カスタム色(+4は番組表の予約枠色)
            if (ContentCustColorList.Count < 17 + 4)
            {
                ContentCustColorList.AddRange(Enumerable.Repeat(0xFFFFFFFF, 17 + 4 - ContentCustColorList.Count));
            }

            //番組表の時間軸のデフォルトの背景色
            if (TimeColorList.Count < 4)
            {
                TimeColorList.AddRange((new string[] {
                    "MediumPurple",
                    "LightSeaGreen",
                    "LightSalmon",
                    "CornflowerBlue" }).Skip(TimeColorList.Count));
            }
            if (TimeCustColorList.Count < 4)
            {
                TimeCustColorList.AddRange(Enumerable.Repeat(0xFFFFFFFF, 4 - TimeCustColorList.Count));
            }
        }

        private static string ConvertXElem(XElement x, bool w, string key, string val, string def)
        {
            if (w)
            {
                x.Add(new XElement(key, val));
            }
            else if (x != null && (x = x.Element(key)) != null)
            {
                return x.Value;
            }
            return def;
        }

        private static bool ConvertXElem(XElement x, bool w, string key, bool val, bool def)
        {
            return ConvertXElem(x, w, key, (val ? "true" : "false"), (def ? "true" : "false")) == "true";
        }

        private static double ConvertXElem(XElement x, bool w, string key, double val, double def)
        {
            return double.TryParse(ConvertXElem(x, w, key, val.ToString(CultureInfo.InvariantCulture), def.ToString(CultureInfo.InvariantCulture)),
                                   NumberStyles.Float, CultureInfo.InvariantCulture, out val) ? val : def;
        }

        private static IEnumerable<T> ConvertXElements<T>(XElement x, bool w, string key, IEnumerable<T> list, string type, Func<XElement, T, T> conv)
        {
            if (w)
            {
                var xx = new XElement(key);
                foreach (T val in list)
                {
                    var xxx = new XElement(type);
                    conv(xxx, val);
                    xx.Add(xxx);
                }
                x.Add(xx);
            }
            else if (x != null && (x = x.Element(key)) != null)
            {
                return x.Elements(type).Select(xx => conv(xx, default(T)));
            }
            return Enumerable.Empty<T>();
        }

        private static IEnumerable<string> ConvertXElements(XElement x, bool w, string key, IEnumerable<string> list)
        {
            return ConvertXElements(x, w, key, list, "string", (xx, val) => {
                if (w)
                {
                    xx.Value = val;
                }
                return xx.Value;
            });
        }

        private static IEnumerable<double> ConvertXElements(XElement x, bool w, string key, IEnumerable<double> list, string type)
        {
            return ConvertXElements(x, w, key, list, type, (xx, val) => {
                if (w)
                {
                    xx.Value = val.ToString(CultureInfo.InvariantCulture);
                }
                double.TryParse(xx.Value, NumberStyles.Float, CultureInfo.InvariantCulture, out val);
                return val;
            });
        }

        private static IEnumerable<EpgContentData> ConvertXElements(XElement x, bool w, string key, IEnumerable<EpgContentData> list)
        {
            return ConvertXElements(x, w, key, list, "EpgContentData", (xx, val) => {
                var r = new EpgContentData();
                val = val ?? r;
                r.content_nibble_level_1 = (byte)ConvertXElem(xx, w, "content_nibble_level_1", val.content_nibble_level_1, 0);
                r.content_nibble_level_2 = (byte)ConvertXElem(xx, w, "content_nibble_level_2", val.content_nibble_level_2, 0);
                r.user_nibble_1 = (byte)ConvertXElem(xx, w, "user_nibble_1", val.user_nibble_1, 0);
                r.user_nibble_2 = (byte)ConvertXElem(xx, w, "user_nibble_2", val.user_nibble_2, 0);
                return val;
            });
        }

        private static IEnumerable<EpgSearchDateInfo> ConvertXElements(XElement x, bool w, string key, IEnumerable<EpgSearchDateInfo> list)
        {
            return ConvertXElements(x, w, key, list, "EpgSearchDateInfo", (xx, val) => {
                var r = new EpgSearchDateInfo();
                val = val ?? r;
                r.startDayOfWeek = (byte)ConvertXElem(xx, w, "startDayOfWeek", val.startDayOfWeek, 0);
                r.startHour = (ushort)ConvertXElem(xx, w, "startHour", val.startHour, 0);
                r.startMin = (ushort)ConvertXElem(xx, w, "startMin", val.startMin, 0);
                r.endDayOfWeek = (byte)ConvertXElem(xx, w, "endDayOfWeek", val.endDayOfWeek, 0);
                r.endHour = (ushort)ConvertXElem(xx, w, "endHour", val.endHour, 0);
                r.endMin = (ushort)ConvertXElem(xx, w, "endMin", val.endMin, 0);
                return val;
            });
        }

        private static IEnumerable<CustomEpgTabInfo> ConvertXElements(XElement x, bool w, string key, IEnumerable<CustomEpgTabInfo> list)
        {
            return ConvertXElements(x, w, key, list, "CustomEpgTabInfo", (xx, val) => {
                var r = new CustomEpgTabInfo();
                val = val ?? r;
                r.TabName = ConvertXElem(xx, w, "TabName", val.TabName, "");
                r.ViewMode = (int)ConvertXElem(xx, w, "ViewMode", val.ViewMode, 0);
                r.NeedTimeOnlyBasic = ConvertXElem(xx, w, "NeedTimeOnlyBasic", val.NeedTimeOnlyBasic, false);
                r.NeedTimeOnlyWeek = ConvertXElem(xx, w, "NeedTimeOnlyWeek", val.NeedTimeOnlyWeek, false);
                r.StartTimeWeek = (int)ConvertXElem(xx, w, "StartTimeWeek", val.StartTimeWeek, 0);
                r.ViewServiceList = ConvertXElements(xx, w, "ViewServiceList",
                    val.ViewServiceList.Select(a => (double)a), "unsignedLong").Select(a => (ulong)a).ToList();
                r.ViewContentKindList = ConvertXElements(xx, w, "ViewContentKindList",
                    val.ViewContentKindList.Select(a => (double)a), "unsignedShort").Select(a => (ushort)a).ToList();
                r.SearchMode = ConvertXElem(xx, w, "SearchMode", val.SearchMode, false);

                var xxx = w ? new XElement("SearchKey") : xx.Element("SearchKey");
                r.SearchKey.andKey = ConvertXElem(xxx, w, "andKey", val.SearchKey.andKey, "");
                r.SearchKey.notKey = ConvertXElem(xxx, w, "notKey", val.SearchKey.notKey, "");
                r.SearchKey.regExpFlag = (int)ConvertXElem(xxx, w, "regExpFlag", val.SearchKey.regExpFlag, 0);
                r.SearchKey.titleOnlyFlag = (int)ConvertXElem(xxx, w, "titleOnlyFlag", val.SearchKey.titleOnlyFlag, 0);
                r.SearchKey.contentList = ConvertXElements(xxx, w, "contentList", val.SearchKey.contentList).ToList();
                r.SearchKey.dateList = ConvertXElements(xxx, w, "dateList", val.SearchKey.dateList).ToList();
                r.SearchKey.serviceList = ConvertXElements(xxx, w, "serviceList",
                    val.SearchKey.serviceList.Select(a => (double)a), "long").Select(a => (long)a).ToList();
                r.SearchKey.videoList = ConvertXElements(xxx, w, "videoList",
                    val.SearchKey.videoList.Select(a => (double)a), "unsignedShort").Select(a => (ushort)a).ToList();
                r.SearchKey.audioList = ConvertXElements(xxx, w, "audioList",
                    val.SearchKey.audioList.Select(a => (double)a), "unsignedShort").Select(a => (ushort)a).ToList();
                r.SearchKey.aimaiFlag = (byte)ConvertXElem(xxx, w, "aimaiFlag", val.SearchKey.aimaiFlag, 0);
                r.SearchKey.notContetFlag = (byte)ConvertXElem(xxx, w, "notContetFlag", val.SearchKey.notContetFlag, 0);
                r.SearchKey.notDateFlag = (byte)ConvertXElem(xxx, w, "notDateFlag", val.SearchKey.notDateFlag, 0);
                r.SearchKey.freeCAFlag = (byte)ConvertXElem(xxx, w, "freeCAFlag", val.SearchKey.freeCAFlag, 0);
                r.SearchKey.chkRecEnd = (byte)ConvertXElem(xxx, w, "chkRecEnd", val.SearchKey.chkRecEnd, 0);
                r.SearchKey.chkRecDay = (ushort)ConvertXElem(xxx, w, "chkRecDay", val.SearchKey.chkRecDay, 0);
                if (w)
                {
                    xx.Add(xxx);
                }
                r.FilterEnded = ConvertXElem(xx, w, "FilterEnded", val.FilterEnded, false);
                return val;
            });
        }

        private static IEnumerable<DateItem> ConvertXElements(XElement x, bool w, string key, IEnumerable<DateItem> list)
        {
            return ConvertXElements(x, w, key, list, "DateItem", (xx, val) => {
                var r = new DateItem() { DateInfo = new EpgSearchDateInfo() };
                val = val ?? r;
                var xxx = w ? new XElement("DateInfo") : xx.Element("DateInfo");
                r.DateInfo.startDayOfWeek = (byte)ConvertXElem(xxx, w, "startDayOfWeek", val.DateInfo.startDayOfWeek, 0);
                r.DateInfo.startHour = (ushort)ConvertXElem(xxx, w, "startHour", val.DateInfo.startHour, 0);
                r.DateInfo.startMin = (ushort)ConvertXElem(xxx, w, "startMin", val.DateInfo.startMin, 0);
                r.DateInfo.endDayOfWeek = (byte)ConvertXElem(xxx, w, "endDayOfWeek", val.DateInfo.endDayOfWeek, 0);
                r.DateInfo.endHour = (ushort)ConvertXElem(xxx, w, "endHour", val.DateInfo.endHour, 0);
                r.DateInfo.endMin = (ushort)ConvertXElem(xxx, w, "endMin", val.DateInfo.endMin, 0);
                if (w)
                {
                    xx.Add(xxx);
                }
                return val;
            });
        }

        private static IEnumerable<ContentKindInfo> ConvertXElements(XElement x, bool w, string key, IEnumerable<ContentKindInfo> list)
        {
            return ConvertXElements(x, w, key, list, "ContentKindInfo", (xx, val) => {
                var r = new ContentKindInfo();
                val = val ?? r;
                r.Nibble1 = (byte)ConvertXElem(xx, w, "Nibble1", val.Nibble1, 0);
                r.Nibble2 = (byte)ConvertXElem(xx, w, "Nibble2", val.Nibble2, 0);
                return r;
            });
        }

        private static IEnumerable<IEPGStationInfo> ConvertXElements(XElement x, bool w, string key, IEnumerable<IEPGStationInfo> list)
        {
            return ConvertXElements(x, w, key, list, "IEPGStationInfo", (xx, val) => {
                var r = new IEPGStationInfo();
                val = val ?? r;
                r.StationName = ConvertXElem(xx, w, "StationName", val.StationName, "");
                r.Key = (ulong)ConvertXElem(xx, w, "Key", val.Key, 0);
                return r;
            });
        }

        private static IEnumerable<ListColumnInfo> ConvertXElements(XElement x, bool w, string key, IEnumerable<ListColumnInfo> list)
        {
            return ConvertXElements(x, w, key, list, "ListColumnInfo", (xx, val) => {
                var r = new ListColumnInfo();
                val = val ?? r;
                r.Tag = ConvertXElem(xx, w, "Tag", val.Tag, "");
                r.Width = ConvertXElem(xx, w, "Width", val.Width, 0);
                return r;
            });
        }
    }
}
