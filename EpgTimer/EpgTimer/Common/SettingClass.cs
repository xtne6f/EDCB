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
using System.Threading;
using System.Windows;
using System.Windows.Media;

namespace EpgTimer
{
    class IniFileHandler
    {
        public static int GetPrivateProfileInt(string appName, string keyName, int nDefault, string fileName)
        {
            return NativeMethods.GetPrivateProfileInt(appName, keyName, nDefault, fileName);
        }

        public static bool WritePrivateProfileString(string appName, string keyName, string lpString, string fileName)
        {
            return NativeMethods.WritePrivateProfileString(appName, keyName, lpString, fileName);
        }

        public static string
          GetPrivateProfileString(string lpAppName,
          string lpKeyName, string lpDefault, string lpFileName)
        {
            StringBuilder buff = null;
            for (uint n = 512; n <= 1024 * 1024; n *= 2)
            {
                //セクション名取得などのNUL文字分割された結果は先頭要素のみ格納される
                buff = new StringBuilder((int)n);
                if (NativeMethods.GetPrivateProfileString(lpAppName, lpKeyName, lpDefault, buff, n, lpFileName) < n - 2)
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

        private static class NativeMethods
        {
            [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
            public static extern uint GetPrivateProfileString(string lpAppName, string lpKeyName, string lpDefault,
                                                              StringBuilder lpReturnedString, uint nSize, string lpFileName);

            [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
            public static extern int GetPrivateProfileInt(string lpAppName, string lpKeyName, int nDefault, string lpFileName);

            [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
            public static extern bool WritePrivateProfileString(string lpAppName, string lpKeyName, string lpString, string lpFileName);
        }
    }

    class SettingPath
    {
        public static string CommonIniPath
        {
            get
            {
                return Path.Combine(ModulePath, "Common.ini");
            }
        }

        public static string BitrateIniPath
        {
            get
            {
                return Path.Combine(ModulePath, "Bitrate.ini");
            }
        }

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

        public static string ModuleName
        {
            get
            {
                return Path.GetFileName(Assembly.GetEntryAssembly().Location);
            }
        }
    }

    public class EpgSetting
    {
        public double MinHeight { get; set; }
        public double MinimumHeight { get; set; }
        public double ServiceWidth { get; set; }
        public bool MouseScrollAuto { get; set; }
        public double ScrollSize { get; set; }
        public bool MouseHorizontalScrollAuto { get; set; }
        public double HorizontalScrollSize { get; set; }
        public string FontName { get; set; }
        public double FontSize { get; set; }
        public string FontNameTitle { get; set; }
        public double FontSizeTitle { get; set; }
        public bool FontBoldTitle { get; set; }
        public double DragScroll { get; set; }
        public List<string> ContentColorList { get; set; }
        public List<uint> ContentCustColorList { get; set; }
        public List<string> TimeColorList { get; set; }
        public List<uint> TimeCustColorList { get; set; }
        public string ReserveRectColorNormal { get; set; }
        public string ReserveRectColorNo { get; set; }
        public string ReserveRectColorNoTuner { get; set; }
        public string ReserveRectColorWarning { get; set; }
        public int ReserveRectFillOpacity { get; set; }
        public bool ReserveRectFillWithShadow { get; set; }
        public bool ReserveRectShowMarker { get; set; }
        public string TitleColor1 { get; set; }
        public string TitleColor2 { get; set; }
        public uint TitleCustColor1 { get; set; }
        public uint TitleCustColor2 { get; set; }
        public string ServiceColor { get; set; }
        public uint ServiceCustColor { get; set; }
        public bool EpgToolTip { get; set; }
        public double EpgBorderLeftSize { get; set; }
        public double EpgBorderTopSize { get; set; }
        public bool EpgTitleIndent { get; set; }
        public string EpgReplacePattern { get; set; }
        public string EpgReplacePatternTitle { get; set; }
        public bool EpgToolTipNoViewOnly { get; set; }
        public int EpgToolTipViewWait { get; set; }
        public bool EpgPopup { get; set; }
        public bool EpgAdjustPopup { get; set; }
        public bool EpgExtInfoTable { get; set; }
        public bool EpgExtInfoPopup { get; set; }
        public bool EpgGradation { get; set; }
        public bool EpgGradationHeader { get; set; }
        public byte EpgTipsBackColorR { get; set; }
        public byte EpgTipsBackColorG { get; set; }
        public byte EpgTipsBackColorB { get; set; }
        public byte EpgTipsForeColorR { get; set; }
        public byte EpgTipsForeColorG { get; set; }
        public byte EpgTipsForeColorB { get; set; }
        public byte EpgBackColorA { get; set; }
        public byte EpgBackColorR { get; set; }
        public byte EpgBackColorG { get; set; }
        public byte EpgBackColorB { get; set; }
        public bool EpgInfoSingleClick { get; set; }
        public byte EpgInfoOpenMode { get; set; }
        public EpgSetting DeepClone()
        {
            var other = (EpgSetting)MemberwiseClone();
            other.ContentColorList = ContentColorList.ToList();
            other.ContentCustColorList = ContentCustColorList.ToList();
            other.TimeColorList = TimeColorList.ToList();
            other.TimeCustColorList = TimeCustColorList.ToList();
            return other;
        }
    }

    public class Settings
    {
        public bool UseCustomEpgView { get; set; }
        public List<CustomEpgTabInfo> CustomEpgTabList { get; set; }
        public List<EpgSetting> EpgSettingList { get; set; }
        public bool NoToolTip { get; set; }
        public bool NoBallonTips { get; set; }
        public bool BalloonTipRealtime { get; set; }
        public int ForceHideBalloonTipSec { get; set; }
        public bool PlayDClick { get; set; }
        public bool ConfirmDelRecInfo { get; set; }
        public bool ConfirmDelRecInfoAlways { get; set; }
        public bool SaveSearchKeyword { get; set; }
        public bool ShowLogo { get; set; }
        public bool ShowEpgCapServiceOnly { get; set; }
        public bool SortServiceList { get; set; }
        public bool ExitAfterProcessingArgs { get; set; }
        public string ResColumnHead { get; set; }
        public ListSortDirection ResSortDirection { get; set; }
        public bool ResHideButton { get; set; }
        public System.Windows.WindowState LastWindowState { get; set; }
        public double MainWndLeft { get; set; }
        public double MainWndTop { get; set; }
        public double MainWndWidth { get; set; }
        public double MainWndHeight { get; set; }
        public double SearchWndTabsHeight { get; set; }
        public double SearchWndNotKeyRatio { get; set; }
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
        public List<EpgSearchDateInfo> SearchKeyDateItemList { get; set; }
        public List<long> SearchKeyServiceList { get; set; }
        public byte SearchKeyFreeCA { get; set; }
        public byte SearchKeyChkRecEnd { get; set; }
        public ushort SearchKeyChkRecDay { get; set; }
        public string RecInfoColumnHead { get; set; }
        public ListSortDirection RecInfoSortDirection { get; set; }
        public bool RecInfoHideButton { get; set; }
        public bool AutoAddEpgHideButton { get; set; }
        public bool AutoAddManualHideButton { get; set; }
        public string TvTestExe { get; set; }
        public string TvTestCmd { get; set; }
        public bool NwTvMode { get; set; }
        public bool NwTvModeUDP { get; set; }
        public bool NwTvModeTCP { get; set; }
        public bool NwTvModePipe { get; set; }
        public bool FilePlay { get; set; }
        public string FilePlayExe { get; set; }
        public string FilePlayCmd { get; set; }
        public string FilePathReplacePattern { get; set; }
        public bool FilePlayOnAirWithExe { get; set; }
        public List<IEPGStationInfo> IEpgStationList { get; set; }
        public string NWServerIP { get; set; }
        public uint NWServerPort { get; set; }
        public uint NWWaitPort { get; set; }
        public string NWMacAdd { get; set; }
        public bool WakeReconnectNW { get; set; }
        public bool SuspendCloseNW { get; set; }
        public bool NgAutoEpgLoadNW { get; set; }
        public bool PrebuildEpg { get; set; }
        public int TvTestOpenWait { get; set; }
        public int TvTestChgBonWait { get; set; }
        public int ResColorPosition { get; set; }
        public int ResAlternationCount { get; set; }
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
        public int RecEndColorPosition { get; set; }
        public int RecEndAlternationCount { get; set; }
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
        public uint ExecBat { get; set; }
        public uint SuspendChk { get; set; }
        public List<ListColumnInfo> ReserveListColumn { get; set; }
        public List<ListColumnInfo> RecInfoListColumn { get; set; }
        public List<ListColumnInfo> AutoAddEpgColumn { get; set; }
        public List<ListColumnInfo> AutoAddManualColumn { get; set; }
        public double SearchWndWidth { get; set; }
        public double SearchWndHeight { get; set; }
        public int NotifyLogMax { get; set; }
        public bool ShowTray { get; set; }
        public bool MinHide { get; set; }
        public int NoStyle { get; set; }
        public bool ApplyContextMenuStyle { get; set; }
        public int NoSendClose { get; set; }
        public string StartTab { get; set; }

        private Settings()
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
            r.EpgSettingList            = new List<EpgSetting>(3) { new EpgSetting(), new EpgSetting(), new EpgSetting() };
            for (int i = 0; i < EpgSettingList.Count; i++)
            {
                //最初(デフォルト)のデザイン設定は直下に置く
                XElement xx = (i == 0 || x == null ? x : w ? new XElement("EpgSetting" + i) : x.Element("EpgSetting" + i));
                EpgSetting rr = r.EpgSettingList[i];
                EpgSetting val = EpgSettingList[i];
                rr.MinHeight                = ConvertXElem(xx, w, "MinHeight", val.MinHeight, 2);
                rr.MinimumHeight            = ConvertXElem(xx, w, "MinimumHeight", val.MinimumHeight, 0.6);
                rr.ServiceWidth             = ConvertXElem(xx, w, "ServiceWidth", val.ServiceWidth, 150);
                rr.MouseScrollAuto          = ConvertXElem(xx, w, "MouseScrollAuto", val.MouseScrollAuto, false);
                rr.ScrollSize               = ConvertXElem(xx, w, "ScrollSize", val.ScrollSize, 240);
                rr.MouseHorizontalScrollAuto = ConvertXElem(xx, w, "MouseHorizontalScrollAuto", val.MouseHorizontalScrollAuto, false);
                rr.HorizontalScrollSize     = ConvertXElem(xx, w, "HorizontalScrollSize", val.HorizontalScrollSize, 150);
                rr.FontName                 = ConvertXElem(xx, w, "FontName", val.FontName, "メイリオ");
                rr.FontSize                 = ConvertXElem(xx, w, "FontSize", val.FontSize, 12);
                rr.FontNameTitle            = ConvertXElem(xx, w, "FontNameTitle", val.FontNameTitle, "メイリオ");
                rr.FontSizeTitle            = ConvertXElem(xx, w, "FontSizeTitle", val.FontSizeTitle, 12);
                rr.FontBoldTitle            = ConvertXElem(xx, w, "FontBoldTitle", val.FontBoldTitle, true);
                rr.DragScroll               = ConvertXElem(xx, w, "DragScroll", val.DragScroll, 1.5);
                rr.ContentColorList         = ConvertXElements(xx, w, "ContentColorList", val.ContentColorList).ToList();
                rr.ContentCustColorList     = ConvertXElements(xx, w, "ContentCustColorList",
                    (val.ContentCustColorList ?? Enumerable.Empty<uint>()).Select(a => (double)a), "unsignedInt").Select(a => (uint)a).ToList();
                rr.TimeColorList            = ConvertXElements(xx, w, "TimeColorList", val.TimeColorList).ToList();
                rr.TimeCustColorList        = ConvertXElements(xx, w, "TimeCustColorList",
                    (val.TimeCustColorList ?? Enumerable.Empty<uint>()).Select(a => (double)a), "unsignedInt").Select(a => (uint)a).ToList();
                rr.ReserveRectColorNormal   = ConvertXElem(xx, w, "ReserveRectColorNormal", val.ReserveRectColorNormal, "Lime");
                rr.ReserveRectColorNo       = ConvertXElem(xx, w, "ReserveRectColorNo", val.ReserveRectColorNo, "Black");
                rr.ReserveRectColorNoTuner  = ConvertXElem(xx, w, "ReserveRectColorNoTuner", val.ReserveRectColorNoTuner, "Red");
                rr.ReserveRectColorWarning  = ConvertXElem(xx, w, "ReserveRectColorWarning", val.ReserveRectColorWarning, "Yellow");
                rr.ReserveRectFillOpacity   = (int)ConvertXElem(xx, w, "ReserveRectFillOpacity", val.ReserveRectFillOpacity, 0);
                rr.ReserveRectFillWithShadow = ConvertXElem(xx, w, "ReserveRectFillWithShadow", val.ReserveRectFillWithShadow, true);
                rr.ReserveRectShowMarker    = ConvertXElem(xx, w, "ReserveRectShowMarker", val.ReserveRectShowMarker, true);
                rr.TitleColor1              = ConvertXElem(xx, w, "TitleColor1", val.TitleColor1, "Black");
                rr.TitleColor2              = ConvertXElem(xx, w, "TitleColor2", val.TitleColor2, "Black");
                rr.TitleCustColor1          = (uint)ConvertXElem(xx, w, "TitleCustColor1", val.TitleCustColor1, 0xFFFFFFFF);
                rr.TitleCustColor2          = (uint)ConvertXElem(xx, w, "TitleCustColor2", val.TitleCustColor2, 0xFFFFFFFF);
                rr.ServiceColor             = ConvertXElem(xx, w, "ServiceColor", val.ServiceColor, "LightSlateGray");
                rr.ServiceCustColor         = (uint)ConvertXElem(xx, w, "ServiceCustColor", val.ServiceCustColor, 0xFFFFFFFF);
                rr.EpgToolTip               = ConvertXElem(xx, w, "EpgToolTip", val.EpgToolTip, false);
                rr.EpgBorderLeftSize        = ConvertXElem(xx, w, "EpgBorderLeftSize", val.EpgBorderLeftSize, 2);
                rr.EpgBorderTopSize         = ConvertXElem(xx, w, "EpgBorderTopSize", val.EpgBorderTopSize, 0.5);
                rr.EpgTitleIndent           = ConvertXElem(xx, w, "EpgTitleIndent", val.EpgTitleIndent, true);
                rr.EpgReplacePattern        = ConvertXElem(xx, w, "EpgReplacePattern", val.EpgReplacePattern, "");
                rr.EpgReplacePatternTitle   = ConvertXElem(xx, w, "EpgReplacePatternTitle", val.EpgReplacePatternTitle, "");
                rr.EpgToolTipNoViewOnly     = ConvertXElem(xx, w, "EpgToolTipNoViewOnly", val.EpgToolTipNoViewOnly, true);
                rr.EpgToolTipViewWait       = (int)ConvertXElem(xx, w, "EpgToolTipViewWait", val.EpgToolTipViewWait, 1500);
                rr.EpgPopup                 = ConvertXElem(xx, w, "EpgPopup", val.EpgPopup, true);
                rr.EpgAdjustPopup           = ConvertXElem(xx, w, "EpgAdjustPopup", val.EpgAdjustPopup, true);
                rr.EpgExtInfoTable          = ConvertXElem(xx, w, "EpgExtInfoTable", val.EpgExtInfoTable, false);
                rr.EpgExtInfoPopup          = ConvertXElem(xx, w, "EpgExtInfoPopup", val.EpgExtInfoPopup, true);
                rr.EpgGradation             = ConvertXElem(xx, w, "EpgGradation", val.EpgGradation, true);
                rr.EpgGradationHeader       = ConvertXElem(xx, w, "EpgGradationHeader", val.EpgGradationHeader, true);
                rr.EpgTipsBackColorR        = (byte)ConvertXElem(xx, w, "EpgTipsBackColorR", val.EpgTipsBackColorR, 0xD3);
                rr.EpgTipsBackColorG        = (byte)ConvertXElem(xx, w, "EpgTipsBackColorG", val.EpgTipsBackColorG, 0xD3);
                rr.EpgTipsBackColorB        = (byte)ConvertXElem(xx, w, "EpgTipsBackColorB", val.EpgTipsBackColorB, 0xD3);
                rr.EpgTipsForeColorR        = (byte)ConvertXElem(xx, w, "EpgTipsForeColorR", val.EpgTipsForeColorR, 0);
                rr.EpgTipsForeColorG        = (byte)ConvertXElem(xx, w, "EpgTipsForeColorG", val.EpgTipsForeColorG, 0);
                rr.EpgTipsForeColorB        = (byte)ConvertXElem(xx, w, "EpgTipsForeColorB", val.EpgTipsForeColorB, 0);
                rr.EpgBackColorA            = (byte)ConvertXElem(xx, w, "EpgBackColorA", val.EpgBackColorA, 0x80);
                rr.EpgBackColorR            = (byte)ConvertXElem(xx, w, "EpgBackColorR", val.EpgBackColorR, 0xA9);
                rr.EpgBackColorG            = (byte)ConvertXElem(xx, w, "EpgBackColorG", val.EpgBackColorG, 0xA9);
                rr.EpgBackColorB            = (byte)ConvertXElem(xx, w, "EpgBackColorB", val.EpgBackColorB, 0xA9);
                rr.EpgInfoSingleClick       = ConvertXElem(xx, w, "EpgInfoSingleClick", val.EpgInfoSingleClick, false);
                rr.EpgInfoOpenMode          = (byte)ConvertXElem(xx, w, "EpgInfoOpenMode", val.EpgInfoOpenMode, 0);
                if (i != 0 && w)
                {
                    x.Add(xx);
                }
            }
            r.NoToolTip                 = ConvertXElem(x, w, "NoToolTip", NoToolTip, false);
            r.NoBallonTips              = ConvertXElem(x, w, "NoBallonTips", NoBallonTips, false);
            r.BalloonTipRealtime        = ConvertXElem(x, w, "BalloonTipRealtime", BalloonTipRealtime, false);
            r.ForceHideBalloonTipSec    = (int)ConvertXElem(x, w, "ForceHideBalloonTipSec", ForceHideBalloonTipSec, 0);
            r.PlayDClick                = ConvertXElem(x, w, "PlayDClick", PlayDClick, false);
            r.ConfirmDelRecInfo         = ConvertXElem(x, w, "ConfirmDelRecInfo", ConfirmDelRecInfo, true);
            r.ConfirmDelRecInfoAlways   = ConvertXElem(x, w, "ConfirmDelRecInfoAlways", ConfirmDelRecInfoAlways, false);
            r.SaveSearchKeyword         = ConvertXElem(x, w, "SaveSearchKeyword", SaveSearchKeyword, true);
            r.ShowLogo                  = ConvertXElem(x, w, "ShowLogo", ShowLogo, true);
            r.ShowEpgCapServiceOnly     = ConvertXElem(x, w, "ShowEpgCapServiceOnly", ShowEpgCapServiceOnly, false);
            r.SortServiceList           = ConvertXElem(x, w, "SortServiceList", SortServiceList, true);
            r.ExitAfterProcessingArgs   = ConvertXElem(x, w, "ExitAfterProcessingArgs", ExitAfterProcessingArgs, false);
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
            r.SearchWndNotKeyRatio      = ConvertXElem(x, w, "SearchWndNotKeyRatio", SearchWndNotKeyRatio, 0.6);
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
            r.SearchKeyDateItemList     = ConvertXElements(x, w, "SearchKeyDateItemList", SearchKeyDateItemList, true).ToList();
            r.SearchKeyServiceList      = ConvertXElements(x, w, "SearchKeyServiceList",
                (SearchKeyServiceList ?? Enumerable.Empty<long>()).Select(a => (double)a), "long").Select(a => (long)a).ToList();
            r.SearchKeyFreeCA           = (byte)ConvertXElem(x, w, "SearchKeyFreeCA", SearchKeyFreeCA, 0);
            r.SearchKeyChkRecEnd        = (byte)ConvertXElem(x, w, "SearchKeyChkRecEnd", SearchKeyChkRecEnd, 0);
            r.SearchKeyChkRecDay        = (ushort)ConvertXElem(x, w, "SearchKeyChkRecDay", SearchKeyChkRecDay, 6);
            r.RecInfoColumnHead         = ConvertXElem(x, w, "RecInfoColumnHead", RecInfoColumnHead, "");
            Enum.TryParse(ConvertXElem(x, w, "RecInfoSortDirection", RecInfoSortDirection.ToString(), ""), out sd);
            r.RecInfoSortDirection      = sd;
            r.RecInfoHideButton         = ConvertXElem(x, w, "RecInfoHideButton", RecInfoHideButton, false);
            r.AutoAddEpgHideButton      = ConvertXElem(x, w, "AutoAddEpgHideButton", AutoAddEpgHideButton, false);
            r.AutoAddManualHideButton   = ConvertXElem(x, w, "AutoAddManualHideButton", AutoAddManualHideButton, false);
            r.TvTestExe                 = ConvertXElem(x, w, "TvTestExe", TvTestExe, "");
            r.TvTestCmd                 = ConvertXElem(x, w, "TvTestCmd", TvTestCmd, "");
            r.NwTvMode                  = ConvertXElem(x, w, "NwTvMode", NwTvMode, false);
            r.NwTvModeUDP               = ConvertXElem(x, w, "NwTvModeUDP", NwTvModeUDP, false);
            r.NwTvModeTCP               = ConvertXElem(x, w, "NwTvModeTCP", NwTvModeTCP, false);
            r.NwTvModePipe              = ConvertXElem(x, w, "NwTvModePipe", NwTvModePipe, false);
            r.FilePlay                  = ConvertXElem(x, w, "FilePlay", FilePlay, true);
            r.FilePlayExe               = ConvertXElem(x, w, "FilePlayExe", FilePlayExe, "");
            r.FilePlayCmd               = ConvertXElem(x, w, "FilePlayCmd", FilePlayCmd, "\"$FilePath$\"");
            r.FilePathReplacePattern    = ConvertXElem(x, w, "FilePathReplacePattern", FilePathReplacePattern, "");
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
            r.ResColorPosition          = (int)ConvertXElem(x, w, "ResColorPosition", ResColorPosition, 2);
            r.ResAlternationCount       = (int)ConvertXElem(x, w, "ResAlternationCount", ResAlternationCount, 2);
            r.ResDefColorA              = (byte)ConvertXElem(x, w, "ResDefColorA", ResDefColorA, 0);
            r.ResDefColorR              = (byte)ConvertXElem(x, w, "ResDefColorR", ResDefColorR, 0xFF);
            r.ResDefColorG              = (byte)ConvertXElem(x, w, "ResDefColorG", ResDefColorG, 0xFF);
            r.ResDefColorB              = (byte)ConvertXElem(x, w, "ResDefColorB", ResDefColorB, 0xFF);
            r.ResErrColorA              = (byte)ConvertXElem(x, w, "ResErrColorA", ResErrColorA, 0x80);
            r.ResErrColorR              = (byte)ConvertXElem(x, w, "ResErrColorR", ResErrColorR, 0xFF);
            r.ResErrColorG              = (byte)ConvertXElem(x, w, "ResErrColorG", ResErrColorG, 0x20);
            r.ResErrColorB              = (byte)ConvertXElem(x, w, "ResErrColorB", ResErrColorB, 0x20);
            r.ResWarColorA              = (byte)ConvertXElem(x, w, "ResWarColorA", ResWarColorA, 0x80);
            r.ResWarColorR              = (byte)ConvertXElem(x, w, "ResWarColorR", ResWarColorR, 0xFF);
            r.ResWarColorG              = (byte)ConvertXElem(x, w, "ResWarColorG", ResWarColorG, 0xFF);
            r.ResWarColorB              = (byte)ConvertXElem(x, w, "ResWarColorB", ResWarColorB, 0);
            r.ResNoColorA               = (byte)ConvertXElem(x, w, "ResNoColorA", ResNoColorA, 0x80);
            r.ResNoColorR               = (byte)ConvertXElem(x, w, "ResNoColorR", ResNoColorR, 0xA9);
            r.ResNoColorG               = (byte)ConvertXElem(x, w, "ResNoColorG", ResNoColorG, 0xA9);
            r.ResNoColorB               = (byte)ConvertXElem(x, w, "ResNoColorB", ResNoColorB, 0xA9);
            r.RecEndColorPosition       = (int)ConvertXElem(x, w, "RecEndColorPosition", RecEndColorPosition, 2);
            r.RecEndAlternationCount    = (int)ConvertXElem(x, w, "RecEndAlternationCount", RecEndAlternationCount, 2);
            r.RecEndDefColorA           = (byte)ConvertXElem(x, w, "RecEndDefColorA", RecEndDefColorA, 0x0C);
            r.RecEndDefColorR           = (byte)ConvertXElem(x, w, "RecEndDefColorR", RecEndDefColorR, 0x80);
            r.RecEndDefColorG           = (byte)ConvertXElem(x, w, "RecEndDefColorG", RecEndDefColorG, 0x80);
            r.RecEndDefColorB           = (byte)ConvertXElem(x, w, "RecEndDefColorB", RecEndDefColorB, 0x80);
            r.RecEndErrColorA           = (byte)ConvertXElem(x, w, "RecEndErrColorA", RecEndErrColorA, 0x80);
            r.RecEndErrColorR           = (byte)ConvertXElem(x, w, "RecEndErrColorR", RecEndErrColorR, 0xFF);
            r.RecEndErrColorG           = (byte)ConvertXElem(x, w, "RecEndErrColorG", RecEndErrColorG, 0x20);
            r.RecEndErrColorB           = (byte)ConvertXElem(x, w, "RecEndErrColorB", RecEndErrColorB, 0x20);
            r.RecEndWarColorA           = (byte)ConvertXElem(x, w, "RecEndWarColorA", RecEndWarColorA, 0x80);
            r.RecEndWarColorR           = (byte)ConvertXElem(x, w, "RecEndWarColorR", RecEndWarColorR, 0xFF);
            r.RecEndWarColorG           = (byte)ConvertXElem(x, w, "RecEndWarColorG", RecEndWarColorG, 0xFF);
            r.RecEndWarColorB           = (byte)ConvertXElem(x, w, "RecEndWarColorB", RecEndWarColorB, 0);
            r.ExecBat                   = (uint)ConvertXElem(x, w, "ExecBat", ExecBat, 0);
            r.SuspendChk                = (uint)ConvertXElem(x, w, "SuspendChk", SuspendChk, 0);
            r.ReserveListColumn         = ConvertXElements(x, w, "ReserveListColumn", ReserveListColumn).ToList();
            r.RecInfoListColumn         = ConvertXElements(x, w, "RecInfoListColumn", RecInfoListColumn).ToList();
            r.AutoAddEpgColumn          = ConvertXElements(x, w, "AutoAddEpgColumn", AutoAddEpgColumn).ToList();
            r.AutoAddManualColumn       = ConvertXElements(x, w, "AutoAddManualColumn", AutoAddManualColumn).ToList();
            r.SearchWndWidth            = ConvertXElem(x, w, "SearchWndWidth", SearchWndWidth, 0);
            r.SearchWndHeight           = ConvertXElem(x, w, "SearchWndHeight", SearchWndHeight, 0);
            r.NotifyLogMax              = (int)ConvertXElem(x, w, "NotifyLogMax", NotifyLogMax, 100);
            r.ShowTray                  = ConvertXElem(x, w, "ShowTray", ShowTray, false);
            r.MinHide                   = ConvertXElem(x, w, "MinHide", MinHide, true);
            r.NoStyle                   = (int)ConvertXElem(x, w, "NoStyle", NoStyle, 1);
            r.ApplyContextMenuStyle     = ConvertXElem(x, w, "ApplyContextMenuStyle", ApplyContextMenuStyle, false);
            r.NoSendClose               = (int)ConvertXElem(x, w, "NoSendClose", NoSendClose, 0);
            r.StartTab                  = ConvertXElem(x, w, "StartTab", StartTab, "ReserveView");
        }

        public Settings DeepCloneStaticSettings()
        {
            var other = (Settings)MemberwiseClone();
            other.CustomEpgTabList = CustomEpgTabList.Select(a => a.DeepClone()).ToList();
            other.EpgSettingList = EpgSettingList.Select(a => a.DeepClone()).ToList();
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
            dest.SearchWndNotKeyRatio = SearchWndNotKeyRatio;
            dest.AndKeyList = AndKeyList;
            dest.NotKeyList = NotKeyList;
            dest.RecInfoColumnHead = RecInfoColumnHead;
            dest.RecInfoSortDirection = RecInfoSortDirection;
            dest.RecInfoHideButton = RecInfoHideButton;
            dest.AutoAddEpgHideButton = AutoAddEpgHideButton;
            dest.AutoAddManualHideButton = AutoAddManualHideButton;
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
            set
            {
                _instance = value;
                _brushCache = null;
            }
        }

        private static SettingsBrushCache _brushCache;
        public static SettingsBrushCache BrushCache
        {
            get
            {
                if (_brushCache == null)
                {
                    _brushCache = new SettingsBrushCache();
                }
                return _brushCache;
            }
        }

        private static bool appResourceDictionaryInitialized;
        private static ResourceDictionary _appResourceDictionary;
        public static ResourceDictionary AppResourceDictionary
        {
            get
            {
                if (appResourceDictionaryInitialized == false)
                {
                    appResourceDictionaryInitialized = true;
                    if (Instance.NoStyle == 0)
                    {
                        try
                        {
                            string path = Path.Combine(SettingPath.ModulePath, SettingPath.ModuleName + ".rd.xaml");
                            if (File.Exists(path))
                            {
                                //ResourceDictionaryを定義したファイルがあるので本体にマージする
                                _appResourceDictionary = (ResourceDictionary)System.Windows.Markup.XamlReader.Load(System.Xml.XmlReader.Create(path));
                            }
                            else
                            {
                                //既定のテーマ(Aero)をマージする
                                _appResourceDictionary = (ResourceDictionary)Application.LoadComponent(
                                    new Uri("/PresentationFramework.Aero, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/aero.normalcolor.xaml", UriKind.Relative));
                            }
                        }
                        catch (Exception ex)
                        {
                            MessageBox.Show(ex.ToString());
                        }
                    }
                }
                return _appResourceDictionary;
            }
        }

        private static bool contextMenuResourceDictionaryInitialized;
        private static ResourceDictionary _contextMenuResourceDictionary;
        public static ResourceDictionary ContextMenuResourceDictionary
        {
            get
            {
                if (contextMenuResourceDictionaryInitialized == false)
                {
                    contextMenuResourceDictionaryInitialized = true;
                    if (Instance.ApplyContextMenuStyle)
                    {
                        try
                        {
                            string path = Path.Combine(SettingPath.ModulePath, SettingPath.ModuleName + ".rdcm.xaml");
                            if (File.Exists(path))
                            {
                                //ResourceDictionaryを定義したファイルがあるので本体にマージする
                                _contextMenuResourceDictionary = (ResourceDictionary)System.Windows.Markup.XamlReader.Load(System.Xml.XmlReader.Create(path));
                            }
                            else
                            {
                                //既定のテーマ(Aero)をマージする
                                _contextMenuResourceDictionary = (ResourceDictionary)Application.LoadComponent(
                                    new Uri("/PresentationFramework.Aero, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/aero.normalcolor.xaml", UriKind.Relative));
                            }
                        }
                        catch (Exception ex)
                        {
                            MessageBox.Show(ex.ToString());
                        }
                    }
                }
                return _contextMenuResourceDictionary;
            }
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
                    Thread.Sleep(200 * retry);
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
                    new ListColumnInfo("RecEnabled", double.NaN),
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
                        Thread.Sleep(200 * retry);
                    }
                }
            }
            catch (Exception ex)
            {
                if (notifyException)
                {
                    MessageBox.Show(ex.ToString());
                }
            }
        }

        private static string GetSettingPath()
        {
            return Path.Combine(SettingPath.ModulePath, SettingPath.ModuleName + ".xml");
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

        public static RecSettingData CreateRecSetting(uint presetID)
        {
            var defKey = new RecSettingData();
            string defName = "REC_DEF";
            string defFolderName = "REC_DEF_FOLDER";
            string defFolder1SegName = "REC_DEF_FOLDER_1SEG";

            if (presetID > 0)
            {
                defName += presetID.ToString();
                defFolderName += presetID.ToString();
                defFolder1SegName += presetID.ToString();
            }

            defKey.RecMode = (byte)IniFileHandler.GetPrivateProfileInt(defName, "RecMode", 1, SettingPath.TimerSrvIniPath);
            if (defKey.IsNoRec())
            {
                defKey.RecMode = (byte)(5 + (byte)(IniFileHandler.GetPrivateProfileInt(defName, "NoRecMode", 1, SettingPath.TimerSrvIniPath) + 4) % 5);
            }
            defKey.Priority = (byte)IniFileHandler.GetPrivateProfileInt(defName, "Priority", 2, SettingPath.TimerSrvIniPath);
            defKey.TuijyuuFlag = (byte)IniFileHandler.GetPrivateProfileInt(defName, "TuijyuuFlag", 1, SettingPath.TimerSrvIniPath);
            defKey.ServiceMode = (byte)IniFileHandler.GetPrivateProfileInt(defName, "ServiceMode", 0, SettingPath.TimerSrvIniPath);
            defKey.PittariFlag = (byte)IniFileHandler.GetPrivateProfileInt(defName, "PittariFlag", 0, SettingPath.TimerSrvIniPath);

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

            defKey.SuspendMode = (byte)IniFileHandler.GetPrivateProfileInt(defName, "SuspendMode", 0, SettingPath.TimerSrvIniPath);
            defKey.RebootFlag = (byte)IniFileHandler.GetPrivateProfileInt(defName, "RebootFlag", 0, SettingPath.TimerSrvIniPath);
            defKey.UseMargineFlag = (byte)IniFileHandler.GetPrivateProfileInt(defName, "UseMargineFlag", 0, SettingPath.TimerSrvIniPath);
            defKey.StartMargine = IniFileHandler.GetPrivateProfileInt(defName, "StartMargine", 0, SettingPath.TimerSrvIniPath);
            defKey.EndMargine = IniFileHandler.GetPrivateProfileInt(defName, "EndMargine", 0, SettingPath.TimerSrvIniPath);
            defKey.ContinueRecFlag = (byte)IniFileHandler.GetPrivateProfileInt(defName, "ContinueRec", 0, SettingPath.TimerSrvIniPath);
            defKey.PartialRecFlag = (byte)IniFileHandler.GetPrivateProfileInt(defName, "PartialRec", 0, SettingPath.TimerSrvIniPath);
            defKey.TunerID = (uint)IniFileHandler.GetPrivateProfileInt(defName, "TunerID", 0, SettingPath.TimerSrvIniPath);
            return defKey;
        }

        public EpgSearchKeyInfo CreateDefSearchSetting()
        {
            var defKey = new EpgSearchKeyInfo();
            defKey.andKey = SearchKeyAndKey;
            defKey.notKey = SearchKeyNotKey;
            defKey.regExpFlag = SearchKeyRegExp ? 1 : 0;
            defKey.aimaiFlag = (byte)(SearchKeyAimaiFlag ? 1 : 0);
            defKey.titleOnlyFlag = SearchKeyTitleOnly ? 1 : 0;
            defKey.notContetFlag = (byte)(SearchKeyNotContent ? 1 : 0);
            defKey.notDateFlag = (byte)(SearchKeyNotDate ? 1 : 0);
            foreach (ContentKindInfo info in SearchKeyContentList)
            {
                EpgContentData item = new EpgContentData();
                item.content_nibble_level_1 = info.Nibble1;
                item.content_nibble_level_2 = info.Nibble2;
                defKey.contentList.Add(item);
            }
            defKey.dateList.AddRange(SearchKeyDateItemList.Select(a => a.DeepClone()));
            defKey.serviceList.AddRange(SearchKeyServiceList);
            defKey.freeCAFlag = SearchKeyFreeCA;
            defKey.chkRecEnd = SearchKeyChkRecEnd;
            defKey.chkRecDay = SearchKeyChkRecDay;
            return defKey;
        }

        private void SetColorSetting()
        {
            foreach (EpgSetting epgSetting in EpgSettingList)
            {
                //番組表の背景色
                epgSetting.ContentColorList.AddRange((new string[] {
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
                    "White" }).Skip(Math.Min(epgSetting.ContentColorList.Count, 17)));
                //番組表の背景カスタム色(+4は番組表の予約枠色)
                epgSetting.ContentCustColorList.AddRange(Enumerable.Repeat(0xFFFFFFFF, Math.Max(17 + 4 - epgSetting.ContentCustColorList.Count, 0)));

                //番組表の時間軸のデフォルトの背景色
                epgSetting.TimeColorList.AddRange((new string[] {
                    "MediumPurple",
                    "LightSeaGreen",
                    "LightSalmon",
                    "CornflowerBlue" }).Skip(Math.Min(epgSetting.TimeColorList.Count, 4)));
                epgSetting.TimeCustColorList.AddRange(Enumerable.Repeat(0xFFFFFFFF, Math.Max(4 - epgSetting.TimeCustColorList.Count, 0)));
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

        private static IEnumerable<EpgSearchDateInfo> ConvertXElements(XElement x, bool w, string key, IEnumerable<EpgSearchDateInfo> list, bool asDateItem)
        {
            return ConvertXElements(x, w, key, list, (asDateItem ? "DateItem" : "EpgSearchDateInfo"), (xx, val) => {
                var xxDateItem = xx;
                if (asDateItem)
                {
                    //旧DateItemクラスと互換をとる
                    xx = w ? new XElement("DateInfo") : xxDateItem.Element("DateInfo");
                }
                var r = new EpgSearchDateInfo();
                val = val ?? r;
                r.startDayOfWeek = (byte)ConvertXElem(xx, w, "startDayOfWeek", val.startDayOfWeek, 0);
                r.startHour = (ushort)ConvertXElem(xx, w, "startHour", val.startHour, 0);
                r.startMin = (ushort)ConvertXElem(xx, w, "startMin", val.startMin, 0);
                r.endDayOfWeek = (byte)ConvertXElem(xx, w, "endDayOfWeek", val.endDayOfWeek, 0);
                r.endHour = (ushort)ConvertXElem(xx, w, "endHour", val.endHour, 0);
                r.endMin = (ushort)ConvertXElem(xx, w, "endMin", val.endMin, 0);
                if (asDateItem && w)
                {
                    xxDateItem.Add(xx);
                }
                return val;
            });
        }

        private static IEnumerable<CustomEpgTabInfo> ConvertXElements(XElement x, bool w, string key, IEnumerable<CustomEpgTabInfo> list)
        {
            return ConvertXElements(x, w, key, list, "CustomEpgTabInfo", (xx, val) => {
                var r = new CustomEpgTabInfo();
                val = val ?? r;
                r.TabName = ConvertXElem(xx, w, "TabName", val.TabName, "");
                r.EpgSettingIndex = (int)ConvertXElem(xx, w, "EpgSettingIndex", val.EpgSettingIndex, 0);
                r.ViewMode = (int)ConvertXElem(xx, w, "ViewMode", val.ViewMode, 0);
                r.NeedTimeOnlyBasic = ConvertXElem(xx, w, "NeedTimeOnlyBasic", val.NeedTimeOnlyBasic, false);
                r.NeedTimeOnlyWeek = ConvertXElem(xx, w, "NeedTimeOnlyWeek", val.NeedTimeOnlyWeek, false);
                r.StartTimeWeek = (int)ConvertXElem(xx, w, "StartTimeWeek", val.StartTimeWeek, 0);
                r.ViewServiceList = ConvertXElements(xx, w, "ViewServiceList",
                    val.ViewServiceList.Select(a => (double)a), "unsignedLong").Select(a => (ulong)a).ToList();
                r.ViewContentKindList = ConvertXElements(xx, w, "ViewContentKindList",
                    val.ViewContentKindList.Select(a => (double)a), "unsignedShort").Select(a => (ushort)a).ToList();
                r.HighlightContentKind = ConvertXElem(xx, w, "HighlightContentKind", val.HighlightContentKind, false);
                r.SearchMode = ConvertXElem(xx, w, "SearchMode", val.SearchMode, false);

                var xxx = w ? new XElement("SearchKey") : xx.Element("SearchKey");
                r.SearchKey.andKey = ConvertXElem(xxx, w, "andKey", val.SearchKey.andKey, "");
                r.SearchKey.notKey = ConvertXElem(xxx, w, "notKey", val.SearchKey.notKey, "");
                r.SearchKey.regExpFlag = (int)ConvertXElem(xxx, w, "regExpFlag", val.SearchKey.regExpFlag, 0);
                r.SearchKey.titleOnlyFlag = (int)ConvertXElem(xxx, w, "titleOnlyFlag", val.SearchKey.titleOnlyFlag, 0);
                r.SearchKey.contentList = ConvertXElements(xxx, w, "contentList", val.SearchKey.contentList).ToList();
                r.SearchKey.dateList = ConvertXElements(xxx, w, "dateList", val.SearchKey.dateList, false).ToList();
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

        public class SettingsBrushCache
        {
            private List<Brush> _contentBrushList;
            public List<Brush> ContentBrushList
            {
                get
                {
                    if (_contentBrushList == null)
                    {
                        _contentBrushList = new List<Brush>();
                        for (int i = 0; i < Instance.EpgSettingList[0].ContentColorList.Count; i++)
                        {
                            SolidColorBrush brush = ColorDef.CustColorBrush(Instance.EpgSettingList[0].ContentColorList[i],
                                                                            Instance.EpgSettingList[0].ContentCustColorList[i]);
                            _contentBrushList.Add(Instance.EpgSettingList[0].EpgGradation ? (Brush)ColorDef.GradientBrush(brush.Color) : brush);
                        }
                    }
                    return _contentBrushList;
                }
            }

            private static SolidColorBrush GetBrush(ref SolidColorBrush brush, byte a, byte r, byte g, byte b)
            {
                if (brush == null)
                {
                    brush = new SolidColorBrush(Color.FromArgb(a, r, g, b));
                    brush.Freeze();
                }
                return brush;
            }

            private SolidColorBrush _resDefBrush;
            public SolidColorBrush ResDefBrush
            {
                get { return GetBrush(ref _resDefBrush, Instance.ResDefColorA, Instance.ResDefColorR, Instance.ResDefColorG, Instance.ResDefColorB); }
            }

            private SolidColorBrush _resErrBrush;
            public SolidColorBrush ResErrBrush
            {
                get { return GetBrush(ref _resErrBrush, Instance.ResErrColorA, Instance.ResErrColorR, Instance.ResErrColorG, Instance.ResErrColorB); }
            }

            private SolidColorBrush _resWarBrush;
            public SolidColorBrush ResWarBrush
            {
                get { return GetBrush(ref _resWarBrush, Instance.ResWarColorA, Instance.ResWarColorR, Instance.ResWarColorG, Instance.ResWarColorB); }
            }

            private SolidColorBrush _resNoBrush;
            public SolidColorBrush ResNoBrush
            {
                get { return GetBrush(ref _resNoBrush, Instance.ResNoColorA, Instance.ResNoColorR, Instance.ResNoColorG, Instance.ResNoColorB); }
            }

            private SolidColorBrush _recEndDefBrush;
            public SolidColorBrush RecEndDefBrush
            {
                get { return GetBrush(ref _recEndDefBrush, Instance.RecEndDefColorA, Instance.RecEndDefColorR, Instance.RecEndDefColorG, Instance.RecEndDefColorB); }
            }

            private SolidColorBrush _recEndErrBrush;
            public SolidColorBrush RecEndErrBrush
            {
                get { return GetBrush(ref _recEndErrBrush, Instance.RecEndErrColorA, Instance.RecEndErrColorR, Instance.RecEndErrColorG, Instance.RecEndErrColorB); }
            }

            private SolidColorBrush _recEndWarBrush;
            public SolidColorBrush RecEndWarBrush
            {
                get { return GetBrush(ref _recEndWarBrush, Instance.RecEndWarColorA, Instance.RecEndWarColorR, Instance.RecEndWarColorG, Instance.RecEndWarColorB); }
            }
        }
    }
}
