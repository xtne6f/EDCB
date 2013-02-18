using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;
using System.Net;

using System.Threading; //紅
using System.Windows.Interop; //紅
using System.Runtime.InteropServices; //紅

namespace EpgTimer
{
    /// <summary>
    /// MainWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class MainWindow : Window
    {
        private System.Threading.Mutex mutex;

        private TaskTrayClass taskTray = null;
        private bool serviceMode = false;
        private Dictionary<string, Button> buttonList = new Dictionary<string, Button>();
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;

        private PipeServer pipeServer = null;
        private string pipeName = "\\\\.\\pipe\\EpgTimerGUI_Ctrl_BonPipe_";
        private string pipeEventName = "Global\\EpgTimerGUI_Ctrl_BonConnect_";

        private bool closeFlag = false;
        private bool initExe = false;

        private bool needUnRegist = true;
        private bool iniConnectNW = false;

        public MainWindow()
        {
            CommonManager.Instance.NWMode = true;
            Settings.LoadFromXmlFileNW();
            ChSet5.LoadFile();
            cmd.SetSendMode(true);
            cmd.SetNWSetting(Settings.Instance.NWServerIP, Settings.Instance.NWServerPort);
            CommonManager.Instance.ReloadCustContentColorList();
            CommonManager.Instance.DB.SetNoAutoReloadEPG(Settings.Instance.NgAutoEpgLoadNW);

            if (Settings.Instance.NoStyle == 0)
            {
                ResourceDictionary rd = new ResourceDictionary();
                rd.MergedDictionaries.Add(
                    Application.LoadComponent(new Uri("/PresentationFramework.Aero, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/aero.normalcolor.xaml", UriKind.Relative)) as ResourceDictionary
                    //Application.LoadComponent(new Uri("/PresentationFramework.Classic, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35, ProcessorArchitecture=MSIL;component/themes/Classic.xaml", UriKind.Relative)) as ResourceDictionary
                    );
                this.Resources = rd;
            }

            mutex = new System.Threading.Mutex(false, "Global\\EpgTimer_BonNW");
            if (!mutex.WaitOne(0, false))
            {
                CheckCmdLine();

                mutex.Close();
                mutex = null;

                closeFlag = true;
                Close();
                return;
            }

            if (CommonManager.Instance.NWMode == false)
            {
                bool startExe = false;
                try
                {
                    if (ServiceCtrlClass.ServiceIsInstalled("EpgTimer Service") == true)
                    {
                        if (ServiceCtrlClass.IsStarted("EpgTimer Service") == false)
                        {
                            bool check = false;
                            for (int i = 0; i < 5; i++)
                            {
                                if (ServiceCtrlClass.StartService("EpgTimer Service") == true)
                                {
                                    check = true;
                                }
                                System.Threading.Thread.Sleep(1000);
                                if (ServiceCtrlClass.IsStarted("EpgTimer Service") == true)
                                {
                                    check = true;
                                }
                            }
                            if (check == false)
                            {
                                MessageBox.Show("サービスの開始に失敗しました。\r\nVista以降のOSでは、管理者権限で起動されている必要があります。");
                                closeFlag = true;
                                Close();
                                return;
                            }
                            else
                            {
                                serviceMode = true;
                                startExe = true;
                            }
                        }
                        else
                        {
                            serviceMode = true;
                            startExe = true;
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                    serviceMode = false;
                }
                try
                {
                    if (serviceMode == false)
                    {
                        String moduleFolder = System.IO.Path.GetDirectoryName(Environment.GetCommandLineArgs()[0]);
                        String exePath = moduleFolder + "\\EpgTimerSrv.exe";
                        System.Diagnostics.Process process = System.Diagnostics.Process.Start(exePath);
                        startExe = true;
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                    startExe = false;
                }

                if (startExe == false)
                {
                    MessageBox.Show("EpgTimerSrv.exeの起動ができませんでした");
                    closeFlag = true;
                    Close();
                    return;
                }
            }

            InitializeComponent();

            initExe = true;

            try
            {
                if (Settings.Instance.WakeMin == true)
                {
                    this.Visibility = System.Windows.Visibility.Hidden;
                }

                //ウインドウ位置の復元
                if (Settings.Instance.MainWndTop != -100)
                {
                    this.Top = Settings.Instance.MainWndTop;
                }
                if (Settings.Instance.MainWndLeft != -100)
                {
                    this.Left = Settings.Instance.MainWndLeft;
                }
                if (Settings.Instance.MainWndWidth != -100)
                {
                    this.Width = Settings.Instance.MainWndWidth;
                }
                if (Settings.Instance.MainWndHeight != -100)
                {
                    this.Height = Settings.Instance.MainWndHeight;
                }
                this.WindowState = Settings.Instance.LastWindowState;


                //上のボタン
                Button settingButton = new Button();
                settingButton.MinWidth = 75;
                settingButton.Margin = new Thickness(2, 2, 2, 5);
                settingButton.Click += new RoutedEventHandler(settingButton_Click);
                settingButton.Content = "設定";
                if (Settings.Instance.NoStyle == 0)
                {
                    settingButton.Style = (Style)App.Current.Resources["ButtonStyle1"];
                }
                buttonList.Add("設定", settingButton);

                Button searchButton = new Button();
                searchButton.MinWidth = 75;
                searchButton.Margin = new Thickness(2, 2, 2, 5);
                searchButton.Click += new RoutedEventHandler(searchButton_Click);
                searchButton.Content = "検索";
                if (Settings.Instance.NoStyle == 0)
                {
                    searchButton.Style = (Style)App.Current.Resources["ButtonStyle1"];
                }
                buttonList.Add("検索", searchButton);

                Button closeButton = new Button();
                closeButton.MinWidth = 75;
                closeButton.Margin = new Thickness(2, 2, 2, 5);
                closeButton.Click += new RoutedEventHandler(closeButton_Click);
                closeButton.Content = "終了";
                if (Settings.Instance.NoStyle == 0)
                {
                    closeButton.Style = (Style)App.Current.Resources["ButtonStyle1"];
                }
                buttonList.Add("終了", closeButton);

                Button stanbyButton = new Button();
                stanbyButton.MinWidth = 75;
                stanbyButton.Margin = new Thickness(2, 2, 2, 5);
                stanbyButton.Click += new RoutedEventHandler(standbyButton_Click);
                stanbyButton.Content = "スタンバイ";
                if (Settings.Instance.NoStyle == 0)
                {
                    stanbyButton.Style = (Style)App.Current.Resources["ButtonStyle1"];
                }
                buttonList.Add("スタンバイ", stanbyButton);

                Button suspendButton = new Button();
                suspendButton.MinWidth = 75;
                suspendButton.Margin = new Thickness(2, 2, 2, 5);
                suspendButton.Click += new RoutedEventHandler(suspendButton_Click);
                suspendButton.Content = "休止";
                if (Settings.Instance.NoStyle == 0)
                {
                    suspendButton.Style = (Style)App.Current.Resources["ButtonStyle1"];
                }
                buttonList.Add("休止", suspendButton);

                Button epgCapButton = new Button();
                epgCapButton.MinWidth = 75;
                epgCapButton.Margin = new Thickness(2, 2, 2, 5);
                epgCapButton.Click += new RoutedEventHandler(epgCapButton_Click);
                epgCapButton.Content = "EPG取得";
                if (Settings.Instance.NoStyle == 0)
                {
                    epgCapButton.Style = (Style)App.Current.Resources["ButtonStyle1"];
                }
                buttonList.Add("EPG取得", epgCapButton);

                Button epgReloadButton = new Button();
                epgReloadButton.MinWidth = 75;
                epgReloadButton.Margin = new Thickness(2, 2, 2, 5);
                epgReloadButton.Click += new RoutedEventHandler(epgReloadButton_Click);
                epgReloadButton.Content = "EPG再読み込み";
                if (Settings.Instance.NoStyle == 0)
                {
                    epgReloadButton.Style = (Style)App.Current.Resources["ButtonStyle1"];
                }
                buttonList.Add("EPG再読み込み", epgReloadButton);

                Button custum1Button = new Button();
                custum1Button.MinWidth = 75;
                custum1Button.Margin = new Thickness(2, 2, 2, 5);
                custum1Button.Click += new RoutedEventHandler(custum1Button_Click);
                custum1Button.Content = "カスタム１";
                if (Settings.Instance.NoStyle == 0)
                {
                    custum1Button.Style = (Style)App.Current.Resources["ButtonStyle1"];
                }
                buttonList.Add("カスタム１", custum1Button);

                Button custum2Button = new Button();
                custum2Button.MinWidth = 75;
                custum2Button.Margin = new Thickness(2, 2, 2, 5);
                custum2Button.Click += new RoutedEventHandler(custum2Button_Click);
                custum2Button.Content = "カスタム２";
                if (Settings.Instance.NoStyle == 0)
                {
                    custum2Button.Style = (Style)App.Current.Resources["ButtonStyle1"];
                }
                buttonList.Add("カスタム２", custum2Button);

                Button nwTVEndButton = new Button();
                nwTVEndButton.MinWidth = 75;
                nwTVEndButton.Margin = new Thickness(2, 2, 2, 5);
                nwTVEndButton.Click += new RoutedEventHandler(nwTVEndButton_Click);
                nwTVEndButton.Content = "NetworkTV終了";
                if (Settings.Instance.NoStyle == 0)
                {
                    nwTVEndButton.Style = (Style)App.Current.Resources["ButtonStyle1"];
                }
                buttonList.Add("NetworkTV終了", nwTVEndButton);

                Button logViewButton = new Button();
                logViewButton.MinWidth = 75;
                logViewButton.Margin = new Thickness(2, 2, 2, 5);
                logViewButton.Click += new RoutedEventHandler(logViewButton_Click);
                logViewButton.Content = "情報通知ログ";
                if (Settings.Instance.NoStyle == 0)
                {
                    logViewButton.Style = (Style)App.Current.Resources["ButtonStyle1"];
                }
                buttonList.Add("情報通知ログ", logViewButton);

                Button connectButton = new Button();
                connectButton.MinWidth = 75;
                connectButton.Margin = new Thickness(2, 2, 2, 5);
                connectButton.Click += new RoutedEventHandler(connectButton_Click);
                connectButton.Content = "再接続";
                if (Settings.Instance.NoStyle == 0)
                {
                    connectButton.Style = (Style)App.Current.Resources["ButtonStyle1"];
                }
                buttonList.Add("再接続", connectButton);

                ResetButtonView();

                //タスクトレイの表示
                taskTray = new TaskTrayClass(this);
                taskTray.Icon = Properties.Resources.TaskIconBlue;
                taskTray.Visible = true;
                taskTray.ContextMenuClick += new EventHandler(taskTray_ContextMenuClick);

                if (CommonManager.Instance.NWMode == false)
                {
                    pipeServer = new PipeServer();
                    pipeName += System.Diagnostics.Process.GetCurrentProcess().Id.ToString();
                    pipeEventName += System.Diagnostics.Process.GetCurrentProcess().Id.ToString();
                    pipeServer.StartServer(pipeEventName, pipeName, OutsideCmdCallback, this);

                    cmd.SendRegistGUI((uint)System.Diagnostics.Process.GetCurrentProcess().Id);

                    CommonManager.Instance.DB.ReloadReserveInfo();
                    ReserveData item = new ReserveData();
                    if (CommonManager.Instance.DB.GetNextReserve(ref item) == true)
                    {
                        String timeView = item.StartTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
                        DateTime endTime = item.StartTime + TimeSpan.FromSeconds(item.DurationSecond);
                        timeView += endTime.ToString("HH:mm:ss");
                        taskTray.Text = "次の予約：" + item.StationName + " " + timeView + " " + item.Title;
                    }
                    else
                    {
                        taskTray.Text = "次の予約なし";
                    }
                }

                ResetTaskMenu();

                CheckCmdLine();

                if (CommonManager.Instance.NWMode == true)
                {
                    if (Settings.Instance.WakeReconnectNW == false)
                    {
                        return;
                    }

                    if (CommonManager.Instance.NW.ConnectServer(Settings.Instance.NWServerIP, Settings.Instance.NWServerPort, Settings.Instance.NWWaitPort, OutsideCmdCallback, this) == false)
                    {
                        return;
                    }
                    iniConnectNW = true;

                    byte[] binData;
                    if (cmd.SendFileCopy("ChSet5.txt", out binData) == 1)
                    {
                        string filePath = SettingPath.SettingFolderPath;
                        System.IO.Directory.CreateDirectory(filePath);
                        filePath += "\\ChSet5.txt";
                        using (System.IO.BinaryWriter w = new System.IO.BinaryWriter(System.IO.File.Create(filePath)))
                        {
                            w.Write(binData);
                            w.Close();
                        }
                        ChSet5.LoadFile();
                    }

                    CommonManager.Instance.DB.ReloadReserveInfo();
                    ReserveData item = new ReserveData();
                    if (CommonManager.Instance.DB.GetNextReserve(ref item) == true)
                    {
                        String timeView = item.StartTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
                        DateTime endTime = item.StartTime + TimeSpan.FromSeconds(item.DurationSecond);
                        timeView += endTime.ToString("HH:mm:ss");
                        taskTray.Text = "次の予約：" + item.StationName + " " + timeView + " " + item.Title;
                    }
                    else
                    {
                        taskTray.Text = "次の予約なし";
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void CheckCmdLine()
        {
            foreach (string arg in Environment.GetCommandLineArgs())
            {
                String ext = System.IO.Path.GetExtension(arg);
                if (string.Compare(ext, ".exe", true) == 0)
                {
                    //何もしない
                }
                else if (string.Compare(ext, ".eaa", true) == 0)
                {
                    //自動予約登録条件追加
                    EAAFileClass eaaFile = new EAAFileClass();
                    if (eaaFile.LoadEAAFile(arg) == true)
                    {
                        List<CtrlCmdCLI.Def.EpgAutoAddData> val = new List<CtrlCmdCLI.Def.EpgAutoAddData>();
                        val.Add(eaaFile.AddKey);
                        cmd.SendAddEpgAutoAdd(val);
                    }
                    else
                    {
                        MessageBox.Show("解析に失敗しました。");
                    }
                }
                else if (string.Compare(ext, ".tvpid", true) == 0 || string.Compare(ext, ".tvpio", true) == 0)
                {
                    //iEPG追加
                    IEPGFileClass iepgFile = new IEPGFileClass();
                    if (iepgFile.LoadTVPIDFile(arg) == true)
                    {
                        List<CtrlCmdCLI.Def.ReserveData> val = new List<CtrlCmdCLI.Def.ReserveData>();
                        val.Add(iepgFile.AddInfo);
                        cmd.SendAddReserve(val);
                    }
                    else
                    {
                        MessageBox.Show("解析に失敗しました。デジタル用Version 2のiEPGの必要があります。");
                    }
                }
                else if (string.Compare(ext, ".tvpi", true) == 0)
                {
                    //iEPG追加
                    IEPGFileClass iepgFile = new IEPGFileClass();
                    if (iepgFile.LoadTVPIFile(arg) == true)
                    {
                        List<CtrlCmdCLI.Def.ReserveData> val = new List<CtrlCmdCLI.Def.ReserveData>();
                        val.Add(iepgFile.AddInfo);
                        cmd.SendAddReserve(val);
                    }
                    else
                    {
                        MessageBox.Show("解析に失敗しました。放送局名がサービスに関連づけされていない可能性があります。");
                    }
                }
            }
        }
        void taskTray_ContextMenuClick(object sender, EventArgs e)
        {
            String tag = sender.ToString();
            if (String.Compare("設定", tag) == 0)
            {
                SettingCmd();
            }
            else if (String.Compare("終了", tag) == 0)
            {
                CloseCmd();
            }
            else if (String.Compare("スタンバイ", tag) == 0)
            {
                StandbyCmd();
            }
            else if (String.Compare("休止", tag) == 0)
            {
                SuspendCmd();
            }
            else if (String.Compare("EPG取得", tag) == 0)
            {
                EpgCapCmd();
            }
        }

        private void ResetTaskMenu()
        {
            List<Object> addList = new List<object>();
            foreach (String info in Settings.Instance.TaskMenuList)
            {
                if (String.Compare(info, "（セパレータ）") == 0)
                {
                    addList.Add("");
                }
                else
                {
                    addList.Add(info);
                }
            }
            taskTray.SetContextMenu(addList);
        }


        private void ResetButtonView()
        {
            stackPanel_button.Children.Clear();
            foreach (string info in Settings.Instance.ViewButtonList)
            {
                if (String.Compare(info, "（空白）") == 0)
                {
                    Label space = new Label();
                    space.Width = 15;
                    stackPanel_button.Children.Add(space);
                }
                else
                {
                    if (buttonList.ContainsKey(info) == true)
                    {
                        if (String.Compare(info, "カスタム１") == 0)
                        {
                            buttonList[info].Content = Settings.Instance.Cust1BtnName;
                        }
                        if (String.Compare(info, "カスタム２") == 0)
                        {
                            buttonList[info].Content = Settings.Instance.Cust2BtnName;
                        }
                        stackPanel_button.Children.Add(buttonList[info]);
                    }
                }
            }
        }

        bool ConnectCmd(bool reloadFlag)
        {
            ConnectWindow dlg = new ConnectWindow();
            PresentationSource topWindow = PresentationSource.FromVisual(this);
            if (topWindow != null)
            {
                dlg.Owner = (Window)topWindow.RootVisual;
            }
            if (dlg.ShowDialog() == true)
            {
                bool connected = false;
                String srvIP = Settings.Instance.NWServerIP;
                try
                {
                    foreach (IPAddress address in Dns.GetHostAddresses(srvIP))
                    {
                        srvIP = address.ToString();
                        if (CommonManager.Instance.NW.ConnectServer(srvIP, Settings.Instance.NWServerPort, Settings.Instance.NWWaitPort, OutsideCmdCallback, this) == true)
                        {
                            connected = true;
                            break;
                        }
                    }
                }
                catch
                {
                }

                if (connected == false)
                {
                    MessageBox.Show("サーバーへの接続に失敗しました");
                }
                else
                {
                    if (reloadFlag == true)
                    {
                        byte[] binData;
                        if (cmd.SendFileCopy("ChSet5.txt", out binData) == 1)
                        {
                            string filePath = SettingPath.SettingFolderPath;
                            System.IO.Directory.CreateDirectory(filePath);
                            filePath += "\\ChSet5.txt";
                            using (System.IO.BinaryWriter w = new System.IO.BinaryWriter(System.IO.File.Create(filePath)))
                            {
                                w.Write(binData);
                                w.Close();
                            }
                            ChSet5.LoadFile();
                        }
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.RecInfo);
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddManualInfo);
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.EpgData);
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.PlugInFile);
                        reserveView.UpdateReserveData();
                        epgView.UpdateReserveData();
                        tunerReserveView.UpdateReserveData();
                        autoAddView.UpdateAutoAddInfo();
                        recInfoView.UpdateInfo();
                        epgView.UpdateEpgData();
                    }

                    return true;
                }
            }
            return false;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if (CommonManager.Instance.NWMode == true)
            {
                if (iniConnectNW == false)
                {
                    if (ConnectCmd(false) == true)
                    {
                        byte[] binData;
                        if (cmd.SendFileCopy("ChSet5.txt", out binData) == 1)
                        {
                            string filePath = SettingPath.SettingFolderPath;
                            System.IO.Directory.CreateDirectory(filePath);
                            filePath += "\\ChSet5.txt";
                            using (System.IO.BinaryWriter w = new System.IO.BinaryWriter(System.IO.File.Create(filePath)))
                            {
                                w.Write(binData);
                                w.Close();
                            }
                            ChSet5.LoadFile();
                        }
                        iniConnectNW = true;
                    }
                }
            }
            //
            foreach (KeyValuePair<string, Button> kvp1 in this.buttonList)
            {
                switch (kvp1.Key)
                {
                    case "設定":
                        kvp1.Value.ToolTip = "Ctrl + C";
                        break;
                    case "検索":
                        kvp1.Value.ToolTip = "Ctrl + F";
                        break;
                    case "終了":
                        kvp1.Value.ToolTip = "Alt + F4";
                        break;
                }
            }
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (Settings.Instance.CloseMin == true && closeFlag == false)
            {
                e.Cancel = true;
                WindowState = System.Windows.WindowState.Minimized;
            }
            else
            {
                if (CommonManager.Instance.NWMode == false)
                {
                    if (initExe == true)
                    {
                        reserveView.SaveSize();
                        recInfoView.SaveSize();

                        cmd.SetConnectTimeOut(3000);
                        cmd.SendUnRegistGUI((uint)System.Diagnostics.Process.GetCurrentProcess().Id);
                        Settings.SaveToXmlFile();
                    }
                    pipeServer.StopServer();

                    if (mutex != null)
                    {
                        if (serviceMode == false && initExe == true)
                        {
                            cmd.SendClose();
                        }
                        mutex.ReleaseMutex();
                        mutex.Close();
                    }
                }
                else
                {
                    reserveView.SaveSize();
                    recInfoView.SaveSize();
                    if (CommonManager.Instance.NW.IsConnected == true && needUnRegist == true)
                    {
                        if (cmd.SendUnRegistTCP(Settings.Instance.NWServerPort) == 205)
                        {
                            //MessageBox.Show("サーバーに接続できませんでした");
                        }
                    }
                    Settings.SaveToXmlFile();

                    if (mutex != null)
                    {
                        mutex.ReleaseMutex();
                        mutex.Close();
                    }
                }
            }
        }

        private void Window_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (this.WindowState == WindowState.Normal)
            {
                if (this.Visibility == System.Windows.Visibility.Visible && this.Width > 0 && this.Height > 0)
                {
                    Settings.Instance.MainWndWidth = this.Width;
                    Settings.Instance.MainWndHeight = this.Height;
                }
            }
        }

        private void Window_LocationChanged(object sender, EventArgs e)
        {
            if (this.WindowState == WindowState.Normal)
            {
                if (this.Visibility == System.Windows.Visibility.Visible && this.Top > 0 && this.Left > 0)
                {
                    Settings.Instance.MainWndTop = this.Top;
                    Settings.Instance.MainWndLeft = this.Left;
                }
            }
        }

        private void Window_StateChanged(object sender, EventArgs e)
        {
            if (this.WindowState == WindowState.Minimized)
            {
                this.Visibility = System.Windows.Visibility.Hidden;
            }
            if (this.WindowState == WindowState.Normal || this.WindowState == WindowState.Maximized)
            {
                this.Visibility = System.Windows.Visibility.Visible;
                taskTray.LastViewState = this.WindowState;
                Settings.Instance.LastWindowState = this.WindowState;
            }
        }

        private void Window_PreviewDragEnter(object sender, DragEventArgs e)
        {
            e.Handled = true;
        }

        private void Window_PreviewDrop(object sender, DragEventArgs e)
        {
            string[] filePath = e.Data.GetData(DataFormats.FileDrop, true) as string[];
            foreach (string path in filePath)
            {
                String ext = System.IO.Path.GetExtension(path);
                if (string.Compare(ext, ".eaa", true) == 0)
                {
                    //自動予約登録条件追加
                    EAAFileClass eaaFile = new EAAFileClass();
                    if (eaaFile.LoadEAAFile(path) == true)
                    {
                        List<CtrlCmdCLI.Def.EpgAutoAddData> val = new List<CtrlCmdCLI.Def.EpgAutoAddData>();
                        val.Add(eaaFile.AddKey);
                        cmd.SendAddEpgAutoAdd(val);
                    }
                    else
                    {
                        MessageBox.Show("解析に失敗しました。");
                    }
                }
                else if (string.Compare(ext, ".tvpid", true) == 0 || string.Compare(ext, ".tvpio", true) == 0)
                {
                    //iEPG追加
                    IEPGFileClass iepgFile = new IEPGFileClass();
                    if (iepgFile.LoadTVPIDFile(path) == true)
                    {
                        List<CtrlCmdCLI.Def.ReserveData> val = new List<CtrlCmdCLI.Def.ReserveData>();
                        val.Add(iepgFile.AddInfo);
                        cmd.SendAddReserve(val);
                    }
                    else
                    {
                        MessageBox.Show("解析に失敗しました。デジタル用Version 2のiEPGの必要があります。");
                    }
                }
                else if (string.Compare(ext, ".tvpi", true) == 0)
                {
                    //iEPG追加
                    IEPGFileClass iepgFile = new IEPGFileClass();
                    if (iepgFile.LoadTVPIFile(path) == true)
                    {
                        List<CtrlCmdCLI.Def.ReserveData> val = new List<CtrlCmdCLI.Def.ReserveData>();
                        val.Add(iepgFile.AddInfo);
                        cmd.SendAddReserve(val);
                    }
                    else
                    {
                        MessageBox.Show("解析に失敗しました。放送局名がサービスに関連づけされていない可能性があります。");
                    }
                }
            }
        }

        void settingButton_Click(object sender, RoutedEventArgs e)
        {
            SettingCmd();
        }

        void SettingCmd()
        {
            SettingWindow setting = new SettingWindow();
            PresentationSource topWindow = PresentationSource.FromVisual(this);
            if (topWindow != null)
            {
                setting.Owner = (Window)topWindow.RootVisual;
            }
            if (setting.ShowDialog() == true)
            {
                if (setting.ServiceStop == false)
                {
                    CommonManager.Instance.DB.SetNoAutoReloadEPG(Settings.Instance.NgAutoEpgLoadNW);

                    epgView.UpdateSetting();
                    cmd.SendReloadSetting();
                    ResetButtonView();
                    ResetTaskMenu();
                }
            }
            if (setting.ServiceStop == true)
            {
                MessageBox.Show("サービスの状態を変更したため終了します。");
                initExe = false;
                closeFlag = true;
                Close();
                return;
            }
            ChSet5.LoadFile();
        }

        void searchButton_Click(object sender, RoutedEventArgs e)
        {
            // Hide()したSearchWindowを復帰
            foreach (Window win1 in this.OwnedWindows)
            {
                if (win1.GetType() == typeof(SearchWindow))
                {
                    win1.Show();
                    return;
                }
            }
            //
            SearchCmd();
        }

        void SearchCmd()
        {
            SearchWindow search = new SearchWindow();
            PresentationSource topWindow = PresentationSource.FromVisual(this);
            if (topWindow != null)
            {
                search.Owner = (Window)topWindow.RootVisual;
            }
            search.SetViewMode(0);
            search.ShowDialog();
        }

        void closeButton_Click(object sender, RoutedEventArgs e)
        {
            CloseCmd();
        }

        void CloseCmd()
        {
            closeFlag = true;
            Close();
        }

        void epgCapButton_Click(object sender, RoutedEventArgs e)
        {
            EpgCapCmd();
        }

        void EpgCapCmd()
        {
            if (cmd.SendEpgCapNow() != 1)
            {
                MessageBox.Show("EPG取得を行える状態ではありません。\r\n（もうすぐ予約が始まる。EPGデータ読み込み中。など）");
            }
        }

        void epgReloadButton_Click(object sender, RoutedEventArgs e)
        {
            EpgReloadCmd();
        }

        void EpgReloadCmd()
        {
            if (CommonManager.Instance.NWMode == true)
            {
                CommonManager.Instance.DB.SetOneTimeReloadEpg();
            }
            if (cmd.SendReloadEpg() != 1)
            {
                MessageBox.Show("EPG再読み込みを行える状態ではありません。\r\n（EPGデータ読み込み中。など）");
            }
        }

        void suspendButton_Click(object sender, RoutedEventArgs e)
        {
            SuspendCmd();
        }

        void SuspendCmd()
        {
            UInt32 err = cmd.SendChkSuspend();
            if (err == 205)
            {
                MessageBox.Show("サーバーに接続できませんでした");
            }
            else if (err != 1)
            {
                MessageBox.Show("休止に移行できる状態ではありません。\r\n（もうすぐ予約が始まる。または抑制条件のexeが起動している。など）");
            }
            else
            {
                if (CommonManager.Instance.NWMode == false)
                {
                    if (IniFileHandler.GetPrivateProfileInt("SET", "Reboot", 0, SettingPath.TimerSrvIniPath) == 1)
                    {
                        cmd.SendSuspend(0x0102);
                    }
                    else
                    {
                        cmd.SendSuspend(2);
                    }
                }
                else
                {
                    if (Settings.Instance.SuspendCloseNW == true)
                    {
                        if (CommonManager.Instance.NW.IsConnected == true)
                        {
                            if (cmd.SendUnRegistTCP(Settings.Instance.NWServerPort) == 205)
                            {

                            }
                            cmd.SendSuspend(0xFF02);
                            closeFlag = true;
                            needUnRegist = false;
                            Close();
                        }
                    }
                    else
                    {
                        cmd.SendSuspend(0xFF02);
                    }
                }
            }
        }

        void standbyButton_Click(object sender, RoutedEventArgs e)
        {
            StandbyCmd();
        }

        void StandbyCmd()
        {
            UInt32 err = cmd.SendChkSuspend();
            if (err == 205)
            {
                MessageBox.Show("サーバーに接続できませんでした");
            }
            else if (err != 1)
            {
                MessageBox.Show("スタンバイに移行できる状態ではありません。\r\n（もうすぐ予約が始まる。または抑制条件のexeが起動している。など）");
            }
            else
            {
                if (CommonManager.Instance.NWMode == false)
                {
                    if (IniFileHandler.GetPrivateProfileInt("SET", "Reboot", 0, SettingPath.TimerSrvIniPath) == 1)
                    {
                        cmd.SendSuspend(0x0101);
                    }
                    else
                    {
                        cmd.SendSuspend(1);
                    }
                }
                else
                {
                    if (Settings.Instance.SuspendCloseNW == true)
                    {
                        if (CommonManager.Instance.NW.IsConnected == true)
                        {
                            if (cmd.SendUnRegistTCP(Settings.Instance.NWServerPort) == 205)
                            {

                            }
                            cmd.SendSuspend(0xFF01);
                            closeFlag = true;
                            needUnRegist = false;
                            Close();
                        }
                    }
                    else
                    {
                        cmd.SendSuspend(0xFF01);
                    }
                }
            }
        }

        void custum1Button_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                System.Diagnostics.Process.Start(Settings.Instance.Cust1BtnCmd, Settings.Instance.Cust1BtnCmdOpt);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        void custum2Button_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                System.Diagnostics.Process.Start(Settings.Instance.Cust2BtnCmd, Settings.Instance.Cust2BtnCmdOpt);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        void nwTVEndButton_Click(object sender, RoutedEventArgs e)
        {
            CommonManager.Instance.TVTestCtrl.CloseTVTest();
        }

        void logViewButton_Click(object sender, RoutedEventArgs e)
        {
            NotifyLogWindow dlg = new NotifyLogWindow();
            PresentationSource topWindow = PresentationSource.FromVisual(this);
            if (topWindow != null)
            {
                dlg.Owner = (Window)topWindow.RootVisual;
            }
            dlg.ShowDialog();
        }

        void connectButton_Click(object sender, RoutedEventArgs e)
        {
            ConnectCmd(true);
        }

        private int OutsideCmdCallback(object pParam, CMD_STREAM pCmdParam, ref CMD_STREAM pResParam)
        {
            System.Diagnostics.Trace.WriteLine((CtrlCmd)pCmdParam.uiParam);
            switch ((CtrlCmd)pCmdParam.uiParam)
            {
                case CtrlCmd.CMD_TIMER_GUI_SHOW_DLG:
                    {
                        pResParam.uiParam = (uint)ErrCode.CMD_SUCCESS;
                        this.Visibility = System.Windows.Visibility.Visible;
                    }
                    break;
                case CtrlCmd.CMD_TIMER_GUI_UPDATE_RESERVE:
                    {
                        pResParam.uiParam = (uint)ErrCode.CMD_SUCCESS;
                        if (Dispatcher.CheckAccess() == true)
                        {
                            CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                            CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.RecInfo);
                            CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                            CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddManualInfo);
                            reserveView.UpdateReserveData();
                            epgView.UpdateReserveData();
                            tunerReserveView.UpdateReserveData();
                            autoAddView.UpdateAutoAddInfo();
                            recInfoView.UpdateInfo();

                            CommonManager.Instance.DB.ReloadReserveInfo();
                            ReserveData item = new ReserveData();
                            if (CommonManager.Instance.DB.GetNextReserve(ref item) == true)
                            {
                                String timeView = item.StartTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
                                DateTime endTime = item.StartTime + TimeSpan.FromSeconds(item.DurationSecond);
                                timeView += endTime.ToString("HH:mm:ss");
                                taskTray.Text = "次の予約：" + item.StationName + " " + timeView + " " + item.Title;
                            }
                            else
                            {
                                taskTray.Text = "次の予約なし";
                            }
                        }
                        else
                        {
                            Dispatcher.BeginInvoke(new Action(() =>
                            {
                                CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                                CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.RecInfo);
                                CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                                CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddManualInfo);
                                reserveView.UpdateReserveData();
                                epgView.UpdateReserveData();
                                tunerReserveView.UpdateReserveData();
                                autoAddView.UpdateAutoAddInfo();
                                recInfoView.UpdateInfo();

                                CommonManager.Instance.DB.ReloadReserveInfo();
                                ReserveData item = new ReserveData();
                                if (CommonManager.Instance.DB.GetNextReserve(ref item) == true)
                                {
                                    String timeView = item.StartTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
                                    DateTime endTime = item.StartTime + TimeSpan.FromSeconds(item.DurationSecond);
                                    timeView += endTime.ToString("HH:mm:ss");
                                    taskTray.Text = "次の予約：" + item.StationName + " " + timeView + " " + item.Title;
                                }
                                else
                                {
                                    taskTray.Text = "次の予約なし";
                                }

                            }));
                        }
                    }
                    break;
                case CtrlCmd.CMD_TIMER_GUI_UPDATE_EPGDATA:
                    {
                        pResParam.uiParam = (uint)ErrCode.CMD_SUCCESS;
                        if (Dispatcher.CheckAccess() == true)
                        {
                            CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.EpgData);
                            epgView.UpdateEpgData();
                        }
                        else
                        {
                            Dispatcher.BeginInvoke(new Action(() =>
                            {
                                CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.EpgData);
                                epgView.UpdateEpgData();
                            }));
                        }
                    }
                    break;
                case CtrlCmd.CMD_TIMER_GUI_VIEW_EXECUTE:
                    {
                        pResParam.uiParam = (uint)ErrCode.CMD_SUCCESS;
                        String exeCmd = "";
                        CmdStreamUtil.ReadStreamData(ref exeCmd, pCmdParam);
                        try
                        {
                            string[] cmd = exeCmd.Split('\"');
                            System.Diagnostics.Process process;
                            if (cmd.Length >= 3)
                            {
                                process = System.Diagnostics.Process.Start(cmd[1], cmd[2]);
                            }
                            else if (cmd.Length >= 2)
                            {
                                process = System.Diagnostics.Process.Start(cmd[1]);
                            }
                            else
                            {
                                process = System.Diagnostics.Process.Start(cmd[0]);
                            }
                            CmdStreamUtil.CreateStreamData(process.Id, ref pResParam);
                        }
                        catch
                        {
                        }
                    }
                    break;
                case CtrlCmd.CMD_TIMER_GUI_QUERY_SUSPEND:
                    {
                        pResParam.uiParam = (uint)ErrCode.CMD_SUCCESS;

                        UInt16 param = 0;
                        CmdStreamUtil.ReadStreamData(ref param, pCmdParam);

                        ///////////////////////////////////////////紅////////////////////////////////////
                        ParameterizedThreadStart ts = new ParameterizedThreadStart(SuspendThread);
                        Thread thread = new Thread(ts);
                        thread.Start(param);
                        /////////////////////////////////////////////////////////////////////////////////////

                        /*Byte reboot = (Byte)((param & 0xFF00) >> 8);
                        Byte suspendMode = (Byte)(param & 0x00FF);

                        Dispatcher.BeginInvoke(new Action(() =>
                        {
                            SuspendCheckWindow dlg = new SuspendCheckWindow();
                            dlg.SetMode(0, suspendMode);
                            if (dlg.ShowDialog() != true)
                            {
                                cmd.SendSuspend(param);
                            }
                        }));*/
                    }
                    break;
                case CtrlCmd.CMD_TIMER_GUI_QUERY_REBOOT:
                    {
                        pResParam.uiParam = (uint)ErrCode.CMD_SUCCESS;

                        UInt16 param = 0;
                        CmdStreamUtil.ReadStreamData(ref param, pCmdParam);

                        Byte reboot = (Byte)((param & 0xFF00) >> 8);
                        Byte suspendMode = (Byte)(param & 0x00FF);

                        Dispatcher.BeginInvoke(new Action(() =>
                        {
                            SuspendCheckWindow dlg = new SuspendCheckWindow();
                            dlg.SetMode(reboot, suspendMode);
                            if (dlg.ShowDialog() != true)
                            {
                                cmd.SendReboot();
                            }
                        }));
                    }
                    break;
                case CtrlCmd.CMD_TIMER_GUI_SRV_STATUS_CHG:
                    {
                        pResParam.uiParam = (uint)ErrCode.CMD_SUCCESS;
                        UInt16 status = 0;
                        CmdStreamUtil.ReadStreamData(ref status, pCmdParam);

                        if (Dispatcher.CheckAccess() == true)
                        {
                            if (status == 1)
                            {
                                taskTray.Icon = Properties.Resources.TaskIconRed;
                            }
                            else if (status == 2)
                            {
                                taskTray.Icon = Properties.Resources.TaskIconGreen;
                            }
                            else
                            {
                                taskTray.Icon = Properties.Resources.TaskIconBlue;
                            }
                        }
                        else
                        {
                            Dispatcher.BeginInvoke(new Action(() =>
                            {
                                if (status == 1)
                                {
                                    taskTray.Icon = Properties.Resources.TaskIconRed;
                                }
                                else if (status == 2)
                                {
                                    taskTray.Icon = Properties.Resources.TaskIconGreen;
                                }
                                else
                                {
                                    taskTray.Icon = Properties.Resources.TaskIconBlue;
                                }
                            }));
                        }
                    }
                    break;
                case CtrlCmd.CMD_TIMER_GUI_SRV_STATUS_NOTIFY2:
                    {
                        pResParam.uiParam = (uint)ErrCode.CMD_SUCCESS;

                        NotifySrvInfo status = new NotifySrvInfo();
                        CmdStreamUtil.ReadStreamData(ref status, pCmdParam);
                        if (Dispatcher.CheckAccess() == true)
                        {
                            NotifyStatus(status);
                        }
                        else
                        {
                            Dispatcher.BeginInvoke(new Action(() =>
                            {
                                NotifyStatus(status);
                            }));
                        }
                    }
                    break;
                default:
                    pResParam.uiParam = (uint)ErrCode.CMD_NON_SUPPORT;
                    break;
            }
            return 0;
        }

        internal struct LASTINPUTINFO
        {
            public uint cbSize;
            public uint dwTime;
        }

        [DllImport("User32.dll")]
        private static extern bool GetLastInputInfo(ref LASTINPUTINFO plii);

        [DllImport("Kernel32.dll")]
        public static extern UInt32 GetTickCount();

        private void SuspendThread(object obj)
        {
            LASTINPUTINFO info = new LASTINPUTINFO();
            info.cbSize = (uint)System.Runtime.InteropServices.Marshal.SizeOf(info);
            GetLastInputInfo(ref info);

            // 現在時刻取得
            UInt64 dwNow = GetTickCount();

            // GetTickCount()は49.7日周期でリセットされるので桁上りさせる
            if (info.dwTime > dwNow)
            {
                dwNow += 0x100000000;
            }

            StringBuilder sb = new StringBuilder(1024);
            IniFileHandler.GetPrivateProfileString("SET", "RecCloseCheck", "False", sb, (uint)sb.Capacity, SettingPath.TimerSrvIniPath);
            bool BoolTryParse = false, BoolCheck = false; ;
            if (bool.TryParse(sb.ToString(), out BoolCheck))//紅
                BoolTryParse = BoolCheck;

            if (BoolTryParse)
            {
                UInt32 RecCloseTime = UInt32.Parse(IniFileHandler.GetPrivateProfileInt("SET", "RecCloseTime", 0, SettingPath.TimerSrvIniPath).ToString()); //紅

                UInt32 threshold = RecCloseTime * 60 * 1000;

                if (RecCloseTime != 0 && dwNow - info.dwTime >= threshold)
                {
                    SleepDialog(obj);
                }
            }
            else SleepDialog(obj);
        }

        void SleepDialog(object obj)
        {
            UInt16 param = (UInt16)obj;
            Byte reboot = (Byte)((param & 0xFF00) >> 8);
            Byte suspendMode = (Byte)(param & 0x00FF);

            Int32 sleepmin = (Int32)IniFileHandler.GetPrivateProfileInt("SET", "RecMarginTime", 0, SettingPath.TimerSrvIniPath);

            sleepmin = sleepmin * 60000;
            Thread.Sleep(sleepmin);

            Dispatcher.BeginInvoke(new Action(() =>
            {
                SuspendCheckWindow dlg = new SuspendCheckWindow();
                dlg.SetMode(0, suspendMode);
                if (dlg.ShowDialog() != true)
                {
                    cmd.SendSuspend(param);
                }
            }));
        }

        void NotifyStatus(NotifySrvInfo status)
        {
            System.Diagnostics.Trace.WriteLine((UpdateNotifyItem)status.notifyID);

            switch ((UpdateNotifyItem)status.notifyID)
            {
                case UpdateNotifyItem.EpgData:
                    {
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.EpgData);
                        if (PresentationSource.FromVisual(Application.Current.MainWindow) != null)
                        {
                            epgView.UpdateEpgData();
                        }
                        GC.Collect();
                    }
                    break;
                case UpdateNotifyItem.ReserveInfo:
                    {
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                        reserveView.UpdateReserveData();
                        epgView.UpdateReserveData();
                        tunerReserveView.UpdateReserveData();

                    }
                    break;
                case UpdateNotifyItem.RecInfo:
                    {
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.RecInfo);
                        recInfoView.UpdateInfo();
                    }
                    break;
                case UpdateNotifyItem.AutoAddEpgInfo:
                    {
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                        autoAddView.UpdateAutoAddInfo();
                    }
                    break;
                case UpdateNotifyItem.AutoAddManualInfo:
                    {
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddManualInfo);
                        autoAddView.UpdateAutoAddInfo();
                    }
                    break;
                case UpdateNotifyItem.SrvStatus:
                    {
                        if (status.param1 == 1)
                        {
                            taskTray.Icon = Properties.Resources.TaskIconRed;
                        }
                        else if (status.param1 == 2)
                        {
                            taskTray.Icon = Properties.Resources.TaskIconGreen;
                        }
                        else
                        {
                            taskTray.Icon = Properties.Resources.TaskIconBlue;
                        }
                    }
                    break;
                case UpdateNotifyItem.PreRecStart:
                    {
                        taskTray.ShowBalloonTip("予約録画開始準備", status.param4, 10 * 1000);
                        CommonManager.Instance.NotifyLogList.Add(status);
                    }
                    break;
                case UpdateNotifyItem.RecStart:
                    {
                        taskTray.ShowBalloonTip("録画開始", status.param4, 10 * 1000);
                        CommonManager.Instance.NotifyLogList.Add(status);
                    }
                    break;
                case UpdateNotifyItem.RecEnd:
                    {
                        taskTray.ShowBalloonTip("録画終了", status.param4, 10 * 1000);
                        CommonManager.Instance.NotifyLogList.Add(status);
                    }
                    break;
                case UpdateNotifyItem.RecTuijyu:
                    {
                        taskTray.ShowBalloonTip("追従発生", status.param4, 10 * 1000);
                        CommonManager.Instance.NotifyLogList.Add(status);
                    }
                    break;
                case UpdateNotifyItem.ChgTuijyu:
                    {
                        taskTray.ShowBalloonTip("番組変更", status.param4, 10 * 1000);
                        CommonManager.Instance.NotifyLogList.Add(status);
                    }
                    break;
                case UpdateNotifyItem.PreEpgCapStart:
                    {
                        taskTray.ShowBalloonTip("EPG取得", status.param4, 10 * 1000);
                        CommonManager.Instance.NotifyLogList.Add(status);
                    }
                    break;
                case UpdateNotifyItem.EpgCapStart:
                    {
                        taskTray.ShowBalloonTip("EPG取得", "開始", 10 * 1000);
                        CommonManager.Instance.NotifyLogList.Add(status);
                    }
                    break;
                case UpdateNotifyItem.EpgCapEnd:
                    {
                        taskTray.ShowBalloonTip("EPG取得", "終了", 10 * 1000);
                        CommonManager.Instance.NotifyLogList.Add(status);
                    }
                    break;
                default:
                    break;
            }

            CommonManager.Instance.DB.ReloadReserveInfo();
            ReserveData item = new ReserveData();

            if (CommonManager.Instance.DB.GetNextReserve(ref item) == true)
            {
                String timeView = item.StartTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
                DateTime endTime = item.StartTime + TimeSpan.FromSeconds(item.DurationSecond);
                timeView += endTime.ToString("HH:mm:ss");
                taskTray.Text = "次の予約：" + item.StationName + " " + timeView + " " + item.Title;
            }
            else
            {
                taskTray.Text = "次の予約なし";
            }
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control))
            {
                switch (e.Key)
                {
                    case Key.C:
                        this.buttonList["設定"].RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.F:
                        this.buttonList["検索"].RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.D1:
                        new BlackoutWindow(this).showWindow(this.tabItem_reserve_Title.Text);
                        this.tabItem_reserve.IsSelected = true;
                        break;
                    case Key.D2:
                        new BlackoutWindow(this).showWindow(this.tabItem_tunerReserve_Title.Text);
                        this.tabItem_tunerReserve.IsSelected = true;
                        break;
                    case Key.D3:
                        new BlackoutWindow(this).showWindow(this.tabItem_recinfo_Title.Text);
                        this.tabItem_recinfo.IsSelected = true;
                        break;
                    case Key.D4:
                        new BlackoutWindow(this).showWindow(this.tabItem_epgAutoAdd_Title.Text);
                        this.tabItem_epgAutoAdd.IsSelected = true;
                        break;
                    case Key.D5:
                        this.moveTo_tabItem_epg();
                        break;
                }
                //
                base.OnKeyDown(e);
            }
        }

        public void moveTo_tabItem_epg()
        {
            new BlackoutWindow(this).showWindow(this.tabItem_epg_Title.Text);
            this.tabItem_epg.IsSelected = true;
        }

        public Button getSearchButton(bool isShowButton0)
        {
            Button button1 = buttonList["検索"];
            if (Settings.Instance.ViewButtonList.Contains("検索") == false)
            {
                if (isShowButton0)
                {
                    stackPanel_button.Children.Add(button1);
                }
                else
                {
                    stackPanel_button.Children.Remove(button1);
                }
            }
            return button1;
        }

    }
}
