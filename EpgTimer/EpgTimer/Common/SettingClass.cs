﻿﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;
using System.Reflection;
using System.Collections;
using Microsoft.Win32;
using System.Runtime.InteropServices;
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;

namespace EpgTimer
{
    using UserCtrlView;

    class IniFileHandler
    {
        [DllImport("KERNEL32.DLL", CharSet = CharSet.Unicode)]
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

        public static double GetPrivateProfileDouble(string lpAppName, string lpKeyName, double dDefault, string lpFileName)
        {
            string s = IniFileHandler.GetPrivateProfileString(lpAppName, lpKeyName, dDefault.ToString(), lpFileName);
            double.TryParse(s, out dDefault);
            return dDefault;
        }

        /// <summary>INIファイルから指定セクションのキー一覧を取得する。lpAppNameにnullを指定すると全セクションの一覧を取得する。</summary>
        public static string[] GetPrivateProfileKeys(string lpAppName, string lpFileName)
        {
            byte[] buff = null;
            uint resultSize = 0;
            for (uint n = 1024; n <= 1024 * 1024; n *= 2)
            {
                buff = new byte[n];
                resultSize = GetPrivateProfileStringByByteArray(lpAppName, null, null, buff, n, lpFileName);
                if (resultSize < n - 2)
                {
                    break;
                }
            }
            return Encoding.Default.GetString(buff, 0, (int)resultSize).TrimEnd('\0').Split('\0');
        }

