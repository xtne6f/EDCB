﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Reflection;
using System.Collections;
using Microsoft.Win32;
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

        public static void UpdateSrvProfileIniNW(List<string> iniList = null)
        {
            if (CommonManager.Instance.NW.IsConnected == false) return;

            ReloadSettingFilesNW(iniList);

            ChSet5.Clear();
            Settings.Instance.RecPresetList = null;
            Settings.Instance.ReloadOtherOptions();
        }

        public static void ReloadSettingFilesNW(List<string> iniList = null)
        {
            if (iniList == null)
            {
                iniList = new List<string> {
                    "EpgTimerSrv.ini"
                    ,"Common.ini"
                    ,"EpgDataCap_Bon.ini"
                    ,"ChSet5.txt"
                };
            }

            try
            {
                var datalist = new List<FileData>();
                if (CommonManager.Instance.CtrlCmd.SendFileCopy2(iniList, ref datalist) == ErrCode.CMD_SUCCESS)
                {
                    Directory.CreateDirectory(SettingPath.SettingFolderPath);
                    foreach (var data in datalist.Where(d1 => d1.Size != 0))
                    {
                        try
                        {
                            using (var w = new BinaryWriter(File.Create(Path.Combine(SettingPath.SettingFolderPath, data.Name))))
                            {
                                w.Write(data.Data);
                            }
                        }
                        catch { }
                    }
                }
            }
            catch { }
        }

    }

    class SettingPath
    {
        private static string IniPath
        {
            get { return (CommonManager.Instance.NWMode == false ? ModulePath : SettingFolderPath); }
        }
        public static string CommonIniPath
        {
            get { return IniPath.TrimEnd('\\') + "\\Common.ini"; }
        }
        public static string TimerSrvIniPath
        {
            get { return IniPath.TrimEnd('\\') + "\\EpgTimerSrv.ini"; }
        }
        public static string EdcbExePath
        {
            get
            {
                string defRecExe = ModulePath.TrimEnd('\\') + "\\EpgDataCap_Bon.exe";
                return IniFileHandler.GetPrivateProfileString("SET", "RecExePath", defRecExe, CommonIniPath);
            }
        }
        public static string EdcbIniPath
        {
            get
            {
                if (CommonManager.Instance.NWMode == false)
                {
                    return EdcbExePath.TrimEnd("exe".ToArray()) + "ini";
                }
                else
                {
                    return IniPath.TrimEnd('\\') + "\\EpgDataCap_Bon.ini";
                }
            }
        }
        public static string DefSettingFolderPath
        {
            get
            {
                return ModulePath.TrimEnd('\\') + "\\Setting" + (CommonManager.Instance.NWMode == false ? "" : "NW");
            }
        }
        public static string SettingFolderPath
        {
            get
            {
                if (CommonManager.Instance.NWMode == false)
                {
                    string path = IniFileHandler.GetPrivateProfileString("SET", "DataSavePath", DefSettingFolderPath, CommonIniPath);
                    return (Path.IsPathRooted(path) ? "" : ModulePath.TrimEnd('\\') + "\\") + path;
                }
                else
                {
                    return DefSettingFolderPath;
                }
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
            get { return Path.GetFileNameWithoutExtension(Assembly.GetExecutingAssembly().Location); }
        }
    }

    public class Settings
    {
        private bool useCustomEpgView;
        private List<CustomEpgTabInfo> customEpgTabList;
        private double minHeight;
        private double minimumHeight;
        private double serviceWidth;
        private double scrollSize;
        private string fontName;
        private double fontSize;
        private string fontNameTitle;
        private double fontSizeTitle;
        private bool fontBoldTitle;
        private bool noToolTip;
        private bool noBallonTips;
        private int forceHideBalloonTipSec;
        private bool playDClick;
        private bool recinfoErrCriticalDrops;
        private double dragScroll;
        private List<string> contentColorList;
        private List<UInt32> contentCustColorList;
        private List<string> epgEtcColors;
        private List<UInt32> epgEtcCustColors;
        private string reserveRectColorNormal;
        private string reserveRectColorNo;
        private string reserveRectColorNoTuner;
        private string reserveRectColorWarning;
        private string reserveRectColorAutoAddMissing;
        private string titleColor1;
        private string titleColor2;
        private UInt32 titleCustColor1;
        private UInt32 titleCustColor2;
        private bool reserveRectBackground;
        private string tunerFontNameService;
        private double tunerFontSizeService;
        private bool tunerFontBoldService;
        private string tunerFontName;
        private double tunerFontSize;
        private List<string> tunerServiceColors;
        private List<UInt32> tunerServiceCustColors;
        private double tunerMinHeight;
        private double tunerMinimumLine;
        private double tunerDragScroll;
        private double tunerScrollSize;
        private bool tunerMouseScrollAuto;
        private double tunerWidth;
        private bool tunerServiceNoWrap;
        private bool tunerTitleIndent;
        private bool tunerPopup;
        private bool tunerPopupRecinfo;
        private bool tunerInfoSingleClick;
        private bool tunerColorModeUse;
        private bool tunerDisplayOffReserve;
        private bool epgTitleIndent;
        private bool epgPopup;
        private bool epgPopupResOnly;
        private bool epgGradation;
        private bool epgGradationHeader;
        private string resColumnHead;
        private ListSortDirection resSortDirection;
        private System.Windows.WindowState lastWindowState;
        private double mainWndLeft;
        private double mainWndTop;
        private double mainWndWidth;
        private double mainWndHeight;
        private bool closeMin;
        private bool wakeMin;
        private bool viewButtonShowAsTab;
        private List<string> viewButtonList;
        private List<string> taskMenuList;
        private string cust1BtnName;
        private string cust1BtnCmd;
        private string cust1BtnCmdOpt;
        private string cust2BtnName;
        private string cust2BtnCmd;
        private string cust2BtnCmdOpt;
        private List<string> andKeyList;
        private List<string> notKeyList;
        private EpgSearchKeyInfo defSearchKey;
        private List<RecPresetItem> recPresetList;
        private string recInfoColumnHead;
        private ListSortDirection recInfoSortDirection;
        private long recInfoDropErrIgnore;
        private long recInfoDropWrnIgnore;
        private long recInfoScrambleIgnore;
        private List<string> recInfoDropExclude;
        private bool recInfoNoYear;
        private bool recInfoNoSecond;
        private bool recInfoNoDurSecond;
        private bool resInfoNoYear;
        private bool resInfoNoSecond;
        private bool resInfoNoDurSecond;
        private string tvTestExe;
        private string tvTestCmd;
        private bool nwTvMode;
        private bool nwTvModeUDP;
        private bool nwTvModeTCP;
        private string filePlayExe;
        private string filePlayCmd;
        private bool filePlayOnAirWithExe;
        private bool openFolderWithFileDialog;
        private List<IEPGStationInfo> iEpgStationList;
        private MenuSettingData menuSet;
        private string nwServerIP;
        private UInt32 nwServerPort;
        private UInt32 nwWaitPort;
        private string nwMacAdd;
        private List<NWPresetItem> nwPreset; 
        private bool wakeReconnectNW;
        private bool suspendCloseNW;
        private bool ngAutoEpgLoadNW;
        private bool chkSrvRegistTCP;
        private double chkSrvRegistInterval;
        private Int32 tvTestOpenWait;
        private Int32 tvTestChgBonWait;
        private List<string> recEndColors;          //録画済リストの背景色
        private List<UInt32> recEndCustColors;      //録画済リストのカスタム背景色
        private string listDefColor;                //各画面のリストのデフォルト文字色
        private UInt32 listDefCustColor;            //各画面のリストのデフォルトカスタム文字色
        private List<string> recModeFontColors;     //予約リストなどの録画モードごとの文字色
        private List<UInt32> recModeFontCustColors; //予約リストなどの録画モードごとのカスタム文字色
        private List<string> resBackColors;         //予約リストなどの背景色
        private List<UInt32> resBackCustColors;     //予約リストなどのカスタム背景色
        private List<string> statColors;            //予約リストなどの「状態」列の予約色
        private List<UInt32> statCustColors;        //予約リストなどの「状態」列のカスタム予約色
        private bool epgInfoSingleClick;
        private byte epgInfoOpenMode;
        private UInt32 execBat;
        private UInt32 suspendChk;
        private UInt32 suspendChkTime;
        private List<ListColumnInfo> reserveListColumn;
        private List<ListColumnInfo> recInfoListColumn;
        private List<ListColumnInfo> autoAddEpgColumn;
        private List<ListColumnInfo> autoAddManualColumn;
        private double searchWndLeft;
        private double searchWndTop;
        private double searchWndWidth;
        private double searchWndHeight;
        private bool searchWndPinned;
        private short autoSaveNotifyLog;
        private bool showTray;
        private bool minHide;
        private bool mouseScrollAuto;
        private int noStyle;
        private bool cautionManyChange;
        private int cautionManyNum;
        private bool cautionOnRecChange;
        private int cautionOnRecMarginMin;
        private bool syncResAutoAddChange;
        private bool syncResAutoAddChgNewRes;
        private bool syncResAutoAddDelete;
        private int keyDeleteDisplayItemNum;
        private bool displayNotifyEpgChange;
        private double displayNotifyJumpTime;
        private bool displayReserveAutoAddMissing;
        private bool tryEpgSetting;
        private bool laterTimeUse;
        private int laterTimeHour;

        public bool UseCustomEpgView
        {
            get { return useCustomEpgView; }
            set { useCustomEpgView = value; }
        }
        public List<CustomEpgTabInfo> CustomEpgTabList
        {
            get { return customEpgTabList; }
            set { customEpgTabList = value; }
        }
        public double MinHeight
        {
            get { return minHeight; }
            set { minHeight = value; }
        }
        public double ServiceWidth
        {
            get { return serviceWidth; }
            set { serviceWidth = value; }
        }
        public double ScrollSize
        {
            get { return scrollSize; }
            set { scrollSize = value; }
        }
        public double MinimumHeight
        {
            get { return minimumHeight; }
            set { minimumHeight = value; }
        }
        public string FontName
        {
            get { return fontName; }
            set { fontName = value; }
        }
        public double FontSize
        {
            get { return fontSize; }
            set { fontSize = value; }
        }
        public string FontNameTitle
        {
            get { return fontNameTitle; }
            set { fontNameTitle = value; }
        }
        public double FontSizeTitle
        {
            get { return fontSizeTitle; }
            set { fontSizeTitle = value; }
        }
        public bool FontBoldTitle
        {
            get { return fontBoldTitle; }
            set { fontBoldTitle = value; }
        }
        public bool NoToolTip
        {
            get { return noToolTip; }
            set { noToolTip = value; }
        }
        public bool NoBallonTips
        {
            get { return noBallonTips; }
            set { noBallonTips = value; }
        }
        public int ForceHideBalloonTipSec
        {
            get { return forceHideBalloonTipSec; }
            set { forceHideBalloonTipSec = value; }
        }
        public bool PlayDClick
        {
            get { return playDClick; }
            set { playDClick = value; }
        }
        public bool RecinfoErrCriticalDrops
        {
            get { return recinfoErrCriticalDrops; }
            set { recinfoErrCriticalDrops = value; }
        }        
        public double DragScroll
        {
            get { return dragScroll; }
            set { dragScroll = value; }
        }
        public List<string> ContentColorList
        {
            get { return contentColorList; }
            set { contentColorList = value; }
        }
        public List<UInt32> ContentCustColorList
        {
            get { return contentCustColorList; }
            set { contentCustColorList = value; }
        }
        public List<string> EpgEtcColors
        {
            get { return epgEtcColors; }
            set { epgEtcColors = value; }
        }
        public List<UInt32> EpgEtcCustColors
        {
            get { return epgEtcCustColors; }
            set { epgEtcCustColors = value; }
        }
        public string ReserveRectColorNormal
        {
            get { return reserveRectColorNormal; }
            set { reserveRectColorNormal = value; }
        }
        public string ReserveRectColorNo
        {
            get { return reserveRectColorNo; }
            set { reserveRectColorNo = value; }
        }
        public string ReserveRectColorNoTuner
        {
            get { return reserveRectColorNoTuner; }
            set { reserveRectColorNoTuner = value; }
        }
        public string ReserveRectColorWarning
        {
            get { return reserveRectColorWarning; }
            set { reserveRectColorWarning = value; }
        }
        public string ReserveRectColorAutoAddMissing
        {
            get { return reserveRectColorAutoAddMissing; }
            set { reserveRectColorAutoAddMissing = value; }
        }
        public bool ReserveRectBackground
        {
            get { return reserveRectBackground; }
            set { reserveRectBackground = value; }
        }
        public string TitleColor1
        {
            get { return titleColor1; }
            set { titleColor1 = value; }
        }
        public string TitleColor2
        {
            get { return titleColor2; }
            set { titleColor2 = value; }
        }
        public UInt32 TitleCustColor1
        {
            get { return titleCustColor1; }
            set { titleCustColor1 = value; }
        }
        public UInt32 TitleCustColor2
        {
            get { return titleCustColor2; }
            set { titleCustColor2 = value; }
        }
        public string TunerFontNameService
        {
            get { return tunerFontNameService; }
            set { tunerFontNameService = value; }
        }
        public double TunerFontSizeService
        {
            get { return tunerFontSizeService; }
            set { tunerFontSizeService = value; }
        }
        public bool TunerFontBoldService
        {
            get { return tunerFontBoldService; }
            set { tunerFontBoldService = value; }
        }
        public string TunerFontName
        {
            get { return tunerFontName; }
            set { tunerFontName = value; }
        }
        public double TunerFontSize
        {
            get { return tunerFontSize; }
            set { tunerFontSize = value; }
        }
        public List<string> TunerServiceColors
        {
            get { return tunerServiceColors; }
            set { tunerServiceColors = value; }
        }
        public List<UInt32> TunerServiceCustColors
        {
            get { return tunerServiceCustColors; }
            set { tunerServiceCustColors = value; }
        }
        public double TunerMinHeight
        {
            get { return tunerMinHeight; }
            set { tunerMinHeight = value; }
        }
        public double TunerMinimumLine
        {
            get { return tunerMinimumLine; }
            set { tunerMinimumLine = value; }
        }
        public double TunerDragScroll
        {
            get { return tunerDragScroll; }
            set { tunerDragScroll = value; }
        }
        public double TunerScrollSize
        {
            get { return tunerScrollSize; }
            set { tunerScrollSize = value; }
        }
        public bool TunerMouseScrollAuto
        {
            get { return tunerMouseScrollAuto; }
            set { tunerMouseScrollAuto = value; }
        }
        public double TunerWidth
        {
            get { return tunerWidth; }
            set { tunerWidth = value; }
        }
        public bool TunerServiceNoWrap
        {
            get { return tunerServiceNoWrap; }
            set { tunerServiceNoWrap = value; }
        }
        public bool TunerTitleIndent
        {
            get { return tunerTitleIndent; }
            set { tunerTitleIndent = value; }
        }
        public bool TunerPopup
        {
            get { return tunerPopup; }
            set { tunerPopup = value; }
        }
        public bool TunerPopupRecinfo
        {
            get { return tunerPopupRecinfo; }
            set { tunerPopupRecinfo = value; }
        }
        public bool TunerInfoSingleClick
        {
            get { return tunerInfoSingleClick; }
            set { tunerInfoSingleClick = value; }
        }
        public bool TunerColorModeUse
        {
            get { return tunerColorModeUse; }
            set { tunerColorModeUse = value; }
        }
        public bool TunerDisplayOffReserve
        {
            get { return tunerDisplayOffReserve; }
            set { tunerDisplayOffReserve = value; }
        }
        public bool EpgTitleIndent
        {
            get { return epgTitleIndent; }
            set { epgTitleIndent = value; }
        }
        public bool EpgPopup
        {
            get { return epgPopup; }
            set { epgPopup = value; }
        }
        public bool EpgPopupResOnly
        {
            get { return epgPopupResOnly; }
            set { epgPopupResOnly = value; }
        }
        public bool EpgGradation
        {
            get { return epgGradation; }
            set { epgGradation = value; }
        }
        public bool EpgGradationHeader
        {
            get { return epgGradationHeader; }
            set { epgGradationHeader = value; }
        }
        public string ResColumnHead
        {
            get { return resColumnHead; }
            set { resColumnHead = value; }
        }
        public ListSortDirection ResSortDirection
        {
            get { return resSortDirection; }
            set { resSortDirection = value; }
        }
        public System.Windows.WindowState LastWindowState
        {
            get { return lastWindowState; }
            set { lastWindowState = value; }
        }
        public double MainWndLeft
        {
            get { return mainWndLeft; }
            set { mainWndLeft = value; }
        }
        public double MainWndTop
        {
            get { return mainWndTop; }
            set { mainWndTop = value; }
        }
        public double MainWndWidth
        {
            get { return mainWndWidth; }
            set { mainWndWidth = value; }
        }
        public double MainWndHeight
        {
            get { return mainWndHeight; }
            set { mainWndHeight = value; }
        }
        public bool CloseMin
        {
            get { return closeMin; }
            set { closeMin = value; }
        }
        public bool WakeMin
        {
            get { return wakeMin; }
            set { wakeMin = value; }
        }
        public bool ViewButtonShowAsTab
        {
            get { return viewButtonShowAsTab; }
            set { viewButtonShowAsTab = value; }
        }
        public List<string> ViewButtonList
        {
            get { return viewButtonList; }
            set { viewButtonList = value; }
        }
        public List<string> TaskMenuList
        {
            get { return taskMenuList; }
            set { taskMenuList = value; }
        }
        public string Cust1BtnName
        {
            get { return cust1BtnName; }
            set { cust1BtnName = value; }
        }
        public string Cust1BtnCmd
        {
            get { return cust1BtnCmd; }
            set { cust1BtnCmd = value; }
        }
        public string Cust1BtnCmdOpt
        {
            get { return cust1BtnCmdOpt; }
            set { cust1BtnCmdOpt = value; }
        }
        public string Cust2BtnName
        {
            get { return cust2BtnName; }
            set { cust2BtnName = value; }
        }
        public string Cust2BtnCmd
        {
            get { return cust2BtnCmd; }
            set { cust2BtnCmd = value; }
        }
        public string Cust2BtnCmdOpt
        {
            get { return cust2BtnCmdOpt; }
            set { cust2BtnCmdOpt = value; }
        }
        public List<string> AndKeyList
        {
            get { return andKeyList; }
            set { andKeyList = value; }
        }
        public List<string> NotKeyList
        {
            get { return notKeyList; }
            set { notKeyList = value; }
        }
        public EpgSearchKeyInfo DefSearchKey
        {
            get { return defSearchKey; }
            set { defSearchKey = value; }
        }
        [System.Xml.Serialization.XmlIgnore]
        public List<RecPresetItem> RecPresetList
        {
            get
            {
                if (recPresetList == null)
                {
                    recPresetList = new List<RecPresetItem>();
                    recPresetList.Add(new RecPresetItem("デフォルト", 0));
                    foreach (string s in IniFileHandler.GetPrivateProfileString("SET", "PresetID", "", SettingPath.TimerSrvIniPath).Split(','))
                    {
                        uint id;
                        uint.TryParse(s, out id);
                        if (recPresetList.Exists(p => p.ID == id) == false)
                        {
                            recPresetList.Add(new RecPresetItem(
                                IniFileHandler.GetPrivateProfileString("REC_DEF" + id, "SetName", "", SettingPath.TimerSrvIniPath)
                                , id));
                        }
                    }
                }
                return recPresetList;
            }
            set { recPresetList = value; }
        }
        public string RecInfoColumnHead
        {
            get { return recInfoColumnHead; }
            set { recInfoColumnHead = value; }
        }
        public ListSortDirection RecInfoSortDirection
        {
            get { return recInfoSortDirection; }
            set { recInfoSortDirection = value; }
        }
        public long RecInfoDropErrIgnore
        {
            get { return recInfoDropErrIgnore; }
            set { recInfoDropErrIgnore = value; }
        }
        public long RecInfoDropWrnIgnore
        {
            get { return recInfoDropWrnIgnore; }
            set { recInfoDropWrnIgnore = value; }
        }
        public long RecInfoScrambleIgnore
        {
            get { return recInfoScrambleIgnore; }
            set { recInfoScrambleIgnore = value; }
        }
        public List<string> RecInfoDropExclude
        {
            get { return recInfoDropExclude; }
            set { recInfoDropExclude = value; }
        }
        public bool RecInfoNoYear
        {
            get { return recInfoNoYear; }
            set { recInfoNoYear = value; }
        }
        public bool RecInfoNoSecond
        {
            get { return recInfoNoSecond; }
            set { recInfoNoSecond = value; }
        }
        public bool RecInfoNoDurSecond
        {
            get { return recInfoNoDurSecond; }
            set { recInfoNoDurSecond = value; }
        }
        public bool ResInfoNoYear
        {
            get { return resInfoNoYear; }
            set { resInfoNoYear = value; }
        }
        public bool ResInfoNoSecond
        {
            get { return resInfoNoSecond; }
            set { resInfoNoSecond = value; }
        }
        public bool ResInfoNoDurSecond
        {
            get { return resInfoNoDurSecond; }
            set { resInfoNoDurSecond = value; }
        }
        public string TvTestExe
        {
            get { return tvTestExe; }
            set { tvTestExe = value; }
        }
        public string TvTestCmd
        {
            get { return tvTestCmd; }
            set { tvTestCmd = value; }
        }
        public bool NwTvMode
        {
            get { return nwTvMode; }
            set { nwTvMode = value; }
        }
        public bool NwTvModeUDP
        {
            get { return nwTvModeUDP; }
            set { nwTvModeUDP = value; }
        }
        public bool NwTvModeTCP
        {
            get { return nwTvModeTCP; }
            set { nwTvModeTCP = value; }
        }
        public string FilePlayExe
        {
            get { return filePlayExe; }
            set { filePlayExe = value; }
        }
        public string FilePlayCmd
        {
            get { return filePlayCmd; }
            set { filePlayCmd = value; }
        }
        public bool FilePlayOnAirWithExe
        {
            get { return filePlayOnAirWithExe; }
            set { filePlayOnAirWithExe = value; }
        }
        public bool OpenFolderWithFileDialog
        {
            get { return openFolderWithFileDialog; }
            set { openFolderWithFileDialog = value; }
        }
        public List<IEPGStationInfo> IEpgStationList
        {
            get { return iEpgStationList; }
            set { iEpgStationList = value; }
        }
        public MenuSettingData MenuSet
        {
            get { return menuSet; }
            set { menuSet = value; }
        }
        public string NWServerIP
        {
            get { return nwServerIP; }
            set { nwServerIP = value; }
        }
        public UInt32 NWServerPort
        {
            get { return nwServerPort; }
            set { nwServerPort = value; }
        }
        public UInt32 NWWaitPort
        {
            get { return nwWaitPort; }
            set { nwWaitPort = value; }
        }
        public string NWMacAdd
        {
            get { return nwMacAdd; }
            set { nwMacAdd = value; }
        }
        public List<NWPresetItem> NWPerset
        {
            get { return nwPreset; }
            set { nwPreset = value; }
        }
        public bool WakeReconnectNW
        {
            get { return wakeReconnectNW; }
            set { wakeReconnectNW = value; }
        }
        public bool SuspendCloseNW
        {
            get { return suspendCloseNW; }
            set { suspendCloseNW = value; }
        }
        public bool NgAutoEpgLoadNW
        {
            get { return ngAutoEpgLoadNW; }
            set { ngAutoEpgLoadNW = value; }
        }
        public bool ChkSrvRegistTCP
        {
            get { return chkSrvRegistTCP; }
            set { chkSrvRegistTCP = value; }
        }
        public double ChkSrvRegistInterval
        {
            get { return chkSrvRegistInterval; }
            set { chkSrvRegistInterval = value; }
        }
        public Int32 TvTestOpenWait
        {
            get { return tvTestOpenWait; }
            set { tvTestOpenWait = value; }
        }
        public Int32 TvTestChgBonWait
        {
            get { return tvTestChgBonWait; }
            set { tvTestChgBonWait = value; }
        }
        public List<string> RecEndColors
        {
            get { return recEndColors; }
            set { recEndColors = value; }
        }
        public List<uint> RecEndCustColors
        {
            get { return recEndCustColors; }
            set { recEndCustColors = value; }
        }
        public string ListDefColor
        {
            get { return listDefColor; }
            set { listDefColor = value; }
        }
        public UInt32 ListDefCustColor
        {
            get { return listDefCustColor; }
            set { listDefCustColor = value; }
        }
        public List<string> RecModeFontColors
        {
            get { return recModeFontColors; }
            set { recModeFontColors = value; }
        }
        public List<uint> RecModeFontCustColors
        {
            get { return recModeFontCustColors; }
            set { recModeFontCustColors = value; }
        }
        public List<string> ResBackColors
        {
            get { return resBackColors; }
            set { resBackColors = value; }
        }
        public List<uint> ResBackCustColors
        {
            get { return resBackCustColors; }
            set { resBackCustColors = value; }
        }
        public List<string> StatColors
        {
            get { return statColors; }
            set { statColors = value; }
        }
        public List<uint> StatCustColors
        {
            get { return statCustColors; }
            set { statCustColors = value; }
        }
        public bool EpgInfoSingleClick
        {
            get { return epgInfoSingleClick; }
            set { epgInfoSingleClick = value; }
        }
        public byte EpgInfoOpenMode
        {
            get { return epgInfoOpenMode; }
            set { epgInfoOpenMode = value; }
        }
        public UInt32 ExecBat
        {
            get { return execBat; }
            set { execBat = value; }
        }
        public UInt32 SuspendChk
        {
            get { return suspendChk; }
            set { suspendChk = value; }
        }
        public UInt32 SuspendChkTime
        {
            get { return suspendChkTime; }
            set { suspendChkTime = value; }
        }
        public List<ListColumnInfo> ReserveListColumn
        {
            get { return reserveListColumn; }
            set { reserveListColumn = value; }
        }
        public List<ListColumnInfo> RecInfoListColumn
        {
            get { return recInfoListColumn; }
            set { recInfoListColumn = value; }
        }
        public List<ListColumnInfo> AutoAddEpgColumn
        {
            get { return autoAddEpgColumn; }
            set { autoAddEpgColumn = value; }
        }
        public List<ListColumnInfo> AutoAddManualColumn
        {
            get { return autoAddManualColumn; }
            set { autoAddManualColumn = value; }
        }
        public double SearchWndLeft
        {
            get { return searchWndLeft; }
            set { searchWndLeft = value; }
        }
        public double SearchWndTop
        {
            get { return searchWndTop; }
            set { searchWndTop = value; }
        }
        public double SearchWndWidth
        {
            get { return searchWndWidth; }
            set { searchWndWidth = value; }
        }
        public double SearchWndHeight
        {
            get { return searchWndHeight; }
            set { searchWndHeight = value; }
        }
        public bool SearchWndPinned
        {
            get { return searchWndPinned; }
            set { searchWndPinned = value; }
        }
        public short AutoSaveNotifyLog
        {
            get { return autoSaveNotifyLog; }
            set { autoSaveNotifyLog = value; }
        }
        public bool ShowTray
        {
            get { return showTray; }
            set { showTray = value; }
        }
        public bool MinHide
        {
            get { return minHide; }
            set { minHide = value; }
        }
        public bool MouseScrollAuto
        {
            get { return mouseScrollAuto; }
            set { mouseScrollAuto = value; }
        }
        public int NoStyle
        {
            get { return noStyle; }
            set { noStyle = value; }
        }
        public bool CautionManyChange
        {
            get { return cautionManyChange; }
            set { cautionManyChange = value; }
        }
        public int CautionManyNum
        {
            get { return cautionManyNum; }
            set { cautionManyNum = value; }
        }
        public bool CautionOnRecChange
        {
            get { return cautionOnRecChange; }
            set { cautionOnRecChange = value; }
        }
        public int CautionOnRecMarginMin
        {
            get { return cautionOnRecMarginMin; }
            set { cautionOnRecMarginMin = value; }
        }
        public bool SyncResAutoAddChange
        {
            get { return syncResAutoAddChange; }
            set { syncResAutoAddChange = value; }
        }
        public bool SyncResAutoAddChgNewRes
        {
            get { return syncResAutoAddChgNewRes; }
            set { syncResAutoAddChgNewRes = value; }
        }
        public bool SyncResAutoAddDelete
        {
            get { return syncResAutoAddDelete; }
            set { syncResAutoAddDelete = value; }
        }
        public int KeyDeleteDisplayItemNum
        {
            get { return keyDeleteDisplayItemNum; }
            set { keyDeleteDisplayItemNum = value; }
        }        
        public bool DisplayNotifyEpgChange
        {
            get { return displayNotifyEpgChange; }
            set { displayNotifyEpgChange = value; }
        }
        public double DisplayNotifyJumpTime
        {
            get { return displayNotifyJumpTime; }
            set { displayNotifyJumpTime = value; }
        }
        public bool DisplayReserveAutoAddMissing
        {
            get { return displayReserveAutoAddMissing; }
            set { displayReserveAutoAddMissing = value; }
        }
        public bool TryEpgSetting
        {
            get { return tryEpgSetting; }
            set { tryEpgSetting = value; }
        }
        public bool LaterTimeUse
        {
            get { return laterTimeUse; }
            set { laterTimeUse = value; }
        }
        public int LaterTimeHour
        {
            get { return laterTimeHour; }
            set { laterTimeHour = value; }
        }
        
        public Settings()
        {
            useCustomEpgView = false;
            customEpgTabList = new List<CustomEpgTabInfo>();
            minHeight = 2;
            minimumHeight = 0;
            serviceWidth = 150;
            scrollSize = 240;
            fontName = System.Drawing.SystemFonts.DefaultFont.Name;
            fontSize = 12;
            fontNameTitle = System.Drawing.SystemFonts.DefaultFont.Name;
            fontSizeTitle = 12;
            fontBoldTitle = true;
            noToolTip = false;
            playDClick = false;
            recinfoErrCriticalDrops = false;
            dragScroll = 1.5;
            contentColorList = new List<string>();
            contentCustColorList = new List<uint>();
            epgEtcColors = new List<string>();
            epgEtcCustColors = new List<uint>();
            reserveRectColorNormal = "Lime";
            reserveRectColorNo = "Black";
            reserveRectColorNoTuner = "Red";
            reserveRectColorWarning = "Yellow";
            reserveRectColorAutoAddMissing = "Blue";
            titleColor1 = "Black";
            titleColor2 = "Black";
            titleCustColor1 = 0xFFFFFFFF;
            titleCustColor2 = 0xFFFFFFFF;
            reserveRectBackground = false;
            tunerFontNameService = System.Drawing.SystemFonts.DefaultFont.Name;
            tunerFontSizeService = 12;
            tunerFontBoldService = true;
            tunerFontName = System.Drawing.SystemFonts.DefaultFont.Name;
            tunerFontSize = 12;
            tunerServiceColors = new List<string>();
            tunerServiceCustColors = new List<uint>();
            tunerMinHeight = 2;
            tunerMinimumLine = 0;
            tunerDragScroll = 1.5;
            tunerScrollSize = 240;
            tunerMouseScrollAuto = false;
            tunerWidth = 150;
            tunerServiceNoWrap = true;
            tunerTitleIndent = true;
            tunerPopup = false;
            tunerPopupRecinfo = false;
            tunerInfoSingleClick = false;
            tunerColorModeUse = false;
            tunerDisplayOffReserve = false;
            epgTitleIndent = true;
            epgPopup = true;
            epgPopupResOnly = false;
            epgGradation = true;
            epgGradationHeader = true;
            resColumnHead = "";
            resSortDirection = ListSortDirection.Ascending;
            lastWindowState = System.Windows.WindowState.Normal;
            mainWndLeft = -100;
            mainWndTop = -100;
            mainWndWidth = -100;
            mainWndHeight = -100;
            closeMin = false;
            wakeMin = false;
            viewButtonShowAsTab = false;
            viewButtonList = new List<string>();
            taskMenuList = new List<string>();
            cust1BtnName = "";
            cust1BtnCmd = "";
            cust1BtnCmdOpt = "";
            cust2BtnName = "";
            cust2BtnCmd = "";
            cust2BtnCmdOpt = "";
            andKeyList = new List<string>();
            notKeyList = new List<string>();
            defSearchKey = new EpgSearchKeyInfo();
            recPresetList = null;
            recInfoColumnHead = "";
            recInfoSortDirection = ListSortDirection.Ascending;
            recInfoDropErrIgnore = 0;
            recInfoDropWrnIgnore = 0;
            recInfoScrambleIgnore = 0;
            recInfoDropExclude = new List<string>();
            recInfoNoYear = false;
            recInfoNoSecond = false;
            recInfoNoDurSecond = false;
            resInfoNoYear = false;
            resInfoNoSecond = false;
            resInfoNoDurSecond = false;
            tvTestExe = "";
            tvTestCmd = "";
            nwTvMode = false;
            nwTvModeUDP = false;
            nwTvModeTCP = false;
            filePlayExe = "";
            filePlayCmd = "\"$FilePath$\"";
            filePlayOnAirWithExe = false;
            openFolderWithFileDialog = false;
            iEpgStationList = new List<IEPGStationInfo>();
            menuSet = new MenuSettingData();
            nwServerIP = "";
            nwServerPort = 4510;
            nwWaitPort = 4520;
            nwMacAdd = "";
            nwPreset = new List<NWPresetItem>();
            wakeReconnectNW = false;
            suspendCloseNW = false;
            ngAutoEpgLoadNW = false;
            chkSrvRegistTCP = false;
            chkSrvRegistInterval = 5;
            tvTestOpenWait = 2000;
            tvTestChgBonWait = 2000;
            recEndColors = new List<string>();
            recEndCustColors = new List<uint>();
            listDefColor = "カスタム";
            listDefCustColor= 0xFF042271;
            recModeFontColors = new List<string>();
            recModeFontCustColors= new List<uint>();
            resBackColors = new List<string>();
            resBackCustColors= new List<uint>();
            statColors = new List<string>();
            statCustColors= new List<uint>();
            epgInfoSingleClick = false;
            epgInfoOpenMode = 0;
            execBat = 0;
            suspendChk = 0;
            suspendChkTime = 15;
            reserveListColumn = new List<ListColumnInfo>();
            recInfoListColumn = new List<ListColumnInfo>();
            autoAddEpgColumn = new List<ListColumnInfo>();
            autoAddManualColumn = new List<ListColumnInfo>();
            searchWndLeft = -100;
            searchWndTop = -100;
            searchWndWidth = -100;
            searchWndHeight = -100;
            searchWndPinned = false;
            autoSaveNotifyLog = 0;
            showTray = true;
            minHide = true;
            mouseScrollAuto = false;
            noStyle = 0;
            cautionManyChange = true;
            cautionManyNum = 10;
            cautionOnRecChange = true;
            cautionOnRecMarginMin = 5;
            syncResAutoAddChange = false;
            syncResAutoAddChgNewRes = false;
            syncResAutoAddDelete = false;
            keyDeleteDisplayItemNum = 10;
            displayNotifyEpgChange = false;
            displayNotifyJumpTime = 3;
            displayReserveAutoAddMissing = false;
            tryEpgSetting = true;
            laterTimeUse = false;
            laterTimeHour = 28 - 24;
        }

        [NonSerialized()]
        private static Settings _instance;
        [System.Xml.Serialization.XmlIgnore]
        public static Settings Instance
        {
            get
            {
                if (_instance == null)
                    _instance = new Settings();
                return _instance;
            }
            set { _instance = value; }
        }

        //色リストの初期化用
        private static void _FillList<T>(List<T> list, T val, int num)
        {
            if (list.Count < num)
            {
                list.AddRange(Enumerable.Repeat(val, num - list.Count));
            }
        }
        private static void _FillList<T>(List<T> list, List<T> val)
        {
            if (list.Count < val.Count)
            {
                list.AddRange(val.Skip(list.Count));
            }
        }

        /// <summary>
        /// 設定ファイルロード関数
        /// </summary>
        public static void LoadFromXmlFile(bool nwMode = false)
        {
            _LoadFromXmlFile(GetSettingPath(), nwMode);
        }
        private static void _LoadFromXmlFile(string path, bool nwMode)
        {
            try
            {
                using (var fs = new FileStream(path, FileMode.Open, FileAccess.Read))
                {
                    //読み込んで逆シリアル化する
                    var xs = new System.Xml.Serialization.XmlSerializer(typeof(Settings));
                    Instance = (Settings)(xs.Deserialize(fs));
                }
            }
            catch (Exception ex)
            {
                if (ex.GetBaseException().GetType() != typeof(System.IO.FileNotFoundException))
                {
                    string backPath = path + ".back";
                    if (System.IO.File.Exists(backPath) == true)
                    {
                        if (MessageBox.Show("設定ファイルが異常な可能性があります。\r\nバックアップファイルから読み込みますか？", "", MessageBoxButton.YesNo) == MessageBoxResult.Yes)
                        {
                            _LoadFromXmlFile(backPath, nwMode);
                            return;
                        }
                    }
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }
            }

            try
            {
                // タイミング合わせにくいので、メニュー系のデータチェックは
                // MenuManager側のワークデータ作成時に実行する。

                SetCustomEpgTabInfoID();

                int num;
                List<string> defColors = new List<string>();

                //番組表の背景色
                num = 0x11;//番組表17色。過去に16色時代があった。
                if (Instance.contentColorList.Count < num)
                {
                    defColors = new List<string>{
                        "LightYellow"
                        ,"Lavender"
                        ,"LavenderBlush"
                        ,"MistyRose"
                        ,"Honeydew"
                        ,"LightCyan"
                        ,"PapayaWhip"
                        ,"Pink"
                        ,"LightYellow"
                        ,"PapayaWhip"
                        ,"AliceBlue"
                        ,"AliceBlue"
                        ,"White"
                        ,"White"
                        ,"White"
                        ,"WhiteSmoke"
                        ,"White"
                    };
                    _FillList(Instance.contentColorList, defColors);
                }
                num = 0x11 + 5;//番組表17色+予約枠5色
                _FillList(Instance.contentCustColorList, 0xFFFFFFFF, num);

                //チューナ画面各フォント色
                num = 2 + 5;//固定色2+優先度色5
                _FillList(Instance.tunerServiceColors, "Black", num);
                _FillList(Instance.tunerServiceCustColors, 0xFFFFFFFF, num);

                //番組表の時間軸のデフォルトの背景色、その他色
                num = 5;
                if (Instance.epgEtcColors.Count < num)
                {
                    defColors = new List<string>{
                        "MediumPurple"      //00-05時
                        ,"LightSeaGreen"    //06-11時
                        ,"LightSalmon"      //12-17時
                        ,"CornflowerBlue"   //18-23時
                        ,"LightSlateGray"   //サービス色
                    };
                    _FillList(Instance.epgEtcColors, defColors);
                }
                _FillList(Instance.epgEtcCustColors, 0xFFFFFFFF, num);

                //録画済み一覧背景色
                num = 3;
                if (Instance.recEndColors.Count < num)
                {
                    defColors = new List<string>{
                        "White"//デフォルト
                        ,"LightPink"//エラー
                        ,"LightYellow"//ワーニング
                    };
                    _FillList(Instance.recEndColors, defColors);
                }
                _FillList(Instance.recEndCustColors, 0xFFFFFFFF, num);

                //録画モード別予約文字色
                num = 6;
                _FillList(Instance.recModeFontColors, "カスタム", num);
                _FillList(Instance.recModeFontCustColors, 0xFF042271, num);

                //状態別予約背景色
                num = 5;
                if (Instance.resBackColors.Count < num)
                {
                    defColors = new List<string>{
                        "White"//通常
                        ,"LightGray"//無効
                        ,"LightPink"//チューナー不足
                        ,"LightYellow"//一部実行
                        ,"LightBlue"//自動予約登録不明
                    };
                    _FillList(Instance.resBackColors, defColors);
                }
                _FillList(Instance.resBackCustColors, 0xFFFFFFFF, num);

                //予約状態列文字色
                num = 3;
                if (Instance.statColors.Count < num)
                {
                    defColors = new List<string>{
                        "Blue"//予約中
                        ,"OrangeRed"//録画中
                        ,"LimeGreen"//放送中
                    };
                    _FillList(Instance.statColors, defColors);
                }
                _FillList(Instance.statCustColors, 0xFFFFFFFF, num);

                if (Instance.viewButtonList.Count == 0)
                {
                    if (nwMode == false)
                    {
                        Instance.viewButtonList.Add("設定");
                        Instance.viewButtonList.Add("（空白）");
                        Instance.viewButtonList.Add("検索");
                        Instance.viewButtonList.Add("（空白）");
                        Instance.viewButtonList.Add("スタンバイ");
                        Instance.viewButtonList.Add("休止");
                        Instance.viewButtonList.Add("（空白）");
                        Instance.viewButtonList.Add("EPG取得");
                        Instance.viewButtonList.Add("（空白）");
                        Instance.viewButtonList.Add("EPG再読み込み");
                        Instance.viewButtonList.Add("（空白）");
                        Instance.viewButtonList.Add("終了");
                    }
                    else
                    {
                        Instance.viewButtonList.Add("設定");
                        Instance.viewButtonList.Add("（空白）");
                        Instance.viewButtonList.Add("再接続");
                        Instance.viewButtonList.Add("（空白）");
                        Instance.viewButtonList.Add("検索");
                        Instance.viewButtonList.Add("（空白）");
                        Instance.viewButtonList.Add("EPG取得");
                        Instance.viewButtonList.Add("（空白）");
                        Instance.viewButtonList.Add("EPG再読み込み");
                        Instance.viewButtonList.Add("（空白）");
                        Instance.viewButtonList.Add("終了");
                    }
                }
                if (Instance.taskMenuList.Count == 0)
                {
                    if (nwMode == false)
                    {
                        Instance.taskMenuList.Add("設定");
                        Instance.taskMenuList.Add("（セパレータ）");
                        Instance.taskMenuList.Add("スタンバイ");
                        Instance.taskMenuList.Add("休止");
                        Instance.taskMenuList.Add("（セパレータ）");
                        Instance.taskMenuList.Add("終了");
                    }
                    else
                    {
                        Instance.taskMenuList.Add("設定");
                        Instance.taskMenuList.Add("（セパレータ）");
                        Instance.taskMenuList.Add("再接続");
                        Instance.taskMenuList.Add("（セパレータ）");
                        Instance.taskMenuList.Add("終了");
                    }
                }
                if (Instance.reserveListColumn.Count == 0)
                {
                    var obj = new ReserveItem();
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.StartTime), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.NetworkName), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.ServiceName), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.EventName), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.RecMode), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.Priority), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.Tuijyu), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.Comment), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.RecFileName), double.NaN));
                }
                if (Instance.recInfoListColumn.Count == 0)
                {
                    var obj = new RecInfoItem();
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.IsProtect), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.StartTime), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.NetworkName), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.ServiceName), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.EventName), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.Drops), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.Scrambles), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.Result), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.RecFilePath), double.NaN));
                }
                if (Instance.autoAddEpgColumn.Count == 0)
                {
                    var obj = new EpgAutoDataItem();
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.EventName), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.NotKey), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.RegExp), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.RecMode), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.Priority), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.Tuijyu), double.NaN));
                }
                if (Instance.autoAddManualColumn.Count == 0)
                {
                    var obj = new ManualAutoAddDataItem();
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.DayOfWeek), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.StartTime), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.EventName), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.ServiceName), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.RecMode), double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo(CommonUtil.GetMemberName(() => obj.Priority), double.NaN));
                }
                if (Instance.recInfoDropExclude.Count == 0)
                {
                    Settings.Instance.RecInfoDropExclude = new List<string> { "EIT", "NIT", "CAT", "SDT", "SDTT", "TOT", "ECM", "EMM" };
                }
                if (Instance.RecPresetList.Count == 0)
                {
                    Instance.RecPresetList.Add(new RecPresetItem("デフォルト", 0));
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
 
        public static void SaveToXmlFile()
        {
            try
            {
                string path = GetSettingPath();

                if (System.IO.File.Exists(path) == true)
                {
                    string backPath = path + ".back";
                    System.IO.File.Copy(path, backPath, true);
                }

                FileStream fs = new FileStream(path,
                    FileMode.Create,
                    FileAccess.Write, FileShare.None);
                System.Xml.Serialization.XmlSerializer xs =
                    new System.Xml.Serialization.XmlSerializer(
                    typeof(Settings));
                //シリアル化して書き込む
                xs.Serialize(fs, Instance);
                fs.Close();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private static string GetSettingPath()
        {
            Assembly myAssembly = Assembly.GetEntryAssembly();
            string path = myAssembly.Location + ".xml";

            return path;
        }

        public void SetSettings(string propertyName, object value)
        {
            if (propertyName == null) return;
            var info = typeof(Settings).GetProperty(propertyName);
            if (info != null) info.SetValue(this, value, null);
        }

        public object GetSettings(string propertyName)
        {
            if (propertyName == null) return null;
            var info = typeof(Settings).GetProperty(propertyName);
            return (info == null ? null : info.GetValue(this, null));
        }

        public static void GetDefRecSetting(UInt32 presetID, ref RecSettingData defKey)
        {
            RecPresetItem preset = Instance.RecPresetList.FirstOrDefault(item => item.ID == presetID);
            if (preset == null) preset = new RecPresetItem("", presetID);
            preset.RecPresetData.CopyTo(defKey);
        }

        public void ReloadOtherOptions()
        {
            DefStartMargin = IniFileHandler.GetPrivateProfileInt("SET", "StartMargin", 0, SettingPath.TimerSrvIniPath);
            DefEndMargin = IniFileHandler.GetPrivateProfileInt("SET", "EndMargin", 0, SettingPath.TimerSrvIniPath);
            defRecfolders = null;
        }

        //デフォルトマージン
        [System.Xml.Serialization.XmlIgnore]
        public int DefStartMargin { get;private set; }
        [System.Xml.Serialization.XmlIgnore]
        public int DefEndMargin { get;private set; }

        List<string> defRecfolders = null;
        [System.Xml.Serialization.XmlIgnore]
        public List<string> DefRecFolders
        {
            get
            {
                if (defRecfolders == null)
                {
                    defRecfolders = new List<string>();
                    int num = IniFileHandler.GetPrivateProfileInt("SET", "RecFolderNum", 0, SettingPath.CommonIniPath);
                    if (num == 0)
                    {
                        defRecfolders.Add(IniFileHandler.GetPrivateProfileString("SET", "DataSavePath", "Setting", SettingPath.CommonIniPath));
                    }
                    else
                    {
                        for (uint i = 0; i < num; i++)
                        {
                            string key = "RecFolderPath" + i.ToString();
                            string folder = IniFileHandler.GetPrivateProfileString("SET", key, "", SettingPath.CommonIniPath);
                            if (folder.Length > 0)
                            {
                                defRecfolders.Add(folder);
                            }
                        }
                    }
                }
                return defRecfolders;
            }
        }

        public static void SetCustomEpgTabInfoID()
        {
            for (int i = 0; i < Settings.Instance.CustomEpgTabList.Count; i++)
            {
                Settings.Instance.CustomEpgTabList[i].ID = i;
            }
        }
    }
}
