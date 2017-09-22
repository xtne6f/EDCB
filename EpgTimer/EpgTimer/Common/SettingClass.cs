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
                return Path.Combine(ModulePath, "Common.ini");
            }
        }
        public static string TimerSrvIniPath
        {
            get
            {
                return Path.Combine(ModulePath, "EpgTimerSrv.ini");
            }
        }
        public static void CheckFolderPath(ref string folderPath)
        {
            //過去にルートディレクトリ区切りを失った状態("C:"など)で設定などに保存していたので、これに対応する
            if (folderPath.Length > 0 &&
                folderPath[folderPath.Length - 1] != Path.DirectorySeparatorChar &&
                folderPath[folderPath.Length - 1] != Path.AltDirectorySeparatorChar)
            {
                //一時的に下層を作って上がる
                folderPath = Path.GetDirectoryName(folderPath + Path.DirectorySeparatorChar + "a") ?? folderPath;
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
        public bool ShowEpgCapServiceOnly { get; set; }
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
        public bool EpgGradation { get; set; }
        public bool EpgGradationHeader { get; set; }
        public double ResColumnWidth0 { get; set; }
        public double ResColumnWidth1 { get; set; }
        public double ResColumnWidth2 { get; set; }
        public double ResColumnWidth3 { get; set; }
        public double ResColumnWidth4 { get; set; }
        public double ResColumnWidth5 { get; set; }
        public double ResColumnWidth6 { get; set; }
        public double ResColumnWidth7 { get; set; }
        public string ResColumnHead { get; set; }
        public ListSortDirection ResSortDirection { get; set; }
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
        public double RecInfoColumnWidth0 { get; set; }
        public double RecInfoColumnWidth1 { get; set; }
        public double RecInfoColumnWidth2 { get; set; }
        public double RecInfoColumnWidth3 { get; set; }
        public double RecInfoColumnWidth4 { get; set; }
        public double RecInfoColumnWidth5 { get; set; }
        public double RecInfoColumnWidth6 { get; set; }
        public string RecInfoColumnHead { get; set; }
        public ListSortDirection RecInfoSortDirection { get; set; }
        public string TvTestExe { get; set; }
        public string TvTestCmd { get; set; }
        public bool NwTvMode { get; set; }
        public bool NwTvModeUDP { get; set; }
        public bool NwTvModeTCP { get; set; }
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
        public double SearchWndLeft { get; set; }
        public double SearchWndTop { get; set; }
        public double SearchWndWidth { get; set; }
        public double SearchWndHeight { get; set; }
        public int NotifyLogMax { get; set; }
        public bool ShowTray { get; set; }
        public bool MinHide { get; set; }
        public bool MouseScrollAuto { get; set; }
        public int NoStyle { get; set; }

        public Settings()
        {
            UseCustomEpgView = false;
            CustomEpgTabList = new List<CustomEpgTabInfo>();
            MinHeight = 2;
            MinimumHeight = 0;
            ServiceWidth = 150;
            ScrollSize = 240;
            FontName = "メイリオ";
            FontSize = 12;
            FontNameTitle = "メイリオ";
            FontSizeTitle = 12;
            FontBoldTitle = true;
            NoToolTip = false;
            PlayDClick = false;
            ShowEpgCapServiceOnly = false;
            DragScroll = 1.5;
            ContentColorList = new List<string>();
            ContentCustColorList = new List<uint>();
            TimeColorList = new List<string>();
            TimeCustColorList = new List<uint>();
            ReserveRectColorNormal = "Lime";
            ReserveRectColorNo = "Black";
            ReserveRectColorNoTuner = "Red";
            ReserveRectColorWarning = "Yellow";
            TitleColor1 = "Black";
            TitleColor2 = "Black";
            TitleCustColor1 = 0xFFFFFFFF;
            TitleCustColor2 = 0xFFFFFFFF;
            ServiceColor = "LightSlateGray";
            ServiceCustColor = 0xFFFFFFFF;
            ReserveRectFillOpacity = 0;
            ReserveRectFillWithShadow = true;
            EpgToolTip = false;
            EpgBorderLeftSize = 2;
            EpgBorderTopSize = 0.5;
            EpgTitleIndent = true;
            EpgReplacePattern = "";
            EpgReplacePatternTitle = "";
            EpgToolTipNoViewOnly = true;
            EpgToolTipViewWait = 1500;
            EpgPopup = true;
            EpgGradation = true;
            EpgGradationHeader = true;
            ResColumnHead = "";
            ResSortDirection = ListSortDirection.Ascending;
            LastWindowState = System.Windows.WindowState.Normal;
            MainWndLeft = -100;
            MainWndTop = -100;
            MainWndWidth = -100;
            MainWndHeight = -100;
            SearchWndTabsHeight = 0;
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
            AndKeyList = new List<string>();
            NotKeyList = new List<string>();
            SearchKeyRegExp = false;
            SearchKeyTitleOnly = false;
            SearchKeyAimaiFlag = false;
            SearchKeyNotContent = false;
            SearchKeyNotDate = false;
            SearchKeyFreeCA = 0;
            SearchKeyChkRecEnd = 0;
            SearchKeyChkRecDay = 6;
            SearchKeyContentList = new List<ContentKindInfo>();
            SearchKeyDateItemList = new List<DateItem>();
            SearchKeyServiceList = new List<Int64>();
            RecInfoColumnHead = "";
            RecInfoSortDirection = ListSortDirection.Ascending;
            TvTestExe = "";
            TvTestCmd = "";
            NwTvMode = false;
            NwTvModeUDP = false;
            NwTvModeTCP = false;
            FilePlayExe = "";
            FilePlayCmd = "\"$FilePath$\"";
            FilePlayOnAirWithExe = false;
            IEpgStationList = new List<IEPGStationInfo>();
            NWServerIP = "";
            NWServerPort = 4510;
            NWWaitPort = 0;
            NWMacAdd = "";
            WakeReconnectNW = false;
            SuspendCloseNW = false;
            NgAutoEpgLoadNW = false;
            TvTestOpenWait = 2000;
            TvTestChgBonWait = 2000;
            ResDefColorA = 0;
            ResDefColorR = 0xFF;
            ResDefColorG = 0xFF;
            ResDefColorB = 0xFF;
            ResErrColorA = 0x80;
            ResErrColorR = 0xFF;
            ResErrColorG = 0;
            ResErrColorB = 0;
            ResWarColorA = 0x80;
            ResWarColorR = 0xFF;
            ResWarColorG = 0xFF;
            ResWarColorB = 0;
            ResNoColorA = 0x80;
            ResNoColorR = 0xA9;
            ResNoColorG = 0xA9;
            ResNoColorB = 0xA9;
            RecEndDefColorA = 0;
            RecEndDefColorR = 0xFF;
            RecEndDefColorG = 0xFF;
            RecEndDefColorB = 0xFF;
            RecEndErrColorA = 0x80;
            RecEndErrColorR = 0xFF;
            RecEndErrColorG = 0;
            RecEndErrColorB = 0;
            RecEndWarColorA = 0x80;
            RecEndWarColorR = 0xFF;
            RecEndWarColorG = 0xFF;
            RecEndWarColorB = 0;
            EpgTipsBackColorR = 0xD3;
            EpgTipsBackColorG = 0xD3;
            EpgTipsBackColorB = 0xD3;
            EpgTipsForeColorR = 0;
            EpgTipsForeColorG = 0;
            EpgTipsForeColorB = 0;
            EpgBackColorR = 0xA9;
            EpgBackColorG = 0xA9;
            EpgBackColorB = 0xA9;
            EpgInfoSingleClick = false;
            EpgInfoOpenMode = 0;
            ExecBat = 0;
            SuspendChk = 0;
            ReserveListColumn = new List<ListColumnInfo>();
            RecInfoListColumn = new List<ListColumnInfo>();
            AutoAddEpgColumn = new List<ListColumnInfo>();
            AutoAddManualColumn = new List<ListColumnInfo>();
            SearchWndLeft = 0;
            SearchWndTop = 0;
            SearchWndWidth = 0;
            SearchWndHeight = 0;
            NotifyLogMax = 100;
            ShowTray = true;
            MinHide = true;
            MouseScrollAuto = false;
            NoStyle = 0;
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
                if (Instance.ContentColorList.Count == 0x10)//多分旧バージョンの互換用コード
                {
                    Instance.ContentColorList.Add("White");
                }
                else if (Instance.ContentColorList.Count != 0x11)
                {
                    //番組表のデフォルトの背景色
                    Instance.ContentColorList.Clear();
                    Instance.ContentColorList.Add("LightYellow");
                    Instance.ContentColorList.Add("Lavender");
                    Instance.ContentColorList.Add("LavenderBlush");
                    Instance.ContentColorList.Add("MistyRose");
                    Instance.ContentColorList.Add("Honeydew");
                    Instance.ContentColorList.Add("LightCyan");
                    Instance.ContentColorList.Add("PapayaWhip");
                    Instance.ContentColorList.Add("Pink");
                    Instance.ContentColorList.Add("LightYellow");
                    Instance.ContentColorList.Add("PapayaWhip");
                    Instance.ContentColorList.Add("AliceBlue");
                    Instance.ContentColorList.Add("AliceBlue");
                    Instance.ContentColorList.Add("White");
                    Instance.ContentColorList.Add("White");
                    Instance.ContentColorList.Add("White");
                    Instance.ContentColorList.Add("WhiteSmoke");
                    Instance.ContentColorList.Add("White");
                }
                if (Instance.ContentCustColorList.Count != 0x11 + 4)
                {
                    Instance.ContentCustColorList.Clear();
                    for (int i = 0; i < 0x11+4; i++)
                    {
                        Instance.ContentCustColorList.Add(0xFFFFFFFF);
                    }
                }
                if (Instance.TimeColorList.Count != 4)
                {
                    //番組表の時間軸のデフォルトの背景色
                    Instance.TimeColorList.Clear();
                    Instance.TimeColorList.Add("MediumPurple");
                    Instance.TimeColorList.Add("LightSeaGreen");
                    Instance.TimeColorList.Add("LightSalmon");
                    Instance.TimeColorList.Add("CornflowerBlue");
                }
                if (Instance.TimeCustColorList.Count != 4)
                {
                    Instance.TimeCustColorList.Clear();
                    for (int i = 0; i < 4; i++)
                    {
                        Instance.TimeCustColorList.Add(0xFFFFFFFF);
                    }
                }
                if (Instance.ViewButtonList.Count == 0)
                {
                    if (nwMode == false)
                    {
                        Instance.ViewButtonList.Add("設定");
                        Instance.ViewButtonList.Add("（空白）");
                        Instance.ViewButtonList.Add("検索");
                        Instance.ViewButtonList.Add("（空白）");
                        Instance.ViewButtonList.Add("スタンバイ");
                        Instance.ViewButtonList.Add("休止");
                        Instance.ViewButtonList.Add("（空白）");
                        Instance.ViewButtonList.Add("EPG取得");
                        Instance.ViewButtonList.Add("（空白）");
                        Instance.ViewButtonList.Add("EPG再読み込み");
                        Instance.ViewButtonList.Add("（空白）");
                        Instance.ViewButtonList.Add("終了");
                    }
                    else
                    {
                        Instance.ViewButtonList.Add("設定");
                        Instance.ViewButtonList.Add("（空白）");
                        Instance.ViewButtonList.Add("再接続");
                        Instance.ViewButtonList.Add("（空白）");
                        Instance.ViewButtonList.Add("検索");
                        Instance.ViewButtonList.Add("（空白）");
                        Instance.ViewButtonList.Add("EPG取得");
                        Instance.ViewButtonList.Add("（空白）");
                        Instance.ViewButtonList.Add("EPG再読み込み");
                        Instance.ViewButtonList.Add("（空白）");
                        Instance.ViewButtonList.Add("終了");
                    }
                }
                if (Instance.TaskMenuList.Count == 0)
                {
                    if (nwMode == false)
                    {
                        Instance.TaskMenuList.Add("設定");
                        Instance.TaskMenuList.Add("（セパレータ）");
                        Instance.TaskMenuList.Add("スタンバイ");
                        Instance.TaskMenuList.Add("休止");
                        Instance.TaskMenuList.Add("（セパレータ）");
                        Instance.TaskMenuList.Add("終了");
                    }
                    else
                    {
                        Instance.TaskMenuList.Add("設定");
                        Instance.TaskMenuList.Add("（セパレータ）");
                        Instance.TaskMenuList.Add("再接続");
                        Instance.TaskMenuList.Add("（セパレータ）");
                        Instance.TaskMenuList.Add("終了");
                    }
                }
                if (Instance.ReserveListColumn.Count == 0)
                {
                    Instance.ReserveListColumn.Add(new ListColumnInfo("StartTime", double.NaN));
                    Instance.ReserveListColumn.Add(new ListColumnInfo("NetworkName", double.NaN));
                    Instance.ReserveListColumn.Add(new ListColumnInfo("ServiceName", double.NaN));
                    Instance.ReserveListColumn.Add(new ListColumnInfo("EventName", double.NaN));
                    Instance.ReserveListColumn.Add(new ListColumnInfo("RecMode", double.NaN));
                    Instance.ReserveListColumn.Add(new ListColumnInfo("Priority", double.NaN));
                    Instance.ReserveListColumn.Add(new ListColumnInfo("Tuijyu", double.NaN));
                    Instance.ReserveListColumn.Add(new ListColumnInfo("Comment", double.NaN));
                    Instance.ReserveListColumn.Add(new ListColumnInfo("RecFileName", double.NaN));
                }
                if (Instance.RecInfoListColumn.Count == 0)
                {
                    Instance.RecInfoListColumn.Add(new ListColumnInfo("IsProtect", double.NaN));
                    Instance.RecInfoListColumn.Add(new ListColumnInfo("StartTime", double.NaN));
                    Instance.RecInfoListColumn.Add(new ListColumnInfo("NetworkName", double.NaN));
                    Instance.RecInfoListColumn.Add(new ListColumnInfo("ServiceName", double.NaN));
                    Instance.RecInfoListColumn.Add(new ListColumnInfo("EventName", double.NaN));
                    Instance.RecInfoListColumn.Add(new ListColumnInfo("Drops", double.NaN));
                    Instance.RecInfoListColumn.Add(new ListColumnInfo("Scrambles", double.NaN));
                    Instance.RecInfoListColumn.Add(new ListColumnInfo("Result", double.NaN));
                    Instance.RecInfoListColumn.Add(new ListColumnInfo("RecFilePath", double.NaN));
                }
                if (Instance.AutoAddEpgColumn.Count == 0)
                {
                    Instance.AutoAddEpgColumn.Add(new ListColumnInfo("AndKey", double.NaN));
                    Instance.AutoAddEpgColumn.Add(new ListColumnInfo("NotKey", double.NaN));
                    Instance.AutoAddEpgColumn.Add(new ListColumnInfo("RegExp", double.NaN));
                    Instance.AutoAddEpgColumn.Add(new ListColumnInfo("RecMode", double.NaN));
                    Instance.AutoAddEpgColumn.Add(new ListColumnInfo("Priority", double.NaN));
                    Instance.AutoAddEpgColumn.Add(new ListColumnInfo("Tuijyu", double.NaN));
                }
                if (Instance.AutoAddManualColumn.Count == 0)
                {
                    Instance.AutoAddManualColumn.Add(new ListColumnInfo("DayOfWeek", double.NaN));
                    Instance.AutoAddManualColumn.Add(new ListColumnInfo("Time", double.NaN));
                    Instance.AutoAddManualColumn.Add(new ListColumnInfo("Title", double.NaN));
                    Instance.AutoAddManualColumn.Add(new ListColumnInfo("StationName", double.NaN));
                    Instance.AutoAddManualColumn.Add(new ListColumnInfo("RecMode", double.NaN));
                    Instance.AutoAddManualColumn.Add(new ListColumnInfo("Priority", double.NaN));
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