        /// <summary>INIファイルから連番のキー/セクションを削除する。lpAppNameにnullを指定すると連番セクションを削除する。</summary>
        public static void DeletePrivateProfileNumberKeys(string lpAppName, string lpFileName, string BaseFront = "", string BaseRear = "", bool deleteBaseName = false)
        {
            string numExp = string.IsNullOrEmpty(BaseFront + BaseRear) == false && deleteBaseName  == true ? "*" : "+";
            foreach (string key in IniFileHandler.GetPrivateProfileKeys(lpAppName, lpFileName))
            {
                if (Regex.Match(key, "^" + BaseFront + "\\d" + numExp + BaseRear + "$").Success == true)
                {
                    if (lpAppName != null)
                    {
                        IniFileHandler.WritePrivateProfileString(lpAppName, key, null, lpFileName);
                    }
                    else
                    {
                        IniFileHandler.WritePrivateProfileString(key, null, null, lpFileName);
                    }
                }
            }
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
            get { return ModulePath.TrimEnd('\\') + "\\Setting" + (CommonManager.Instance.NWMode == false ? "" : "\\EpgTimerNW"); }
        }
        public static string SettingFolderPath
        {
            get
            {
                string path = DefSettingFolderPath;
                if (CommonManager.Instance.NWMode == false)
                {
                    path = IniFileHandler.GetPrivateProfileString("SET", "DataSavePath", path, CommonIniPath);
                }
                else
                {
                    path = Settings.Instance.SettingFolderPathNW == "" ? path : Settings.Instance.SettingFolderPathNW;
                }
                return (Path.IsPathRooted(path) ? "" : ModulePath.TrimEnd('\\') + "\\") + path;
            }
            set
            {
                string path = value.Trim();
                bool isDefaultPath = string.Compare(path.TrimEnd('\\'), SettingPath.DefSettingFolderPath.TrimEnd('\\'), true) == 0;
                if (CommonManager.Instance.NWMode == false)
                {
                    IniFileHandler.WritePrivateProfileString("SET", "DataSavePath", isDefaultPath == true ? null : path, SettingPath.CommonIniPath);
                }
                else
                {
                    Settings.Instance.SettingFolderPathNW = isDefaultPath == true ? "" : path;
                }
            }
        }
        public static string ModulePath
        {
            get { return Path.GetDirectoryName(Assembly.GetEntryAssembly().Location); }
        }
        public static string ModuleName
        {
            get { return Path.GetFileNameWithoutExtension(Assembly.GetExecutingAssembly().Location); }
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
        public double ToolTipWidth { get; set; }
        public bool NoBallonTips { get; set; }
        public int ForceHideBalloonTipSec { get; set; }
        public bool PlayDClick { get; set; }
        public bool RecinfoErrCriticalDrops { get; set; }
        public double DragScroll { get; set; }
        public List<string> ContentColorList { get; set; }
        public List<UInt32> ContentCustColorList { get; set; }
        public List<string> EpgEtcColors { get; set; }
        public List<UInt32> EpgEtcCustColors { get; set; }
        public string ReserveRectColorNormal { get; set; }
        public string ReserveRectColorNo { get; set; }
        public string ReserveRectColorNoTuner { get; set; }
        public string ReserveRectColorWarning { get; set; }
        public string ReserveRectColorAutoAddMissing { get; set; }
        public string ReserveRectColorMultiple { get; set; }
        public bool ReserveRectBackground { get; set; }
        public string TitleColor1 { get; set; }
        public string TitleColor2 { get; set; }
        public UInt32 TitleCustColor1 { get; set; }
        public UInt32 TitleCustColor2 { get; set; }
        public string TunerFontNameService { get; set; }
        public double TunerFontSizeService { get; set; }
        public bool TunerFontBoldService { get; set; }
        public string TunerFontName { get; set; }
        public double TunerFontSize { get; set; }
        public List<string> TunerServiceColors { get; set; }
        public List<UInt32> TunerServiceCustColors { get; set; }
        public double TunerMinHeight { get; set; }
        public double TunerMinimumLine { get; set; }
        public double TunerDragScroll { get; set; }
        public double TunerScrollSize { get; set; }
        public bool TunerMouseScrollAuto { get; set; }
        public double TunerWidth { get; set; }
        public bool TunerServiceNoWrap { get; set; }
        public bool TunerTitleIndent { get; set; }
        public bool TunerToolTip { get; set; }
        public int TunerToolTipViewWait { get; set; }
        public bool TunerPopup { get; set; }
        public int TunerPopupMode { get; set; }
        public bool TunerPopupRecinfo { get; set; }
        public double TunerPopupWidth { get; set; }
        public bool TunerInfoSingleClick { get; set; }
        public bool TunerColorModeUse { get; set; }
        public bool TunerDisplayOffReserve { get; set; }
        public bool EpgTitleIndent { get; set; }
        public bool EpgToolTip { get; set; }
        public bool EpgToolTipNoViewOnly { get; set; }
        public int EpgToolTipViewWait { get; set; }
        public bool EpgPopup { get; set; }
        public int EpgPopupMode { get; set; }
        public double EpgPopupWidth { get; set; }
        public bool EpgExtInfoTable { get; set; }
        public bool EpgExtInfoPopup { get; set; }
        public bool EpgExtInfoTooltip { get; set; }
        public bool EpgGradation { get; set; }
        public bool EpgGradationHeader { get; set; }
        public bool EpgLoadArcInfo { get; set; }
        public bool EpgNoDisplayOld { get; set; }
        public double EpgNoDisplayOldDays { get; set; }
        public string ResColumnHead { get; set; }
        public ListSortDirection ResSortDirection { get; set; }
        public WindowSettingData WndSettings { get; set; }
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
        public string Cust3BtnName { get; set; }
        public string Cust3BtnCmd { get; set; }
        public string Cust3BtnCmdOpt { get; set; }
        public List<string> AndKeyList { get; set; }
        public List<string> NotKeyList { get; set; }
        public EpgSearchKeyInfo DefSearchKey { get; set; }
        private List<RecPresetItem> recPresetList = null;
        [System.Xml.Serialization.XmlIgnore]
        public List<RecPresetItem> RecPresetList
        {
            get
            {
                if (recPresetList == null)
                {
                    recPresetList = RecPresetItem.LoadRecPresetList();
                }
                return recPresetList;
            }
            set { recPresetList = value; }
        }
        public string RecInfoColumnHead { get; set; }
        public ListSortDirection RecInfoSortDirection { get; set; }
        public long RecInfoDropErrIgnore { get; set; }
        public long RecInfoDropWrnIgnore { get; set; }
        public long RecInfoScrambleIgnore { get; set; }
        public List<string> RecInfoDropExclude { get; set; }
        public bool RecInfoNoYear { get; set; }
        public bool RecInfoNoSecond { get; set; }
        public bool RecInfoNoDurSecond { get; set; }
        public bool ResInfoNoYear { get; set; }
        public bool ResInfoNoSecond { get; set; }
        public bool ResInfoNoDurSecond { get; set; }
        public string TvTestExe { get; set; }
        public string TvTestCmd { get; set; }
        public bool NwTvMode { get; set; }
        public bool NwTvModeUDP { get; set; }
        public bool NwTvModeTCP { get; set; }
        public string FilePlayExe { get; set; }
        public string FilePlayCmd { get; set; }
        public bool FilePlayOnAirWithExe { get; set; }
        public bool FilePlayOnNwWithExe { get; set; }
        public bool OpenFolderWithFileDialog { get; set; }
        public List<IEPGStationInfo> IEpgStationList { get; set; }
        public MenuSettingData MenuSet { get; set; }
        public string NWServerIP { get; set; }
        public UInt32 NWServerPort { get; set; }
        public UInt32 NWWaitPort { get; set; }
        public string NWMacAdd { get; set; }
        public List<NWPresetItem> NWPreset { get; set; }
        public bool WakeReconnectNW { get; set; }
        public bool WoLWait { get; set; }
        public bool WoLWaitRecconect { get; set; }
        public double WoLWaitSecond { get; set; }
        public bool SuspendCloseNW { get; set; }
        public bool NgAutoEpgLoadNW { get; set; }
        public bool ChkSrvRegistTCP { get; set; }
        public double ChkSrvRegistInterval { get; set; }
        public Int32 TvTestOpenWait { get; set; }
        public Int32 TvTestChgBonWait { get; set; }
        public List<string> RecEndColors { get; set; }
        public List<uint> RecEndCustColors { get; set; }
        public string ListDefColor { get; set; }
        public UInt32 ListDefCustColor { get; set; }
        public List<string> RecModeFontColors { get; set; }
        public List<uint> RecModeFontCustColors { get; set; }
        public List<string> ResBackColors { get; set; }
        public List<uint> ResBackCustColors { get; set; }
        public List<string> StatColors { get; set; }
        public List<uint> StatCustColors { get; set; }
        public bool EpgInfoSingleClick { get; set; }
        public byte EpgInfoOpenMode { get; set; }
        public UInt32 ExecBat { get; set; }
        public UInt32 SuspendChk { get; set; }
        public UInt32 SuspendChkTime { get; set; }
        public List<ListColumnInfo> ReserveListColumn { get; set; }
        public List<ListColumnInfo> RecInfoListColumn { get; set; }
        public List<ListColumnInfo> AutoAddEpgColumn { get; set; }
        public List<ListColumnInfo> AutoAddManualColumn { get; set; }
        public List<ListColumnInfo> EpgListColumn { get; set; }
        public string EpgListColumnHead { get; set; }
        public ListSortDirection EpgListSortDirection { get; set; }
        public List<ListColumnInfo> SearchWndColumn { get; set; }
        public string SearchColumnHead { get; set; }
        public ListSortDirection SearchSortDirection { get; set; }
        public bool SaveSearchKeyword { get; set; }
        public List<ListColumnInfo> InfoSearchWndColumn { get; set; }
        public string InfoSearchColumnHead { get; set; }
        public ListSortDirection InfoSearchSortDirection { get; set; }
        public string InfoSearchLastWord { get; set; }
        public bool InfoSearchTitleOnly { get; set; }
        public bool InfoSearchReserveInfo { get; set; }
        public bool InfoSearchRecInfo { get; set; }
        public bool InfoSearchEpgAutoAddInfo { get; set; }
        public bool InfoSearchManualAutoAddInfo { get; set; }
        public bool InfoSearchItemTooltip { get; set; }
        public short AutoSaveNotifyLog { get; set; }
        public bool ShowTray { get; set; }
        public bool MinHide { get; set; }
        public bool MouseScrollAuto { get; set; }
        public int NoStyle { get; set; }
        public bool CautionManyChange { get; set; }
        public int CautionManyNum { get; set; }
        public bool CautionOnRecChange { get; set; }
        public int CautionOnRecMarginMin { get; set; }
        public bool SyncResAutoAddChange { get; set; }
        public bool SyncResAutoAddChgNewRes { get; set; }
        public bool SyncResAutoAddDelete { get; set; }
        public bool DisplayNotifyEpgChange { get; set; }
        public double DisplayNotifyJumpTime { get; set; }
        public bool DisplayReserveAutoAddMissing { get; set; }
        public bool DisplayReserveMultiple { get; set; }
        public bool TryEpgSetting { get; set; }
        public bool LaterTimeUse { get; set; }
        public int LaterTimeHour { get; set; }
        public bool DisplayPresetOnSearch { get; set; }
        public bool ForceNWMode { get; set; }
        public string SettingFolderPathNW { get; set; }
        public bool RecInfoExtraDataCache { get; set; }
        public bool RecInfoExtraDataCacheOptimize { get; set; }
        public bool RecInfoExtraDataCacheKeepConnect { get; set; }
        public bool UpdateTaskText { get; set; }
        public bool DisplayStatus { get; set; }
        public bool DisplayStatusNotify { get; set; }
        public bool IsVisibleReserveView { get; set; }
        public bool IsVisibleRecInfoView { get; set; }
        public bool IsVisibleAutoAddView { get; set; }
        public bool IsVisibleAutoAddViewMoveOnly { get; set; }
        public Dock MainViewButtonsDock { get; set; }
        public CtxmCode StartTab { get; set; }
        public bool TrimSortTitle { get; set; }
        public bool NotifyWindowAutoReload { get; set; }

        public Settings()
        {
            ResetColorSetting();
            
            UseCustomEpgView = false;
            CustomEpgTabList = new List<CustomEpgTabInfo>();
            MinHeight = 2;
            MinimumHeight = 0;
            ServiceWidth = 150;
            ScrollSize = 240;
            FontName = System.Drawing.SystemFonts.DefaultFont.Name;
            FontSize = 12;
            FontNameTitle = System.Drawing.SystemFonts.DefaultFont.Name;
            FontSizeTitle = 12;
            FontBoldTitle = true;
            NoToolTip = false;
            ToolTipWidth = 400;
            NoBallonTips = false;
            ForceHideBalloonTipSec = 0;
            PlayDClick = false;
            RecinfoErrCriticalDrops = false;
            DragScroll = 1.5;
            TunerFontNameService = System.Drawing.SystemFonts.DefaultFont.Name;
            TunerFontSizeService = 12;
            TunerFontBoldService = true;
            TunerFontName = System.Drawing.SystemFonts.DefaultFont.Name;
            TunerFontSize = 12;
            TunerMinHeight = 2;
            TunerMinimumLine = 0;
            TunerDragScroll = 1.5;
            TunerScrollSize = 240;
            TunerMouseScrollAuto = false;
            TunerWidth = 150;
            TunerServiceNoWrap = true;
            TunerTitleIndent = true;
            TunerToolTip = false;
            TunerToolTipViewWait = 1500;
            TunerPopup = false;
            TunerPopupMode = 0;
            TunerPopupRecinfo = false;
            TunerInfoSingleClick = false;
            TunerPopupWidth = 1;
            TunerColorModeUse = false;
            TunerDisplayOffReserve = false;
            EpgTitleIndent = true;
            EpgToolTip = false;
            EpgToolTipNoViewOnly = true;
            EpgToolTipViewWait = 1500;
            EpgPopup = true;
            EpgPopupMode = 0;
            EpgPopupWidth = 1;
            EpgExtInfoTable = false;
            EpgExtInfoPopup = false;
            EpgExtInfoTooltip = true;
            EpgGradation = true;
            EpgGradationHeader = true;
            EpgLoadArcInfo = false;
            EpgNoDisplayOld = false;
            EpgNoDisplayOldDays = 1;
            ResColumnHead = "";
            ResSortDirection = ListSortDirection.Ascending;
            WndSettings = new WindowSettingData();
            CloseMin = false;
            WakeMin = false;
            ViewButtonShowAsTab = false;
            ViewButtonList = new List<string>();
            TaskMenuList = new List<string>();
            Cust1BtnName = "";
            Cust1BtnCmd = "";
            Cust1BtnCmdOpt = "";
            Cust2BtnName = "";
            Cust2BtnCmd = "";
            Cust2BtnCmdOpt = "";
            Cust3BtnName = "";
            Cust3BtnCmd = "";
            Cust3BtnCmdOpt = "";
            AndKeyList = new List<string>();
            NotKeyList = new List<string>();
            DefSearchKey = new EpgSearchKeyInfo();
            RecInfoColumnHead = "";
            RecInfoSortDirection = ListSortDirection.Ascending;
            RecInfoDropErrIgnore = 0;
            RecInfoDropWrnIgnore = 0;
            RecInfoScrambleIgnore = 0;
            RecInfoDropExclude = new List<string>();
            RecInfoNoYear = false;
            RecInfoNoSecond = false;
            RecInfoNoDurSecond = false;
            ResInfoNoYear = false;
            ResInfoNoSecond = false;
            ResInfoNoDurSecond = false;
            TvTestExe = "";
            TvTestCmd = "";
            NwTvMode = false;
            NwTvModeUDP = false;
            NwTvModeTCP = false;
            FilePlayExe = "";
            FilePlayCmd = "\"$FilePath$\"";
            FilePlayOnAirWithExe = false;
            FilePlayOnNwWithExe = false;
            OpenFolderWithFileDialog = false;
            IEpgStationList = new List<IEPGStationInfo>();
            MenuSet = new MenuSettingData();
            NWServerIP = "";
            NWServerPort = 4510;
            NWWaitPort = 0;
            NWMacAdd = "";
            NWPreset = new List<NWPresetItem>();
            WakeReconnectNW = false;
            WoLWaitRecconect = false;
            WoLWaitSecond= 30;
            WoLWait = false;
            SuspendCloseNW = false;
            NgAutoEpgLoadNW = false;
            ChkSrvRegistTCP = false;
            ChkSrvRegistInterval = 5;
            TvTestOpenWait = 2000;
            TvTestChgBonWait = 2000;
            EpgInfoSingleClick = false;
            EpgInfoOpenMode = 0;
            ExecBat = 0;
            SuspendChk = 0;
            SuspendChkTime = 15;
            ReserveListColumn = new List<ListColumnInfo>();
            RecInfoListColumn = new List<ListColumnInfo>();
            AutoAddEpgColumn = new List<ListColumnInfo>();
            AutoAddManualColumn = new List<ListColumnInfo>();
            EpgListColumn = new List<ListColumnInfo>();
            EpgListColumnHead = "";
            EpgListSortDirection = ListSortDirection.Ascending;
            SearchWndColumn = new List<ListColumnInfo>();
            SearchColumnHead = "";
            SearchSortDirection = ListSortDirection.Ascending;
            SaveSearchKeyword = true;
            InfoSearchWndColumn = new List<ListColumnInfo>();
            InfoSearchColumnHead = "";
            InfoSearchSortDirection = ListSortDirection.Ascending;
            InfoSearchLastWord = "";
            InfoSearchTitleOnly = true;
            InfoSearchReserveInfo = true;
            InfoSearchRecInfo = true;
            InfoSearchEpgAutoAddInfo = true;
            InfoSearchManualAutoAddInfo = true;
            InfoSearchItemTooltip = true;
            AutoSaveNotifyLog = 0;
            ShowTray = true;
            MinHide = true;
            MouseScrollAuto = false;
            NoStyle = 0;
            CautionManyChange = true;
            CautionManyNum = 10;
            CautionOnRecChange = true;
            CautionOnRecMarginMin = 5;
            SyncResAutoAddChange = false;
            SyncResAutoAddChgNewRes = false;
            SyncResAutoAddDelete = false;
            DisplayNotifyEpgChange = false;
            DisplayNotifyJumpTime = 3;
            DisplayReserveAutoAddMissing = false;
            DisplayReserveMultiple = true;
            TryEpgSetting = true;
            LaterTimeUse = false;
            LaterTimeHour = 28 - 24;
            DisplayPresetOnSearch = false;
            ForceNWMode = false;
            SettingFolderPathNW = "";
            RecInfoExtraDataCache = true;
            RecInfoExtraDataCacheOptimize = true;
            RecInfoExtraDataCacheKeepConnect = false;
            UpdateTaskText = false;
            DisplayStatus = false;
            DisplayStatusNotify = false;
            IsVisibleReserveView = true;
            IsVisibleRecInfoView = true;
            IsVisibleAutoAddView = true;
            IsVisibleAutoAddViewMoveOnly = false;
            MainViewButtonsDock = Dock.Right;
            StartTab = CtxmCode.ReserveView;
            TrimSortTitle = false;
            NotifyWindowAutoReload = false;
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
                nwMode |= Settings.Instance.ForceNWMode;

                // タイミング合わせにくいので、メニュー系のデータチェックは
                // MenuManager側のワークデータ作成時に実行する。

                SetCustomEpgTabInfoID();

                //色設定関係
                Instance.SetColorSetting();

                if (Instance.ViewButtonList.Count == 0)
                {
                    Instance.ViewButtonList.AddRange(GetViewButtonDefIDs(nwMode));
                }
                if (Instance.TaskMenuList.Count == 0)
                {
                    Instance.TaskMenuList.AddRange(GetTaskMenuDefIDs(nwMode));
                }
                if (Instance.ReserveListColumn.Count == 0)
                {
                    var obj = new ReserveItem();
                    Instance.ReserveListColumn.AddRange(GetDefaultColumn(typeof(ReserveView)));
                    Instance.ResColumnHead = CommonUtil.NameOf(() => obj.StartTime);
                    Instance.ResSortDirection = ListSortDirection.Ascending;
                }
                if (Instance.RecInfoListColumn.Count == 0)
                {
                    var obj = new RecInfoItem();
                    Instance.RecInfoListColumn.AddRange(GetDefaultColumn(typeof(RecInfoView)));
                    Instance.RecInfoColumnHead = CommonUtil.NameOf(() => obj.StartTime);
                    Instance.RecInfoSortDirection = ListSortDirection.Descending;
                }
                if (Instance.AutoAddEpgColumn.Count == 0)
                {
                    Instance.AutoAddEpgColumn.AddRange(GetDefaultColumn(typeof(EpgAutoAddView)));
                }
                if (Instance.AutoAddManualColumn.Count == 0)
                {
                    Instance.AutoAddManualColumn.AddRange(GetDefaultColumn(typeof(ManualAutoAddView)));
                }
                if (Instance.EpgListColumn.Count == 0)
                {
                    var obj = new SearchItem();
                    Instance.EpgListColumn.AddRange(GetDefaultColumn(typeof(EpgListMainView)));
                    Instance.EpgListColumnHead = CommonUtil.NameOf(() => obj.StartTime);
                    Instance.EpgListSortDirection = ListSortDirection.Ascending;
                }
                if (Instance.SearchWndColumn.Count == 0)
                {
                    var obj = new SearchItem();
                    Instance.SearchWndColumn.AddRange(GetDefaultColumn(typeof(SearchWindow)));
                    Instance.SearchColumnHead = CommonUtil.NameOf(() => obj.StartTime);
                    Instance.SearchSortDirection = ListSortDirection.Ascending;
                }
                if (Instance.InfoSearchWndColumn.Count == 0)
                {
                    var obj = new InfoSearchItem();
                    Instance.InfoSearchWndColumn.AddRange(GetDefaultColumn(typeof(InfoSearchWindow)));
                    Instance.InfoSearchColumnHead = CommonUtil.NameOf(() => obj.StartTime);
                    Instance.InfoSearchSortDirection = ListSortDirection.Ascending;
                }
                if (Instance.RecInfoDropExclude.Count == 0)
                {
                    Settings.Instance.RecInfoDropExclude = new List<string> { "EIT", "NIT", "CAT", "SDT", "SDTT", "TOT", "ECM", "EMM" };
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

                using (var fs = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.None))
                {
                    //シリアル化して書き込む
                    var xs = new System.Xml.Serialization.XmlSerializer(typeof(Settings));
                    xs.Serialize(fs, Instance);
                }
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
            DefStartMargin = IniFileHandler.GetPrivateProfileInt("SET", "StartMargin", 5, SettingPath.TimerSrvIniPath);
            DefEndMargin = IniFileHandler.GetPrivateProfileInt("SET", "EndMargin", 2, SettingPath.TimerSrvIniPath);
            defRecfolders = null;
        }

        //デフォルトマージン
        [System.Xml.Serialization.XmlIgnore]
        public int DefStartMargin { get; private set; }
        [System.Xml.Serialization.XmlIgnore]
        public int DefEndMargin { get; private set; }

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

        private static List<string> viewButtonIDs = new List<string>();
        public const string ViewButtonSpacer = "（空白）";
        public const string TaskMenuSeparator = "（セパレータ）";
        public static void ResisterViewButtonIDs(IEnumerable<string> list)
        {
            viewButtonIDs = list == null ? new List<string>() : list.ToList();
        } 
        public static List<string> GetViewButtonAllIDs()
        {
            return new List<string> { ViewButtonSpacer }.Concat(viewButtonIDs).ToList();
        }
        public static List<string> GetTaskMenuAllIDs()
        {
            return new List<string> { TaskMenuSeparator }.Concat(viewButtonIDs).ToList();
        }
        public static List<string> GetViewButtonDefIDs(bool nwMode)
        {
            if (nwMode == false)
            {
                return new List<string>
                {
                    "設定",
                    ViewButtonSpacer,
                    "検索",
                    ViewButtonSpacer,
                    "スタンバイ",
                    "休止",
                    ViewButtonSpacer,
                    "EPG取得",
                    ViewButtonSpacer,
                    "EPG再読み込み",
                    ViewButtonSpacer,
                    "終了",
                };
            }
            else
            {
                return new List<string>
                {
                    "設定",
                    ViewButtonSpacer,
                    "再接続",
                    ViewButtonSpacer,
                    "検索",
                    ViewButtonSpacer,
                    "EPG取得",
                    ViewButtonSpacer,
                    "EPG再読み込み",
                    ViewButtonSpacer,
                    "終了",
                };
            }
        }
        public static List<string> GetTaskMenuDefIDs(bool nwMode)
        {
            if (nwMode == false)
            {
                return new List<string>
                {
                    "設定",
                    TaskMenuSeparator,
                    "スタンバイ",
                    "休止",
                    TaskMenuSeparator,
                    "終了",
                };
            }
            else
            {
                return new List<string>
                {
                    "設定",
                    TaskMenuSeparator,
                    "再接続",
                    TaskMenuSeparator,
                    "終了",
                };
            }
        }

        public static List<ListColumnInfo> GetDefaultColumn(Type t)
        {
            if (t == typeof(ReserveView))
            {
                var obj = new ReserveItem();
                return new List<ListColumnInfo>
                {
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.StartTime) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.NetworkName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.ServiceName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.EventName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.RecMode) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.Priority) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.Tuijyu) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.Comment) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.RecFileName) },
                };
            }
            else if (t == typeof(RecInfoView))
            {
                var obj = new RecInfoItem();
                return new List<ListColumnInfo>
                {
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.IsProtect) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.StartTime) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.NetworkName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.ServiceName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.EventName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.Drops) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.Scrambles) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.Result) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.RecFilePath) },
                };
            }
            else if (t == typeof(EpgAutoAddView))
            {
                var obj = new EpgAutoDataItem();
                return new List<ListColumnInfo>
                {
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.EventName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.NotKey) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.RegExp) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.RecMode) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.Priority) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.Tuijyu) },
                };
            }
            else if (t == typeof(ManualAutoAddView))
            {
                var obj = new ManualAutoAddDataItem();
                return new List<ListColumnInfo>
                {
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.DayOfWeek) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.StartTime) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.EventName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.ServiceName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.RecMode) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.Priority) },
                };
            }
            else if (t == typeof(EpgListMainView))
            {
                var obj = new SearchItem();
                return new List<ListColumnInfo>
                {
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.Status) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.StartTime) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.NetworkName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.ServiceName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.EventName) },
                };
            }
            else if (t == typeof(SearchWindow))
            {
                var obj = new SearchItem();
                return new List<ListColumnInfo>
                {
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.Status) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.StartTime) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.ProgramDuration) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.EventName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.NetworkName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.ServiceName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.ProgramContent) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.JyanruKey) },
                };
            }
            else if (t == typeof(InfoSearchWindow))
            {
                var obj = new InfoSearchItem();
                return new List<ListColumnInfo>
                {
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.ViewItemName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.Status) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.StartTime) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.ProgramDuration) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.NetworkName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.ServiceName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.EventName) },
                    new ListColumnInfo { Tag = CommonUtil.NameOf(() => obj.ProgramContent) },
                };
            }
            return null;
        }

        public void ResetColorSetting()
        {
            ContentColorList = new List<string>();
            ContentCustColorList = new List<uint>();
            EpgEtcColors = new List<string>();
            EpgEtcCustColors = new List<uint>();
            ReserveRectColorNormal = "Lime";
            ReserveRectColorNo = "Black";
            ReserveRectColorNoTuner = "Red";
            ReserveRectColorWarning = "Yellow";
            ReserveRectColorAutoAddMissing = "Blue";
            ReserveRectColorMultiple = "Purple";
            ReserveRectBackground = false;
            TitleColor1 = "Black";
            TitleColor2 = "Black";
            TitleCustColor1 = 0xFFFFFFFF;
            TitleCustColor2 = 0xFFFFFFFF;
            TunerServiceColors = new List<string>();
            TunerServiceCustColors = new List<uint>();
            RecEndColors = new List<string>();
            RecEndCustColors = new List<uint>();
            ListDefColor = "カスタム";
            ListDefCustColor = 0xFF042271;
            RecModeFontColors = new List<string>();
            RecModeFontCustColors = new List<uint>();
            ResBackColors = new List<string>();
            ResBackCustColors = new List<uint>();
            StatColors = new List<string>();
            StatCustColors = new List<uint>();
        }
        //色リストの初期化用
        private static void _FillList<T>(List<T> list, T val, int num)
        {
            if (list.Count < num) list.AddRange(Enumerable.Repeat(val, num - list.Count));
        }
        private static void _FillList<T>(List<T> list, IEnumerable<T> val)
        {
            if (list.Count < val.Count()) list.AddRange(val.Skip(list.Count));
        }
        public void SetColorSetting()
        {
            int num;
            //番組表の背景色
            num = 0x11;//番組表17色。過去に16色時代があった。
            if (ContentColorList.Count < num)
            {
                var defColors = new string[]{
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
                _FillList(ContentColorList, defColors);
            }
            num = 0x11 + 6;//番組表17色+予約枠6色
            _FillList(ContentCustColorList, 0xFFFFFFFF, num);

            //番組表の時間軸のデフォルトの背景色、その他色
            num = 12;
            if (EpgEtcColors.Count < num)
            {
                var defColors = new string[]{
                        "MediumPurple"      //00-05時
                        ,"LightSeaGreen"    //06-11時
                        ,"LightSalmon"      //12-17時
                        ,"CornflowerBlue"   //18-23時
                        ,"LightSlateGray"   //サービス背景色
                        ,"DarkGray"         //番組表背景色
                        ,"DarkGray"         //番組枠色
                        ,"White"            //サービス文字
                        ,"Silver"           //サービス枠色
                        ,"White"            //時間文字色
                        ,"White"            //時間枠色
                        ,"LightSlateGray"   //一週間モード日付枠色
                    };
                _FillList(EpgEtcColors, defColors);
            }
            _FillList(EpgEtcCustColors, 0xFFFFFFFF, num);

            //チューナー画面各色、保存名はServiceColorsだが他も含む。
            num = 2 + 5 + 8;//固定色2+優先度色5+ 追加の画面色
            if (TunerServiceColors.Count < num)
            {
                var defColors = Enumerable.Repeat("Black", 7)
                    .Concat(new string[]{
                        "DarkGray"          //チューナ画面背景色
                        ,"LightGray"        //予約枠色
                        ,"Black"            //時間文字色
                        ,"AliceBlue"        //時間背景色
                        ,"LightSlateGray"   //時間枠色
                        ,"Black"            //チューナー名文字色
                        ,"AliceBlue"        //チューナー名背景色
                        ,"LightSlateGray"   //チューナー名枠色
                    });
                _FillList(TunerServiceColors, defColors);
            }
            _FillList(TunerServiceCustColors, 0xFFFFFFFF, num);

            //録画済み一覧背景色
            num = 3;
            if (RecEndColors.Count < num)
            {
                var defColors = new string[]{
                        "White"         //デフォルト
                        ,"LightPink"    //エラー
                        ,"LightYellow"  //ワーニング
                    };
                _FillList(RecEndColors, defColors);
            }
            _FillList(RecEndCustColors, 0xFFFFFFFF, num);

            //録画モード別予約文字色
            num = 6;
            _FillList(RecModeFontColors, "カスタム", num);
            _FillList(RecModeFontCustColors, 0xFF042271, num);

            //状態別予約背景色
            num = 6;
            if (ResBackColors.Count < num)
            {
                var defColors = new string[]{
                        "White"         //通常
                        ,"LightGray"    //無効
                        ,"LightPink"    //チューナー不足
                        ,"LightYellow"  //一部実行
                        ,"LightBlue"    //自動予約登録不明
                        ,"Thistle"      //重複EPG予約
                    };
                _FillList(ResBackColors, defColors);
            }
            _FillList(ResBackCustColors, 0xFFFFFFFF, num);

            //予約状態列文字色
            num = 3;
            if (StatColors.Count < num)
            {
                var defColors = new List<string>{
                        "Blue"      //予約中
                        ,"OrangeRed"//録画中
                        ,"LimeGreen"//放送中
                    };
                _FillList(StatColors, defColors);
            }
            _FillList(StatCustColors, 0xFFFFFFFF, num);
        }
    }
}
