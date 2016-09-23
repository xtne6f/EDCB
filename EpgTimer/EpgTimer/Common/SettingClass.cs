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
    }

    class SettingPath
    {
        public static string CommonIniPath
        {
            get
            {
                string iniPath = ModulePath.TrimEnd('\\');
                iniPath += "\\Common.ini";
                return iniPath;
            }
        }
        public static string TimerSrvIniPath
        {
            get
            {
                string iniPath = ModulePath.TrimEnd('\\');
                iniPath += "\\EpgTimerSrv.ini";
                return iniPath;
            }
        }
        public static string DefSettingFolderPath
        {
            get
            {
//                string defSetPath = System.Environment.GetFolderPath(Environment.SpecialFolder.Personal);
//                defSetPath += "\\EpgTimerBon";
                string defSetPath = ModulePath.TrimEnd('\\');
                defSetPath += "\\Setting";

                return defSetPath;
            }
        }
        public static string SettingFolderPath
        {
            get
            {
                string path = IniFileHandler.GetPrivateProfileString("SET", "DataSavePath", DefSettingFolderPath, CommonIniPath);
                return (Path.IsPathRooted(path) ? "" : ModulePath.TrimEnd('\\') + "\\") + path;
            }
        }
        public static void CheckFolderPath(ref string folderPath)
        {
            if( folderPath.LastIndexOf("\\") == folderPath.Length-1 ){
                folderPath = folderPath.Remove(folderPath.Length - 1);
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
        private string titleColor1;
        private string titleColor2;
        private UInt32 titleCustColor1;
        private UInt32 titleCustColor2;
        private string serviceColor;
        private UInt32 serviceCustColor;
        private bool reserveRectBackground;
        private bool epgTitleIndent;
        private bool epgPopup;
        private bool epgGradation;
        private bool epgGradationHeader;
        private double resColumnWidth0;
        private double resColumnWidth1;
        private double resColumnWidth2;
        private double resColumnWidth3;
        private double resColumnWidth4;
        private double resColumnWidth5;
        private double resColumnWidth6;
        private double resColumnWidth7;
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
        private bool searchKeyRegExp;
        private bool searchKeyTitleOnly;
        private bool searchKeyAimaiFlag;
        private bool searchKeyNotContent;
        private bool searchKeyNotDate;
        private List<ContentKindInfo> searchKeyContentList;
        private List<DateItem> searchKeyDateItemList;
        private List<Int64> searchKeyServiceList;
        private byte searchKeyFreeCA;
        private byte searchKeyChkRecEnd;
        private UInt16 searchKeyChkRecDay;
        private double recInfoColumnWidth0;
        private double recInfoColumnWidth1;
        private double recInfoColumnWidth2;
        private double recInfoColumnWidth3;
        private double recInfoColumnWidth4;
        private double recInfoColumnWidth5;
        private double recInfoColumnWidth6;
        private string recInfoColumnHead;
        private ListSortDirection recInfoSortDirection;
        private string tvTestExe;
        private string tvTestCmd;
        private bool nwTvMode;
        private bool nwTvModeUDP;
        private bool nwTvModeTCP;
        private string filePlayExe;
        private string filePlayCmd;
        private bool filePlayOnAirWithExe;
        private List<IEPGStationInfo> iEpgStationList;
        private string nwServerIP;
        private UInt32 nwServerPort;
        private UInt32 nwWaitPort;
        private string nwMacAdd;
        private bool wakeReconnectNW;
        private bool suspendCloseNW;
        private bool ngAutoEpgLoadNW;
        private Int32 tvTestOpenWait;
        private Int32 tvTestChgBonWait;
        private byte resDefColorR;
        private byte resDefColorG;
        private byte resDefColorB;
        private byte resErrColorR;
        private byte resErrColorG;
        private byte resErrColorB;
        private byte resWarColorR;
        private byte resWarColorG;
        private byte resWarColorB;
        private byte resNoColorR;
        private byte resNoColorG;
        private byte resNoColorB;
        private byte recEndDefColorR;
        private byte recEndDefColorG;
        private byte recEndDefColorB;
        private byte recEndErrColorR;
        private byte recEndErrColorG;
        private byte recEndErrColorB;
        private byte recEndWarColorR;
        private byte recEndWarColorG;
        private byte recEndWarColorB;
        private byte epgTipsBackColorR;
        private byte epgTipsBackColorG;
        private byte epgTipsBackColorB;
        private byte epgTipsForeColorR;
        private byte epgTipsForeColorG;
        private byte epgTipsForeColorB;
        private bool epgInfoSingleClick;
        private byte epgInfoOpenMode;
        private UInt32 execBat;
        private UInt32 suspendChk;
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
        public double ResColumnWidth0
        {
            get { return resColumnWidth0; }
            set { resColumnWidth0 = value; }
        }
        public double ResColumnWidth1
        {
            get { return resColumnWidth1; }
            set { resColumnWidth1 = value; }
        }
        public double ResColumnWidth2
        {
            get { return resColumnWidth2; }
            set { resColumnWidth2 = value; }
        }
        public double ResColumnWidth3
        {
            get { return resColumnWidth3; }
            set { resColumnWidth3 = value; }
        }
        public double ResColumnWidth4
        {
            get { return resColumnWidth4; }
            set { resColumnWidth4 = value; }
        }
        public double ResColumnWidth5
        {
            get { return resColumnWidth5; }
            set { resColumnWidth5 = value; }
        }
        public double ResColumnWidth6
        {
            get { return resColumnWidth6; }
            set { resColumnWidth6 = value; }
        }
        public double ResColumnWidth7
        {
            get { return resColumnWidth7; }
            set { resColumnWidth7 = value; }
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
        public bool SearchKeyRegExp
        {
            get { return searchKeyRegExp; }
            set { searchKeyRegExp = value; }
        }
        public bool SearchKeyTitleOnly
        {
            get { return searchKeyTitleOnly; }
            set { searchKeyTitleOnly = value; }
        }
        public bool SearchKeyAimaiFlag
        {
            get { return searchKeyAimaiFlag; }
            set { searchKeyAimaiFlag = value; }
        }
        public bool SearchKeyNotContent
        {
            get { return searchKeyNotContent; }
            set { searchKeyNotContent = value; }
        }
        public bool SearchKeyNotDate
        {
            get { return searchKeyNotDate; }
            set { searchKeyNotDate = value; }
        }
        public List<ContentKindInfo> SearchKeyContentList
        {
            get { return searchKeyContentList; }
            set { searchKeyContentList = value; }
        }
        public List<DateItem> SearchKeyDateItemList
        {
            get { return searchKeyDateItemList; }
            set { searchKeyDateItemList = value; }
        }
        public List<Int64> SearchKeyServiceList
        {
            get { return searchKeyServiceList; }
            set { searchKeyServiceList = value; }
        }
        public byte SearchKeyFreeCA
        {
            get { return searchKeyFreeCA; }
            set { searchKeyFreeCA = value; }
        }
        public byte SearchKeyChkRecEnd
        {
            get { return searchKeyChkRecEnd; }
            set { searchKeyChkRecEnd = value; }
        }
        public UInt16 SearchKeyChkRecDay
        {
            get { return searchKeyChkRecDay; }
            set { searchKeyChkRecDay = value; }
        }
        [System.Xml.Serialization.XmlIgnore]
        public List<RecPresetItem> RecPresetList
        {
            get
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
        }
        public double RecInfoColumnWidth0
        {
            get { return recInfoColumnWidth0; }
            set { recInfoColumnWidth0 = value; }
        }
        public double RecInfoColumnWidth1
        {
            get { return recInfoColumnWidth1; }
            set { recInfoColumnWidth1 = value; }
        }
        public double RecInfoColumnWidth2
        {
            get { return recInfoColumnWidth2; }
            set { recInfoColumnWidth2 = value; }
        }
        public double RecInfoColumnWidth3
        {
            get { return recInfoColumnWidth3; }
            set { recInfoColumnWidth3 = value; }
        }
        public double RecInfoColumnWidth4
        {
            get { return recInfoColumnWidth4; }
            set { recInfoColumnWidth4 = value; }
        }
        public double RecInfoColumnWidth5
        {
            get { return recInfoColumnWidth5; }
            set { recInfoColumnWidth5 = value; }
        }
        public double RecInfoColumnWidth6
        {
            get { return recInfoColumnWidth6; }
            set { recInfoColumnWidth6 = value; }
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
        public List<IEPGStationInfo> IEpgStationList
        {
            get { return iEpgStationList; }
            set { iEpgStationList = value; }
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
        public byte ResDefColorR
        {
            get { return resDefColorR; }
            set { resDefColorR = value; }
        }
        public byte ResDefColorG
        {
            get { return resDefColorG; }
            set { resDefColorG = value; }
        }
        public byte ResDefColorB
        {
            get { return resDefColorB; }
            set { resDefColorB = value; }
        }
        public byte ResErrColorR
        {
            get { return resErrColorR; }
            set { resErrColorR = value; }
        }
        public byte ResErrColorG
        {
            get { return resErrColorG; }
            set { resErrColorG = value; }
        }
        public byte ResErrColorB
        {
            get { return resErrColorB; }
            set { resErrColorB = value; }
        }
        public byte ResWarColorR
        {
            get { return resWarColorR; }
            set { resWarColorR = value; }
        }
        public byte ResWarColorG
        {
            get { return resWarColorG; }
            set { resWarColorG = value; }
        }
        public byte ResWarColorB
        {
            get { return resWarColorB; }
            set { resWarColorB = value; }
        }
        public byte ResNoColorR
        {
            get { return resNoColorR; }
            set { resNoColorR = value; }
        }
        public byte ResNoColorG
        {
            get { return resNoColorG; }
            set { resNoColorG = value; }
        }
        public byte ResNoColorB
        {
            get { return resNoColorB; }
            set { resNoColorB = value; }
        }
        public byte RecEndDefColorR
        {
            get { return recEndDefColorR; }
            set { recEndDefColorR = value; }
        }
        public byte RecEndDefColorG
        {
            get { return recEndDefColorG; }
            set { recEndDefColorG = value; }
        }
        public byte RecEndDefColorB
        {
            get { return recEndDefColorB; }
            set { recEndDefColorB = value; }
        }
        public byte RecEndErrColorR
        {
            get { return recEndErrColorR; }
            set { recEndErrColorR = value; }
        }
        public byte RecEndErrColorG
        {
            get { return recEndErrColorG; }
            set { recEndErrColorG = value; }
        }
        public byte RecEndErrColorB
        {
            get { return recEndErrColorB; }
            set { recEndErrColorB = value; }
        }
        public byte RecEndWarColorR
        {
            get { return recEndWarColorR; }
            set { recEndWarColorR = value; }
        }
        public byte RecEndWarColorG
        {
            get { return recEndWarColorG; }
            set { recEndWarColorG = value; }
        }
        public byte RecEndWarColorB
        {
            get { return recEndWarColorB; }
            set { recEndWarColorB = value; }
        }
        public byte EpgTipsBackColorR
        {
            get { return epgTipsBackColorR; }
            set { epgTipsBackColorR = value; }
        }
        public byte EpgTipsBackColorG
        {
            get { return epgTipsBackColorG; }
            set { epgTipsBackColorG = value; }
        }
        public byte EpgTipsBackColorB
        {
            get { return epgTipsBackColorB; }
            set { epgTipsBackColorB = value; }
        }
        public byte EpgTipsForeColorR
        {
            get { return epgTipsForeColorR; }
            set { epgTipsForeColorR = value; }
        }
        public byte EpgTipsForeColorG
        {
            get { return epgTipsForeColorG; }
            set { epgTipsForeColorG = value; }
        }
        public byte EpgTipsForeColorB
        {
            get { return epgTipsForeColorB; }
            set { epgTipsForeColorB = value; }
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
        
        
        public Settings()
        {
            useCustomEpgView = false;
            customEpgTabList = new List<CustomEpgTabInfo>();
            minHeight = 2;
            minimumHeight = 0;
            serviceWidth = 150;
            scrollSize = 240;
            fontName = "メイリオ";
            fontSize = 12;
            fontNameTitle = "メイリオ";
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
            titleColor1 = "Black";
            titleColor2 = "Black";
            titleCustColor1 = 0xFFFFFFFF;
            titleCustColor2 = 0xFFFFFFFF;
            serviceColor = "LightSlateGray";
            serviceCustColor = 0xFFFFFFFF;
            reserveRectBackground = false;
            epgTitleIndent = true;
            epgPopup = true;
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
            searchKeyRegExp = false;
            searchKeyTitleOnly = false;
            searchKeyAimaiFlag = false;
            searchKeyNotContent = false;
            searchKeyNotDate = false;
            searchKeyFreeCA = 0;
            searchKeyChkRecEnd = 0;
            searchKeyChkRecDay = 6;
            searchKeyContentList = new List<ContentKindInfo>();
            searchKeyDateItemList = new List<DateItem>();
            searchKeyServiceList = new List<Int64>();
            recInfoColumnHead = "";
            recInfoSortDirection = ListSortDirection.Ascending;
            tvTestExe = "";
            tvTestCmd = "";
            nwTvMode = false;
            nwTvModeUDP = false;
            nwTvModeTCP = false;
            filePlayExe = "";
            filePlayCmd = "\"$FilePath$\"";
            filePlayOnAirWithExe = false;
            iEpgStationList = new List<IEPGStationInfo>();
            nwServerIP = "";
            nwServerPort = 4510;
            nwWaitPort = 0;
            nwMacAdd = "";
            wakeReconnectNW = false;
            suspendCloseNW = false;
            ngAutoEpgLoadNW = false;
            tvTestOpenWait = 2000;
            tvTestChgBonWait = 2000;
            resDefColorR = 0xFF;
            resDefColorG = 0xFF;
            resDefColorB = 0xFF;
            resErrColorR = 0xFF;
            resErrColorG = 0;
            resErrColorB = 0;
            resWarColorR = 0xFF;
            resWarColorG = 0xFF;
            resWarColorB = 0;
            resNoColorR = 0xA9;
            resNoColorG = 0xA9;
            resNoColorB = 0xA9;
            recEndDefColorR = 0xFF;
            recEndDefColorG = 0xFF;
            recEndDefColorB = 0xFF;
            recEndErrColorR = 0xFF;
            recEndErrColorG = 0;
            recEndErrColorB = 0;
            recEndWarColorR = 0xFF;
            recEndWarColorG = 0xFF;
            recEndWarColorB = 0;
            epgTipsBackColorR = 0xD3;
            epgTipsBackColorG = 0xD3;
            epgTipsBackColorB = 0xD3;
            epgTipsForeColorR = 0;
            epgTipsForeColorG = 0;
            epgTipsForeColorB = 0;
            epgInfoSingleClick = false;
            epgInfoOpenMode = 0;
            execBat = 0;
            suspendChk = 0;
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

        //番組表のデフォルトの背景色
        private static void DefaultcontentColorList()
        {
            Instance.contentColorList.Add("LightYellow");
            Instance.contentColorList.Add("Lavender");
            Instance.contentColorList.Add("LavenderBlush");
            Instance.contentColorList.Add("MistyRose");
            Instance.contentColorList.Add("Honeydew");
            Instance.contentColorList.Add("LightCyan");
            Instance.contentColorList.Add("PapayaWhip");
            Instance.contentColorList.Add("Pink");
            Instance.contentColorList.Add("LightYellow");
            Instance.contentColorList.Add("PapayaWhip");
            Instance.contentColorList.Add("AliceBlue");
            Instance.contentColorList.Add("AliceBlue");
            Instance.contentColorList.Add("White");
            Instance.contentColorList.Add("White");
            Instance.contentColorList.Add("White");
            Instance.contentColorList.Add("WhiteSmoke");
            Instance.contentColorList.Add("White");
        }

        //番組表の時間軸のデフォルトの背景色
        private static void DefaulttimeColorList()
        {
            Instance.timeColorList.Add("MediumPurple");
            Instance.timeColorList.Add("LightSeaGreen");
            Instance.timeColorList.Add("LightSalmon");
            Instance.timeColorList.Add("CornflowerBlue");
        }

        /// <summary>
        /// EpgTimer用設定ファイルロード関数
        /// </summary>
        public static void LoadFromXmlFile()
        {
            string path = GetSettingPath();
            try
            {
                FileStream fs = new FileStream(path,
                    FileMode.Open,
                    FileAccess.Read);
                System.Xml.Serialization.XmlSerializer xs =
                    new System.Xml.Serialization.XmlSerializer(
                        typeof(Settings));
                //読み込んで逆シリアル化する
                object obj = xs.Deserialize(fs);
                fs.Close();
                Instance = (Settings)obj;

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
                            try
                            {
                                FileStream fs = new FileStream(backPath,
                                    FileMode.Open,
                                    FileAccess.Read, FileShare.None);
                                System.Xml.Serialization.XmlSerializer xs =
                                    new System.Xml.Serialization.XmlSerializer(
                                        typeof(Settings));
                                //読み込んで逆シリアル化する
                                object obj = xs.Deserialize(fs);
                                fs.Close();
                                Instance = (Settings)obj;
                            }
                            catch (Exception ex2)
                            {
                                MessageBox.Show(ex2.Message + "\r\n" + ex2.StackTrace);
                            }
                        }
                    }
                    else
                    {
                        MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                    }
                }
            }
            finally
            {
                if (Instance.contentColorList.Count == 0)
                {
                    DefaultcontentColorList();
                }
                else if (Instance.contentColorList.Count == 0x10)
                {
                    Instance.contentColorList.Add("White");
                }
                if (Instance.ContentCustColorList.Count == 0)
                {
                    for (int i = 0; i < 0x11+4; i++)
                    {
                        Instance.ContentCustColorList.Add(0xFFFFFFFF);
                    }
                }
                if (Instance.timeColorList.Count == 0)
                {
                    DefaulttimeColorList();
                }
                if (Instance.TimeCustColorList.Count == 0)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        Instance.TimeCustColorList.Add(0xFFFFFFFF);
                    }
                }
                if (Instance.viewButtonList.Count == 0)
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
                if (Instance.taskMenuList.Count == 0)
                {
                    Instance.taskMenuList.Add("設定");
                    Instance.taskMenuList.Add("（セパレータ）");
                    Instance.taskMenuList.Add("スタンバイ");
                    Instance.taskMenuList.Add("休止");
                    Instance.taskMenuList.Add("（セパレータ）");
                    Instance.taskMenuList.Add("終了");
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
        }
 
        /// <summary>
        /// EpgTimerNW用の設定ファイルロード関数
        /// </summary>
        public static void LoadFromXmlFileNW()
        {
            string path = GetSettingPath();

            try
            {
                FileStream fs = new FileStream(path,
                    FileMode.Open,
                    FileAccess.Read, FileShare.None);
                System.Xml.Serialization.XmlSerializer xs =
                    new System.Xml.Serialization.XmlSerializer(
                        typeof(Settings));
                //読み込んで逆シリアル化する
                object obj = xs.Deserialize(fs);
                fs.Close();
                Instance = (Settings)obj;

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
                            try
                            {
                                FileStream fs = new FileStream(backPath,
                                    FileMode.Open,
                                    FileAccess.Read, FileShare.None);
                                System.Xml.Serialization.XmlSerializer xs =
                                    new System.Xml.Serialization.XmlSerializer(
                                        typeof(Settings));
                                //読み込んで逆シリアル化する
                                object obj = xs.Deserialize(fs);
                                fs.Close();
                                Instance = (Settings)obj;
                            }
                            catch (Exception ex2)
                            {
                                MessageBox.Show(ex2.Message + "\r\n" + ex2.StackTrace);
                            }
                        }
                    }
                    else
                    {
                        MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                    }
                }
            }
            finally
            {
                if (Instance.contentColorList.Count == 0)
                {
                    DefaultcontentColorList();
                }
                else if (Instance.contentColorList.Count == 0x10)
                {
                    Instance.contentColorList.Add("White");
                }
                if (Instance.ContentCustColorList.Count == 0)
                {
                    for (int i = 0; i < 0x11+4; i++)
                    {
                        Instance.ContentCustColorList.Add(0xFFFFFFFF);
                    }
                }
                if (Instance.timeColorList.Count == 0)
                {
                    DefaulttimeColorList();
                }
                if (Instance.TimeCustColorList.Count == 0)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        Instance.TimeCustColorList.Add(0xFFFFFFFF);
                    }
                }
                if (Instance.viewButtonList.Count == 0)
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
                if (Instance.taskMenuList.Count == 0)
                {
                    Instance.taskMenuList.Add("設定");
                    Instance.taskMenuList.Add("（セパレータ）");
                    Instance.taskMenuList.Add("再接続");
                    Instance.taskMenuList.Add("（セパレータ）");
                    Instance.taskMenuList.Add("終了");
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
                //Instance.nwTvMode = true;
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

        public static void GetDefSearchSetting(ref EpgSearchKeyInfo defKey)
        {
            if (Settings.Instance.SearchKeyRegExp == true)
            {
                defKey.regExpFlag = 1;
            }
            if (Settings.Instance.SearchKeyAimaiFlag == true)
            {
                defKey.aimaiFlag = 1;
            }
            if (Settings.Instance.SearchKeyTitleOnly == true)
            {
                defKey.titleOnlyFlag = 1;
            }
            if (Settings.Instance.SearchKeyNotContent == true)
            {
                defKey.notContetFlag = 1;
            }
            if (Settings.Instance.SearchKeyNotDate == true)
            {
                defKey.notDateFlag = 1;
            }
            foreach (ContentKindInfo info in Settings.Instance.SearchKeyContentList)
            {
                EpgContentData item = new EpgContentData();
                item.content_nibble_level_1 = info.Nibble1;
                item.content_nibble_level_2 = info.Nibble2;
                defKey.contentList.Add(item);
            }
            foreach (DateItem info in Settings.Instance.SearchKeyDateItemList)
            {
                defKey.dateList.Add(info.DateInfo);
            }
            foreach (Int64 info in Settings.Instance.SearchKeyServiceList)
            {
                defKey.serviceList.Add(info);
            }
            defKey.freeCAFlag = Settings.Instance.SearchKeyFreeCA;
            defKey.chkRecEnd = Settings.Instance.SearchKeyChkRecEnd;
            defKey.chkRecDay = Settings.Instance.SearchKeyChkRecDay;
        }
    }
}
