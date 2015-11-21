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
        [DllImport("KERNEL32.DLL")]
        public static extern uint
          GetPrivateProfileString(string lpAppName,
          string lpKeyName, string lpDefault,
          StringBuilder lpReturnedString, uint nSize,
          string lpFileName);

        [DllImport("KERNEL32.DLL",
            EntryPoint = "GetPrivateProfileStringA")]
        public static extern uint
          GetPrivateProfileStringByByteArray(string lpAppName,
          string lpKeyName, string lpDefault,
          byte[] lpReturnedString, uint nSize,
          string lpFileName);

        [DllImport("KERNEL32.DLL")]
        public static extern int
          GetPrivateProfileInt(string lpAppName,
          string lpKeyName, int nDefault, string lpFileName);

        [DllImport("KERNEL32.DLL")]
        public static extern uint WritePrivateProfileString(
          string lpAppName,
          string lpKeyName,
          string lpString,
          string lpFileName);

        public static string
          GetPrivateProfileString(string lpAppName,
          string lpKeyName, string lpDefault, string lpFileName)
        {
            StringBuilder buff = new StringBuilder(512);
            IniFileHandler.GetPrivateProfileString(lpAppName, lpKeyName, lpDefault, buff, 512, lpFileName);
            return buff.ToString();
        }
        
        public static void UpdateSrvProfileIniNW()
        {
            SendIniCopy("EpgTimerSrv.ini");
            SendIniCopy("Common.ini");
            SendIniCopy("EpgDataCap_Bon.ini");
            SendIniCopy("ChSet5.txt");

            Settings.UpdateDefRecSetting();
            ChSet5.LoadFile();
        }

        private static bool SendIniCopy(string iniFileName)
        {
            try
            {
                byte[] binData;
                if (CommonManager.Instance.CtrlCmd.SendFileCopy(iniFileName, out binData) == ErrCode.CMD_SUCCESS)
                {
                    string filePath = SettingPath.SettingFolderPath;
                    System.IO.Directory.CreateDirectory(filePath);
                    filePath = filePath.TrimEnd('\\') + "\\" + iniFileName;
                    using (System.IO.BinaryWriter w = new System.IO.BinaryWriter(System.IO.File.Create(filePath)))
                    {
                        w.Write(binData);
                        w.Close();
                    }
                    return true;
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
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
                string defRecExe = SettingPath.ModulePath.TrimEnd('\\') + "\\EpgDataCap_Bon.exe";
                return IniFileHandler.GetPrivateProfileString("SET", "RecExePath", defRecExe, SettingPath.CommonIniPath);
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
                    string path = IniFileHandler.GetPrivateProfileString("SET", "DataSavePath", SettingPath.DefSettingFolderPath, SettingPath.CommonIniPath);
                    return (Path.IsPathRooted(path) ? "" : SettingPath.ModulePath.TrimEnd('\\') + "\\") + path;
                }
                else
                {
                    return SettingPath.DefSettingFolderPath;
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
        private double dragScroll;
        private List<string> contentColorList;
        private List<UInt32> contentCustColorList;
        private List<string> timeColorList;
        private List<UInt32> timeCustColorList;
        private string reserveRectColorNormal;
        private string reserveRectColorNo;
        private string reserveRectColorNoTuner;
        private string reserveRectColorWarning;
        private string reserveRectColorAutoAddMissing;
        private string titleColor1;
        private string titleColor2;
        private UInt32 titleCustColor1;
        private UInt32 titleCustColor2;
        private string serviceColor;
        private UInt32 serviceCustColor;
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
        private string tvTestExe;
        private string tvTestCmd;
        private bool nwTvMode;
        private bool nwTvModeUDP;
        private bool nwTvModeTCP;
        private string filePlayExe;
        private string filePlayCmd;
        private bool openFolderWithFileDialog;
        private List<IEPGStationInfo> iEpgStationList;
        private MenuSettingData menuSet;
        private string nwServerIP;
        private UInt32 nwServerPort;
        private UInt32 nwWaitPort;
        private string nwMacAdd;
        private bool wakeReconnectNW;
        private bool suspendCloseNW;
        private bool ngAutoEpgLoadNW;
        private bool chkSrvRegistTCP;
        private double chkSrvRegistInterval;
        private Int32 tvTestOpenWait;
        private Int32 tvTestChgBonWait;
        private string listDefFontColor;            //各画面のリストのデフォルト文字色
        private List<string> recModeFontColorList;  //予約リストなどの録画モードごとの文字色
        private string resDefBackColor;             //予約リストなどのデフォルト背景色
        private string resErrBackColor;             //予約リストなどのチューナ不足の背景色
        private string resWarBackColor;             //予約リストなどのチューナ不足で一部実行の背景色
        private string resNoBackColor;              //予約リストなどの無効予約の背景色
        private string resAutoAddMissingBackColor;  //予約リストなどの自動登録が見つからない予約の背景色
        private string recEndDefBackColor;          //録画済リストのデフォルト背景色
        private string recEndErrBackColor;          //録画済リストのエラー表示の背景色
        private string recEndWarBackColor;          //録画済リストの警告表示の背景色
        private string statResForeColor;            //予約リストなどの「状態」列の予約色
        private string statRecForeColor;            //予約リストなどの「状態」の録画色
        private string statOnAirForeColor;          //予約リストなどの「状態」の放送色
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
        private short autoSaveNotifyLog;
        private bool showTray;
        private bool minHide;
        private bool mouseScrollAuto;
        private int noStyle;
        private bool cautionManyChange;
        private int cautionManyNum;
        private bool cautionOnRecChange;
        private int cautionOnRecMarginMin;
        private int keyDeleteDisplayItemNum;
        private bool displayNotifyEpgChange;
        private int displayNotifyJumpTime;
        private bool displayReserveAutoAddMissing;
        private bool tryEpgSetting;

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
        public List<string> TimeColorList
        {
            get { return timeColorList; }
            set { timeColorList = value; }
        }
        public List<UInt32> TimeCustColorList
        {
            get { return timeCustColorList; }
            set { timeCustColorList = value; }
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
        public string ServiceColor
        {
            get { return serviceColor; }
            set { serviceColor = value; }
        }
        public UInt32 ServiceCustColor
        {
            get { return serviceCustColor; }
            set { serviceCustColor = value; }
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
        public List<RecPresetItem> RecPresetList
        {
            get { return recPresetList; }
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
        public string ListDefFontColor
        {
            get { return listDefFontColor; }
            set { listDefFontColor = value; }
        }
        public List<string> RecModeFontColorList
        {
            get { return recModeFontColorList; }
            set { recModeFontColorList = value; }
        }
        public string ResDefBackColor
        {
            get { return resDefBackColor; }
            set { resDefBackColor = value; }
        }
        public string ResErrBackColor
        {
            get { return resErrBackColor; }
            set { resErrBackColor = value; }
        }
        public string ResWarBackColor
        {
            get { return resWarBackColor; }
            set { resWarBackColor = value; }
        }
        public string ResNoBackColor
        {
            get { return resNoBackColor; }
            set { resNoBackColor = value; }
        }
        public string ResAutoAddMissingBackColor
        {
            get { return resAutoAddMissingBackColor; }
            set { resAutoAddMissingBackColor = value; }
        }
        public string RecEndDefBackColor
        {
            get { return recEndDefBackColor; }
            set { recEndDefBackColor = value; }
        }
        public string RecEndErrBackColor
        {
            get { return recEndErrBackColor; }
            set { recEndErrBackColor = value; }
        }
        public string RecEndWarBackColor
        {
            get { return recEndWarBackColor; }
            set { recEndWarBackColor = value; }
        }
        public string StatResForeColor
        {
            get { return statResForeColor; }
            set { statResForeColor = value; }
        }
        public string StatRecForeColor
        {
            get { return statRecForeColor; }
            set { statRecForeColor = value; }
        }
        public string StatOnAirForeColor
        {
            get { return statOnAirForeColor; }
            set { statOnAirForeColor = value; }
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
        public int DisplayNotifyJumpTime
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
            dragScroll = 1.5;
            contentColorList = new List<string>();
            contentCustColorList = new List<uint>();
            timeColorList = new List<string>();
            timeCustColorList = new List<uint>();
            reserveRectColorNormal = "Lime";
            reserveRectColorNo = "Black";
            reserveRectColorNoTuner = "Red";
            reserveRectColorWarning = "Yellow";
            reserveRectColorAutoAddMissing = "Blue";
            titleColor1 = "Black";
            titleColor2 = "Black";
            titleCustColor1 = 0xFFFFFFFF;
            titleCustColor2 = 0xFFFFFFFF;
            serviceColor = "LightSlateGray";
            serviceCustColor = 0xFFFFFFFF;
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
            recPresetList = new List<RecPresetItem>();
            recInfoColumnHead = "";
            recInfoSortDirection = ListSortDirection.Ascending;
            recInfoDropErrIgnore = 0;
            recInfoDropWrnIgnore = 0;
            recInfoScrambleIgnore = 0;
            tvTestExe = "";
            tvTestCmd = "";
            nwTvMode = false;
            nwTvModeUDP = false;
            nwTvModeTCP = false;
            filePlayExe = "";
            filePlayCmd = "\"$FilePath$\"";
            openFolderWithFileDialog = false;
            iEpgStationList = new List<IEPGStationInfo>();
            menuSet = new MenuSettingData();
            nwServerIP = "";
            nwServerPort = 4510;
            nwWaitPort = 4520;
            nwMacAdd = "";
            wakeReconnectNW = false;
            suspendCloseNW = false;
            ngAutoEpgLoadNW = false;
            chkSrvRegistTCP = false;
            chkSrvRegistInterval = 5;
            tvTestOpenWait = 2000;
            tvTestChgBonWait = 2000;
            listDefFontColor = "#FF042271";
            recModeFontColorList = new List<String>();
            resDefBackColor = "White";
            resErrBackColor = "#FFFFAAAA";
            resWarBackColor = "#FFFFFFAA";
            resNoBackColor = "#FFAAAAAA";
            resAutoAddMissingBackColor = "Powderblue";
            recEndDefBackColor = "White";
            recEndErrBackColor = "#FFFFAAAA";
            recEndWarBackColor = "#FFFFFFAA";
            statResForeColor = "RoyalBlue";
            statRecForeColor = "OrangeRed";
            statOnAirForeColor = "LimeGreen";
            epgInfoSingleClick = false;
            epgInfoOpenMode = 0;
            execBat = 0;
            suspendChk = 0;
            suspendChkTime = 15;
            reserveListColumn = new List<ListColumnInfo>();
            recInfoListColumn = new List<ListColumnInfo>();
            autoAddEpgColumn = new List<ListColumnInfo>();
            autoAddManualColumn = new List<ListColumnInfo>();
            searchWndLeft = 0;
            searchWndTop = 0;
            searchWndWidth = 0;
            searchWndHeight = 0;
            autoSaveNotifyLog = 0;
            showTray = true;
            minHide = true;
            mouseScrollAuto = false;
            noStyle = 0;
            cautionManyChange = true;
            cautionManyNum = 10;
            cautionOnRecChange = true;
            cautionOnRecMarginMin = 5;
            keyDeleteDisplayItemNum = 10;
            displayNotifyEpgChange = false;
            displayNotifyJumpTime = 3;
            displayReserveAutoAddMissing = false;
            tryEpgSetting = true;
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

                if (Instance.recModeFontColorList.Count != 6)
                {
                    //予約アイテムのデフォルトの文字色
                    Instance.recModeFontColorList = Enumerable.Repeat("#FF042271", 6).ToList();
                }
                int num;
                num = 0x11;//番組表17色。forは色の増加対策の読込。過去に16色時代があった。
                if (Instance.contentColorList.Count < 0x11)
                {
                    //番組表のデフォルトの背景色
                    var defColors = new List<string>{
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
                    for (int i = Instance.contentColorList.Count; i < 0x11; i++)
                    {
                        Instance.contentColorList.Add(defColors[i]);
                    }
                }
                num = 0x11 + 5;//番組表17色+予約枠5色
                if (Instance.contentCustColorList.Count < num)
                {
                    Instance.contentCustColorList.AddRange(
                        Enumerable.Repeat(0xFFFFFFFF, num - Instance.contentCustColorList.Count));
                }
                num = 2 + 5;//固定色2+優先度色5
                if (Instance.tunerServiceColors.Count < num)
                {
                    Instance.tunerServiceColors.AddRange(
                        Enumerable.Repeat("Black", num - Instance.tunerServiceColors.Count));
                }
                if (Instance.tunerServiceCustColors.Count < num)
                {
                    Instance.tunerServiceCustColors.AddRange(
                        Enumerable.Repeat(0xFFFFFFFF, num - Instance.tunerServiceCustColors.Count));
                }
                if (Instance.timeColorList.Count != 4)
                {
                    //番組表の時間軸のデフォルトの背景色
                    Instance.timeColorList.Clear();
                    Instance.timeColorList.Add("MediumPurple");
                    Instance.timeColorList.Add("LightSeaGreen");
                    Instance.timeColorList.Add("LightSalmon");
                    Instance.timeColorList.Add("CornflowerBlue");
                }
                if (Instance.timeCustColorList.Count != 4)
                {
                    Instance.timeCustColorList = Enumerable.Repeat(0xFFFFFFFF, 4).ToList();
                }
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
                    Instance.reserveListColumn.Add(new ListColumnInfo("StartTime", double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo("NetworkName", double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo("ServiceName", double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo("EventName", double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo("RecMode", double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo("Priority", double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo("Tuijyu", double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo("Comment", double.NaN));
                    Instance.reserveListColumn.Add(new ListColumnInfo("RecFileName", double.NaN));
                }
                if (Instance.recInfoListColumn.Count == 0)
                {
                    Instance.recInfoListColumn.Add(new ListColumnInfo("IsProtect", double.NaN));
                    Instance.recInfoListColumn.Add(new ListColumnInfo("StartTime", double.NaN));
                    Instance.recInfoListColumn.Add(new ListColumnInfo("NetworkName", double.NaN));
                    Instance.recInfoListColumn.Add(new ListColumnInfo("ServiceName", double.NaN));
                    Instance.recInfoListColumn.Add(new ListColumnInfo("EventName", double.NaN));
                    Instance.recInfoListColumn.Add(new ListColumnInfo("Drops", double.NaN));
                    Instance.recInfoListColumn.Add(new ListColumnInfo("Scrambles", double.NaN));
                    Instance.recInfoListColumn.Add(new ListColumnInfo("Result", double.NaN));
                    Instance.recInfoListColumn.Add(new ListColumnInfo("RecFilePath", double.NaN));
                }
                if (Instance.autoAddEpgColumn.Count == 0)
                {
                    Instance.autoAddEpgColumn.Add(new ListColumnInfo("AndKey", double.NaN));
                    Instance.autoAddEpgColumn.Add(new ListColumnInfo("NotKey", double.NaN));
                    Instance.autoAddEpgColumn.Add(new ListColumnInfo("RegExp", double.NaN));
                    Instance.autoAddEpgColumn.Add(new ListColumnInfo("RecMode", double.NaN));
                    Instance.autoAddEpgColumn.Add(new ListColumnInfo("Priority", double.NaN));
                    Instance.autoAddEpgColumn.Add(new ListColumnInfo("Tuijyu", double.NaN));
                }
                if (Instance.autoAddManualColumn.Count == 0)
                {
                    Instance.autoAddManualColumn.Add(new ListColumnInfo("DayOfWeek", double.NaN));
                    Instance.autoAddManualColumn.Add(new ListColumnInfo("Time", double.NaN));
                    Instance.autoAddManualColumn.Add(new ListColumnInfo("Title", double.NaN));
                    Instance.autoAddManualColumn.Add(new ListColumnInfo("StationName", double.NaN));
                    Instance.autoAddManualColumn.Add(new ListColumnInfo("RecMode", double.NaN));
                    Instance.autoAddManualColumn.Add(new ListColumnInfo("Priority", double.NaN));
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

            defKey.RecFolderList.Clear();
            int count = IniFileHandler.GetPrivateProfileInt(defFolderName, "Count", 0, SettingPath.TimerSrvIniPath);
            for (int i = 0; i < count; i++)
            {
                RecFileSetInfo folderInfo = new RecFileSetInfo();
                folderInfo.RecFolder = IniFileHandler.GetPrivateProfileString(defFolderName, i.ToString(), "", SettingPath.TimerSrvIniPath);
                folderInfo.WritePlugIn = IniFileHandler.GetPrivateProfileString(defFolderName, "WritePlugIn" + i.ToString(), "Write_Default.dll", SettingPath.TimerSrvIniPath);
                folderInfo.RecNamePlugIn = IniFileHandler.GetPrivateProfileString(defFolderName, "RecNamePlugIn" + i.ToString(), "", SettingPath.TimerSrvIniPath);

                defKey.RecFolderList.Add(folderInfo);
            }

            defKey.PartialRecFolder.Clear();
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

        //プリセットの更新
        public static void UpdateDefRecSetting()
        {
            Settings.Instance.RecPresetList.Clear();
            string pIDs = "0," + IniFileHandler.GetPrivateProfileString("SET", "PresetID", "", SettingPath.TimerSrvIniPath);
            foreach (string pID in pIDs.Split(','))
            {
                uint id;
                if (uint.TryParse(pID, out id) == false) continue;
                string name = IniFileHandler.GetPrivateProfileString("REC_DEF" + (id == 0 ? "" : id.ToString()), "SetName", "", SettingPath.TimerSrvIniPath);
                Settings.Instance.RecPresetList.Add(new RecPresetItem(name, id));
            }
        }

        public static List<string> GetDefRecFolders()
        {
            var folders = new List<string>();
            int num = IniFileHandler.GetPrivateProfileInt("SET", "RecFolderNum", 0, SettingPath.CommonIniPath);
            if (num == 0)
            {
                folders.Add(IniFileHandler.GetPrivateProfileString("SET", "DataSavePath", "Setting", SettingPath.CommonIniPath));
            }
            else
            {
                for (uint i = 0; i < num; i++)
                {
                    string key = "RecFolderPath" + i.ToString();
                    string folder = IniFileHandler.GetPrivateProfileString("SET", key, "", SettingPath.CommonIniPath);
                    if (folder.Length > 0)
                    {
                        folders.Add(folder);
                    }
                }
            }
            return folders;
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
