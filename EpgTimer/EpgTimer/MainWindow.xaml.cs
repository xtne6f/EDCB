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

        private MenuBinds mBinds = new MenuBinds();

        private PipeServer pipeServer = null;
        private string pipeName = "\\\\.\\pipe\\EpgTimerGUI_Ctrl_BonPipe_";
        private string pipeEventName = "Global\\EpgTimerGUI_Ctrl_BonConnect_";

        private bool closeFlag = false;
        private bool initExe = false;

        private System.Windows.Threading.DispatcherTimer chkRegistTCPTimer = null;
        private bool needUnRegist = true;

        private bool idleShowBalloon = false;

        public MainWindow()
        {
            string appName = System.IO.Path.GetFileNameWithoutExtension(System.Reflection.Assembly.GetEntryAssembly().Location);
            CommonManager.Instance.NWMode = appName == "EpgTimerNW";

            Settings.LoadFromXmlFile(CommonManager.Instance.NWMode);
            if (CommonManager.Instance.NWMode == true)
            {
                CommonManager.Instance.DB.SetNoAutoReloadEPG(Settings.Instance.NgAutoEpgLoadNW);
                cmd.SetSendMode(true);
                cmd.SetNWSetting("", Settings.Instance.NWServerPort);

                if (Settings.Instance.ChkSrvRegistTCP == true)
                {
                    chkRegistTCPTimer = new System.Windows.Threading.DispatcherTimer();
                    chkRegistTCPTimer.Interval = TimeSpan.FromMinutes(Math.Max(Settings.Instance.ChkSrvRegistInterval, 1));
                    chkRegistTCPTimer.Tick += (sender, e) =>
                    {
                        if (CommonManager.Instance.NW.IsConnected == true)
                        {
                            bool registered = true;
                            if ((ErrCode)cmd.SendIsRegistTCP(Settings.Instance.NWWaitPort, ref registered) == ErrCode.CMD_SUCCESS)
                            {
                                if (registered == false)
                                {
                                    ConnectCmd(false);
                                }
                            }
                        }
                    };
                    chkRegistTCPTimer.Start();
                }
            }

            ChSet5.LoadFile();
            CommonManager.Instance.MM.ReloadWorkData();
            CommonManager.Instance.ReloadCustContentColorList();

            if (Settings.Instance.NoStyle == 1)
            {
                App.Current.Resources = new ResourceDictionary();
            }
            else
            {
                //EpgTimerはボタンだけ独自テーマだけど、どういう経緯があったのだろう？
                App.Current.Resources.MergedDictionaries.Add(
                    //Application.LoadComponent(new Uri("/PresentationFramework.AeroLite, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/aerolite.normalcolor.xaml", UriKind.Relative)) as ResourceDictionary
                    //Application.LoadComponent(new Uri("/PresentationFramework.Aero2, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/aero2.normalcolor.xaml", UriKind.Relative)) as ResourceDictionary
                    Application.LoadComponent(new Uri("/PresentationFramework.Aero, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/aero.normalcolor.xaml", UriKind.Relative)) as ResourceDictionary
                    //Application.LoadComponent(new Uri("/PresentationFramework.Royale, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/royale.normalcolor.xaml", UriKind.Relative)) as ResourceDictionary
                    //Application.LoadComponent(new Uri("/PresentationFramework.Luna, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/luna.normalcolor.xaml", UriKind.Relative)) as ResourceDictionary
                    //Application.LoadComponent(new Uri("/PresentationFramework.Classic, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/classic.xaml", UriKind.Relative)) as ResourceDictionary
                    );
            }

            mutex = new System.Threading.Mutex(false, CommonManager.Instance.NWMode ? "Global\\EpgTimer_BonNW" : "Global\\EpgTimer_Bon2");
            if (!mutex.WaitOne(0, false))
            {
                CheckCmdLine();

                mutex.Close();
                mutex = null;

                CloseCmd();
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
                        String moduleFolder = SettingPath.ModulePath.TrimEnd('\\');
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
                    CloseCmd();
                    return;
                }
            }

            InitializeComponent();

            Title = appName;
            initExe = true;

            try
            {
                if (Settings.Instance.WakeMin == true)
                {
                    if (Settings.Instance.ShowTray && Settings.Instance.MinHide)
                    {
                        this.Visibility = System.Windows.Visibility.Hidden;
                    }
                    else
                    {
                        Dispatcher.BeginInvoke(new Action(() =>
                        {
                            this.WindowState = System.Windows.WindowState.Minimized;
                        }));
                    }
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
                Action<string, RoutedEventHandler> ButtonGen = (key, handler) =>
                {
                    Button btn = new Button();
                    btn.MinWidth = 75;
                    btn.Margin = new Thickness(2, 2, 2, 5);
                    if (handler != null) btn.Click += new RoutedEventHandler(handler);
                    btn.Content = key;
                    buttonList.Add(key, btn);
                };
                ButtonGen("設定", settingButton_Click);
                ButtonGen("検索", null);
                ButtonGen("終了", closeButton_Click);
                ButtonGen("スタンバイ", standbyButton_Click);
                ButtonGen("休止", suspendButton_Click);
                ButtonGen("EPG取得", epgCapButton_Click);
                ButtonGen("EPG再読み込み", epgReloadButton_Click);
                ButtonGen("カスタム１", custum1Button_Click);
                ButtonGen("カスタム２", custum2Button_Click);
                ButtonGen("NetworkTV終了", nwTVEndButton_Click);
                ButtonGen("情報通知ログ", logViewButton_Click);
                ButtonGen("再接続", connectButton_Click);

                //検索ボタンは他と共通でショートカット割り振られているので、コマンド側で処理する。
                this.CommandBindings.Add(new CommandBinding(EpgCmds.Search, searchButton_Click));
                mBinds.SetCommandToButton(buttonList["検索"], EpgCmds.Search);
                RefreshButton();

                ResetButtonView();

                if (CommonManager.Instance.NWMode == false)
                {
                    pipeServer = new PipeServer();
                    pipeName += System.Diagnostics.Process.GetCurrentProcess().Id.ToString();
                    pipeEventName += System.Diagnostics.Process.GetCurrentProcess().Id.ToString();
                    pipeServer.StartServer(pipeEventName, pipeName, OutsideCmdCallback, this);

                    for (int i = 0; i < 150 && cmd.SendRegistGUI((uint)System.Diagnostics.Process.GetCurrentProcess().Id) != (uint)ErrCode.CMD_SUCCESS; i++)
                    {
                        Thread.Sleep(100);
                    }
                }

                //タスクトレイの表示
                taskTray = new TaskTrayClass(this);
                taskTray.Icon = Properties.Resources.TaskIconBlue;
                taskTray.Visible = Settings.Instance.ShowTray;
                taskTray.ContextMenuClick += new EventHandler(taskTray_ContextMenuClick);
                taskTray.Text = GetTaskTrayReserveInfoText();
                ResetTaskMenu();

                CheckCmdLine();
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
                PresentationSource topWindow = PresentationSource.FromVisual(this);
                if (topWindow == null)
                {
                    this.Visibility = System.Windows.Visibility.Visible;
                    this.WindowState = Settings.Instance.LastWindowState;
                    Dispatcher.BeginInvoke(new Action(() =>
                    {
                        SettingCmd();
                    }));
                }
                else
                {
                    SettingCmd();
                }
            }
            else if (String.Compare("終了", tag) == 0)
            {
                CloseCmd();
            }
            else if (String.Compare("スタンバイ", tag) == 0)
            {
                SuspendCmd(1);
            }
            else if (String.Compare("休止", tag) == 0)
            {
                SuspendCmd(2);
            }
            else if (String.Compare("EPG取得", tag) == 0)
            {
                EpgCapCmd();
            }
            else if (String.Compare("再接続", tag) == 0)
            {
                if (CommonManager.Instance.NWMode == true)
                {
                    ConnectCmd(true);
                }
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
            for (int i = 0; i < tabControl_main.Items.Count; i++)
            {
                TabItem ti = tabControl_main.Items.GetItemAt(i) as TabItem;
                if (ti != null && ti.Tag is string && ((string)ti.Tag).StartsWith("PushLike"))
                {
                    tabControl_main.Items.Remove(ti);
                    i--;
                }
            }
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

                        if (Settings.Instance.ViewButtonShowAsTab)
                        {
                            //ボタン風のタブを追加する
                            TabItem ti = new TabItem();
                            ti.Header = buttonList[info].Content;
                            ti.Tag = "PushLike" + info;
                            ti.Background = null;
                            ti.BorderBrush = null;
                            //タブ移動をキャンセルしつつ擬似的に対応するボタンを押す
                            ti.PreviewMouseDown += (sender, e) =>
                            {
                                if (e.ChangedButton == MouseButton.Left)
                                {
                                    Button btn = buttonList[((string)((TabItem)sender).Tag).Substring(8)];
                                    if (btn.Command != null)
                                    {
                                        btn.Command.Execute(btn.CommandParameter);
                                    }
                                    else
                                    {
                                        btn.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                                    }
                                    e.Handled = true;
                                }
                            };
                            //コマンド割り当ての場合の自動ツールチップ表示、一応ボタンと同様のショートカット変更対応のコード
                            if (buttonList[info].Command != null)
                            {
                                ti.ToolTip = "";
                                ti.ToolTipOpening += new ToolTipEventHandler((sender, e) =>
                                {
                                    var icmd = buttonList[((string)((TabItem)sender).Tag).Substring(8)].Command;
                                    ti.ToolTip = MenuBinds.GetInputGestureText(icmd);
                                    ti.ToolTip = ti.ToolTip == null ? "" : ti.ToolTip;
                                });
                            }
                            tabControl_main.Items.Add(ti);
                        }
                    }
                }
            }
            //タブとして表示するかボタンが1つもないときは行を隠す
            rowDefinition_row0.Height = new GridLength(Settings.Instance.ViewButtonShowAsTab || stackPanel_button.Children.Count == 0 ? 0 : 30);
        }

        bool ConnectCmd(bool showDialog)
        {
            if (showDialog == true)
            {
                ConnectWindow dlg = new ConnectWindow();
                PresentationSource topWindow = PresentationSource.FromVisual(this);
                if (topWindow != null)
                {
                    dlg.Owner = (Window)topWindow.RootVisual;
                }
                if (dlg.ShowDialog() == false)
                {
                    return true;
                }
            }

            bool connected = false;
            String srvIP = Settings.Instance.NWServerIP;
            try
            {
                foreach (var address in System.Net.Dns.GetHostAddresses(srvIP))
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
                if (showDialog == true)
                {
                    MessageBox.Show("サーバーへの接続に失敗しました");
                }
                return false;
            }

            IniFileHandler.UpdateSrvProfileIniNW();

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
            CommonManager.Instance.DB.ReloadReserveInfo();
            CommonManager.Instance.DB.ReloadEpgData();
            reserveView.UpdateInfo();
            epgView.UpdateReserveData();
            tunerReserveView.UpdateInfo();
            autoAddView.UpdateAutoAddInfo();
            recInfoView.UpdateInfo();
            epgView.UpdateEpgData();
            return true;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if (CommonManager.Instance.NWMode == true)
            {
                if (Settings.Instance.WakeReconnectNW == false || ConnectCmd(false) == false)
                {
                    ConnectCmd(true);
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
                if (initExe == true)
                {
                    SaveData();
                }
                if (CommonManager.Instance.NWMode == false)
                {
                    if (initExe == true)
                    {
                        cmd.SetConnectTimeOut(3000);
                        cmd.SendUnRegistGUI((uint)System.Diagnostics.Process.GetCurrentProcess().Id);
                    }
                    pipeServer.StopServer();

                    if (mutex != null)
                    {
                        if (serviceMode == false && initExe == true)
                        {
                            cmd.SendClose();
                        }
                    }
                }
                else
                {
                    if (CommonManager.Instance.NW.IsConnected == true && needUnRegist == true)
                    {
                        if (cmd.SendUnRegistTCP(Settings.Instance.NWServerPort) == 205)
                        {
                            //MessageBox.Show("サーバーに接続できませんでした");
                        }
                    }
                }
                if (mutex != null)
                {
                    mutex.ReleaseMutex();
                    mutex.Close();
                }
                if (taskTray != null)
                {
                    taskTray.Dispose();
                }
            }
        }

        private void SaveData()
        {
            SaveViewData();
            Settings.SaveToXmlFile();
        }

        private void SaveViewData()
        {
            reserveView.SaveViewData();
            recInfoView.SaveViewData();
            autoAddView.SaveViewData();
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
                if (Settings.Instance.ShowTray && Settings.Instance.MinHide)
                {
                    this.Visibility = System.Windows.Visibility.Hidden;
                }
            }
            if (this.WindowState == WindowState.Normal || this.WindowState == WindowState.Maximized)
            {
                this.Visibility = System.Windows.Visibility.Visible;
                taskTray.LastViewState = this.WindowState;
                Settings.Instance.LastWindowState = this.WindowState;
            }
            taskTray.Visible = Settings.Instance.ShowTray;
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

        private void Window_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.D1:
                        if (e.IsRepeat == false)
                        {
                            this.tabItem_reserve.IsSelected = true;
                        }
                        e.Handled = true;
                        break;
                    case Key.D2:
                        if (e.IsRepeat == false)
                        {
                            this.tabItem_tunerReserve.IsSelected = true;
                        }
                        e.Handled = true;
                        break;
                    case Key.D3:
                        if (e.IsRepeat == false)
                        {
                            this.tabItem_recinfo.IsSelected = true;
                        }
                        e.Handled = true;
                        break;
                    case Key.D4:
                        if (e.IsRepeat == false)
                        {
                            this.tabItem_epgAutoAdd.IsSelected = true;
                        }
                        e.Handled = true;
                        break;
                    case Key.D5:
                        if (e.IsRepeat == false)
                        {
                            this.tabItem_epg.IsSelected = true;
                        }
                        e.Handled = true;
                        break;
                }
            }
        }

        void settingButton_Click(object sender, RoutedEventArgs e)
        {
            SettingCmd();
        }

        void SettingCmd()
        {
            SaveViewData();

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
                    if (CommonManager.Instance.NWMode == true)
                    {
                        CommonManager.Instance.DB.SetNoAutoReloadEPG(Settings.Instance.NgAutoEpgLoadNW);
                    }
                    else
                    {
                        CommonManager.Instance.CtrlCmd.SendNotifyProfileUpdate();
                    }
                    reserveView.UpdateInfo();
                    tunerReserveView.UpdateInfo();
                    recInfoView.UpdateInfo();
                    autoAddView.UpdateAutoAddInfo();
                    epgView.UpdateSetting();
                    cmd.SendReloadSetting();
                    ResetButtonView();
                    ResetTaskMenu();
                    RefreshMenu(false);
                }
            }
            if (setting.ServiceStop == true)
            {
                MessageBox.Show("サービスの状態を変更したため終了します。");
                initExe = false;
                CloseCmd();
                return;
            }
            ChSet5.LoadFile();
        }

        public void RefreshMenu(bool MenuOnly = false)
        {
            CommonManager.Instance.MM.ReloadWorkData();
            reserveView.RefreshMenu();
            tunerReserveView.RefreshMenu();
            recInfoView.RefreshMenu();
            autoAddView.RefreshMenu();

            //epgViewでは設定全体の更新の際に、EPG再描画に合わせてメニューも更新されるため。
            if (MenuOnly == true)
            {
                epgView.RefreshMenu();
            }

            RefreshButton();
        }
        public void RefreshButton()
        {
            //検索ボタン用。
            mBinds.ResetInputBindings(this);
        }

        void searchButton_Click(object sender, ExecutedRoutedEventArgs e)
        {
            // Hide()したSearchWindowを復帰
            foreach (Window win1 in this.OwnedWindows)
            {
                if (win1 is SearchWindow)
                {
                    //他で予約情報が更新されてたりするので情報を再読み込みさせる。その後はモーダルウィンドウに。
                    //ウィンドウ管理を真面目にやればモードレスもありか
                    (win1 as SearchWindow).button_search.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                    win1.ShowDialog();
                    return;
                }
            }
            //
            CommonManager.Instance.MUtil.OpenSearchEpgDialog(this);
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

        void standbyButton_Click(object sender, RoutedEventArgs e)
        {
            SuspendCmd(1);
        }

        void suspendButton_Click(object sender, RoutedEventArgs e)
        {
            SuspendCmd(2);
        }

        void SuspendCmd(byte suspendMode)
        {
            suspendMode = suspendMode == 1 ? suspendMode : (byte)2;
            UInt32 err = cmd.SendChkSuspend();
            if (err != 1)
            {
                if (err == 205)
                {
                    MessageBox.Show("サーバーに接続できませんでした");
                }
                else
                {
                    MessageBox.Show((suspendMode == 1 ? "スタンバイ" : "休止") +
                        "に移行できる状態ではありません。\r\n（もうすぐ予約が始まる。または抑制条件のexeが起動している。など）");
                }
                return;
            }

            if (Settings.Instance.SuspendChk == 1)
            {
                SuspendCheckWindow dlg = new SuspendCheckWindow();
                dlg.SetMode(0, suspendMode);
                if (dlg.ShowDialog() == true)
                {
                    return;
                }
            }

            ushort cmdVal = suspendMode;
            if (IniFileHandler.GetPrivateProfileInt("SET", "Reboot", 0, SettingPath.TimerSrvIniPath) == 1)
            {
                cmdVal |= 0x0100;
            }
            if (CommonManager.Instance.NWMode == true)
            {
                cmdVal |= 0xFF00;//今はサーバ側の設定を読めてるので無くても大丈夫なはずだけど、一応そのまま

                if (Settings.Instance.SuspendCloseNW == true)
                {
                    if (CommonManager.Instance.NW.IsConnected == true)
                    {
                        if (cmd.SendUnRegistTCP(Settings.Instance.NWServerPort) == 205)
                        { }

                        cmd.SendSuspend(cmdVal);
                        needUnRegist = false;
                        CloseCmd();
                        return;
                    }
                }
            }
            SaveData();
            cmd.SendSuspend(cmdVal);
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
            if (CommonManager.Instance.NWMode == true)
            {
                ConnectCmd(true);
            }
        }

        private int OutsideCmdCallback(object pParam, CMD_STREAM pCmdParam, ref CMD_STREAM pResParam)
        {
            var DispatcherCheckAction = new Action<Action>((action) =>
            {
                if (Dispatcher.CheckAccess() == true)
                {
                    action();
                }
                else
                {
                    Dispatcher.BeginInvoke(action);
                }
            });

            System.Diagnostics.Trace.WriteLine((CtrlCmd)pCmdParam.uiParam);
            pResParam.uiParam = (uint)ErrCode.CMD_SUCCESS;

            switch ((CtrlCmd)pCmdParam.uiParam)
            {
                case CtrlCmd.CMD_TIMER_GUI_SHOW_DLG:
                    this.Visibility = System.Windows.Visibility.Visible;
                    break;
                case CtrlCmd.CMD_TIMER_GUI_UPDATE_RESERVE:
                    {
                        DispatcherCheckAction(new Action(() =>
                        {
                            CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                            CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.RecInfo);
                            CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                            CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddManualInfo);
                            CommonManager.Instance.DB.ReloadReserveInfo();
                            reserveView.UpdateInfo();
                            epgView.UpdateReserveData();
                            tunerReserveView.UpdateInfo();
                            autoAddView.UpdateAutoAddInfo();
                            recInfoView.UpdateInfo();

                            taskTray.Text = GetTaskTrayReserveInfoText();
                        }));
                    }
                    break;
                case CtrlCmd.CMD_TIMER_GUI_UPDATE_EPGDATA:
                    {
                        DispatcherCheckAction(new Action(() =>
                        {
                            CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.EpgData);
                            CommonManager.Instance.DB.ReloadEpgData();
                            reserveView.UpdateInfo();
                            epgView.UpdateEpgData();
                        }));
                    }
                    break;
                case CtrlCmd.CMD_TIMER_GUI_VIEW_EXECUTE:
                    {
                        String exeCmd = "";
                        CmdStreamUtil.ReadStreamData(ref exeCmd, pCmdParam);
                        try
                        {
                            string[] cmd = exeCmd.Split('\"');
                            System.Diagnostics.Process process;
                            System.Diagnostics.ProcessStartInfo startInfo;
                            if (cmd.Length >= 3)
                            {
                                startInfo = new System.Diagnostics.ProcessStartInfo(cmd[1], cmd[2]);
                            }
                            else if (cmd.Length >= 2)
                            {
                                startInfo = new System.Diagnostics.ProcessStartInfo(cmd[1]);
                            }
                            else
                            {
                                startInfo = new System.Diagnostics.ProcessStartInfo(cmd[0]);
                            }
                            if (cmd.Length >= 2)
                            {
                                if (cmd[1].IndexOf(".bat") >= 0)
                                {
                                    startInfo.CreateNoWindow = true;
                                    if (Settings.Instance.ExecBat == 0)
                                    {
                                        startInfo.WindowStyle = System.Diagnostics.ProcessWindowStyle.Minimized;
                                    }
                                    else if (Settings.Instance.ExecBat == 1)
                                    {
                                        startInfo.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;
                                    }
                                }
                            }
                            process = System.Diagnostics.Process.Start(startInfo);
                            CmdStreamUtil.CreateStreamData(process.Id, ref pResParam);
                        }
                        catch { }
                    }
                    break;
                case CtrlCmd.CMD_TIMER_GUI_QUERY_SUSPEND:
                    {
                        UInt16 param = 0;
                        CmdStreamUtil.ReadStreamData(ref param, pCmdParam);
                        Dispatcher.BeginInvoke(new Action(() => ShowSleepDialog(param)));
                    }
                    break;
                case CtrlCmd.CMD_TIMER_GUI_QUERY_REBOOT:
                    {
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
                                SaveData();
                                cmd.SendReboot();
                            }
                        }));
                    }
                    break;
                case CtrlCmd.CMD_TIMER_GUI_SRV_STATUS_CHG:
                    {
                        UInt16 status = 0;
                        CmdStreamUtil.ReadStreamData(ref status, pCmdParam);
                        DispatcherCheckAction(new Action(() => taskTray.Icon = GetTaskTrayIcon(status)));
                    }
                    break;
                case CtrlCmd.CMD_TIMER_GUI_SRV_STATUS_NOTIFY2:
                    {
                        NotifySrvInfo status = new NotifySrvInfo();
                        CmdStreamUtil.ReadStreamData(ref status, pCmdParam);
                        DispatcherCheckAction(new Action(() => NotifyStatus(status)));
                    }
                    break;
                default:
                    pResParam.uiParam = (uint)ErrCode.CMD_NON_SUPPORT;
                    break;
            }
            return 0;
        }

        private System.Drawing.Icon GetTaskTrayIcon(uint status)
        {
            switch(status)
            {
                case 1: return Properties.Resources.TaskIconRed;
                case 2: return Properties.Resources.TaskIconGreen;
                default: return Properties.Resources.TaskIconBlue;
            }
        }

        private string GetTaskTrayReserveInfoText()
        {
            CommonManager.Instance.DB.ReloadReserveInfo();
            ReserveData item = new ReserveData();
            if (CommonManager.Instance.DB.GetNextReserve(ref item) == true)
            {
                String timeView = item.StartTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
                DateTime endTime = item.StartTime + TimeSpan.FromSeconds(item.DurationSecond);
                timeView += endTime.ToString("HH:mm:ss");
                return "次の予約：" + item.StationName + " " + timeView + " " + item.Title;
            }
            else
            {
                return "次の予約なし";
            }
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

        private void ShowSleepDialog(UInt16 param)
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

            if (IniFileHandler.GetPrivateProfileInt("NO_SUSPEND", "NoUsePC", 0, SettingPath.TimerSrvIniPath) == 1)
            {
                UInt32 ngUsePCTime = (UInt32)IniFileHandler.GetPrivateProfileInt("NO_SUSPEND", "NoUsePCTime", 3, SettingPath.TimerSrvIniPath);
                UInt32 threshold = ngUsePCTime * 60 * 1000;

                if (ngUsePCTime == 0 || dwNow - info.dwTime < threshold)
                {
                    return;
                }
            }

            Byte suspendMode = (Byte)(param & 0x00FF);

            {
                SuspendCheckWindow dlg = new SuspendCheckWindow();
                dlg.SetMode(0, suspendMode);
                if (dlg.ShowDialog() != true)
                {
                    SaveData();
                    cmd.SendSuspend(param);
                }
            }
        }

        void NotifyStatus(NotifySrvInfo status)
        {
            int IdleTimeSec = 10 * 60;
            int TimeOutMSec = 10 * 1000;
            var TaskTrayBaloonWork = new Action<string, string>((title, tips) =>
            {
                if (CommonUtil.GetIdleTimeSec() < IdleTimeSec || idleShowBalloon == false)
                {
                    taskTray.ShowBalloonTip(title, tips, TimeOutMSec);
                    if (CommonUtil.GetIdleTimeSec() > IdleTimeSec)
                    {
                        idleShowBalloon = true;
                    }
                }
                CommonManager.Instance.NotifyLogList.Add(status);
                CommonManager.Instance.AddNotifySave(status);
            });

            System.Diagnostics.Trace.WriteLine((UpdateNotifyItem)status.notifyID);

            switch ((UpdateNotifyItem)status.notifyID)
            {
                case UpdateNotifyItem.SrvStatus:
                    taskTray.Icon = GetTaskTrayIcon(status.param1);
                    break;
                case UpdateNotifyItem.PreRecStart:
                    TaskTrayBaloonWork("予約録画開始準備", status.param4);
                    break;
                case UpdateNotifyItem.RecStart:
                    TaskTrayBaloonWork("録画開始", status.param4);
                    reserveView.UpdateInfo();
                    epgView.UpdateReserveData();
                    break;
                case UpdateNotifyItem.RecEnd:
                    TaskTrayBaloonWork("録画終了", status.param4);
                    break;
                case UpdateNotifyItem.RecTuijyu:
                    TaskTrayBaloonWork("追従発生", status.param4);
                    break;
                case UpdateNotifyItem.ChgTuijyu:
                    TaskTrayBaloonWork("番組変更", status.param4);
                    break;
                case UpdateNotifyItem.PreEpgCapStart:
                    TaskTrayBaloonWork("EPG取得", status.param4);
                    break;
                case UpdateNotifyItem.EpgCapStart:
                    TaskTrayBaloonWork("取得", "開始");
                    break;
                case UpdateNotifyItem.EpgCapEnd:
                    TaskTrayBaloonWork("取得", "終了");
                    break;
                case UpdateNotifyItem.EpgData:
                    {
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.EpgData);
                        //NWでは重いが、番組情報などをあちこちで使用しているので即取得する。
                        //自動取得falseのときはReloadEpgData()ではじかれているので元々読込まれない。
                        CommonManager.Instance.DB.ReloadEpgData();
                        reserveView.UpdateInfo();//ジャンルや番組内容などが更新される
                        epgView.UpdateEpgData();
                        GC.Collect();
                    }
                    break;
                case UpdateNotifyItem.ReserveInfo:
                    {
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                        //使用している箇所多いので即取得する。
                        //というより後ろでタスクトレイのルーチンが取得をかけるので遅延の効果がない。
                        CommonManager.Instance.DB.ReloadReserveInfo();
                        reserveView.UpdateInfo();
                        autoAddView.UpdateAutoAddInfo();
                        epgView.UpdateReserveData();
                        tunerReserveView.UpdateInfo();
                    }
                    break;
                case UpdateNotifyItem.RecInfo:
                    {
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.RecInfo);
                        if (CommonManager.Instance.NWMode == false)
                        {
                            CommonManager.Instance.DB.ReloadrecFileInfo();
                        }
                        recInfoView.UpdateInfo();
                    }
                    break;
                case UpdateNotifyItem.AutoAddEpgInfo:
                    {
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                        if (CommonManager.Instance.NWMode == false)
                        {
                            CommonManager.Instance.DB.ReloadEpgAutoAddInfo();
                        }
                        autoAddView.UpdateAutoAddInfo();
                    }
                    break;
                case UpdateNotifyItem.AutoAddManualInfo:
                    {
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddManualInfo);
                        if (CommonManager.Instance.NWMode == false)
                        {
                            CommonManager.Instance.DB.ReloadManualAutoAddInfo();
                        }
                        autoAddView.UpdateAutoAddInfo();
                    }
                    break;
                case UpdateNotifyItem.IniFile:
                    {
                        if (CommonManager.Instance.NWMode == true)
                        {
                            IniFileHandler.UpdateSrvProfileIniNW();
                            reserveView.UpdateInfo();
                            autoAddView.UpdateAutoAddInfo();
                            epgView.UpdateReserveData();
                            tunerReserveView.UpdateInfo();
                        }
                    }
                    break;
                default:
                    break;
            }

            if (CommonUtil.GetIdleTimeSec() < IdleTimeSec)
            {
                idleShowBalloon = false;
            }

            taskTray.Text = GetTaskTrayReserveInfoText();
        }

        void RefreshReserveInfo()
        {
            try
            {
                new BlackoutWindow(this).showWindow("情報の強制更新");
                DBManager DB = CommonManager.Instance.DB;

                //誤って変更しないよう、一度Srv側のリストを読み直す
                DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                if (DB.ReloadEpgAutoAddInfo() == ErrCode.CMD_SUCCESS)
                {
                    if (DB.EpgAutoAddList.Count != 0)
                    {
                        cmd.SendChgEpgAutoAdd(DB.EpgAutoAddList.Values.ToList());
                    }
                }

                //EPG自動登録とは独立
                DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddManualInfo);
                if (DB.ReloadManualAutoAddInfo() == ErrCode.CMD_SUCCESS)
                {
                    if (DB.ManualAutoAddList.Count != 0)
                    {
                        cmd.SendChgManualAdd(DB.ManualAutoAddList.Values.ToList());
                    }
                }

                //上の二つが空リストでなくても、予約情報の更新がされない場合もある
                DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                if (DB.ReloadReserveInfo() == ErrCode.CMD_SUCCESS)
                {
                    if (DB.ReserveList.Count != 0)
                    {
                        //予約一覧は一つでも更新をかければ、再構築される。
                        List<ReserveData> list = new List<ReserveData>();
                        list.Add(DB.ReserveList.Values.ToList()[0]);
                        cmd.SendChgReserve(list);
                    }
                    else
                    {
                        //更新しない場合でも、再描画だけはかけておく
                        reserveView.UpdateInfo();
                        tunerReserveView.UpdateInfo();
                        autoAddView.UpdateAutoAddInfo();
                        epgView.UpdateReserveData();
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }

        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.F5:
                        RefreshReserveInfo();
                        break;
                }
            }
            base.OnKeyDown(e);
        }

        public void moveTo_tabItem_epg()
        {
            BlackoutWindow.NowJumpTable = true;
            new BlackoutWindow(this).showWindow(this.tabItem_epg.Header.ToString());
            this.Focus();//チューナ画面でのフォーカス対策。とりあえずこれで解決する。
            this.tabItem_epg.IsSelected = true;
        }

        public void EmphasizeSearchButton(bool emphasize)
        {
            Button button1 = buttonList["検索"];
            if (Settings.Instance.ViewButtonList.Contains("検索") == false)
            {
                if (emphasize)
                {
                    stackPanel_button.Children.Add(button1);
                }
                else
                {
                    stackPanel_button.Children.Remove(button1);
                }
            }

            //検索ボタンを点滅させる
            if (emphasize)
            {
                button1.Effect = new System.Windows.Media.Effects.DropShadowEffect();
                var animation = new System.Windows.Media.Animation.DoubleAnimation
                {
                    From = 1.0,
                    To = 0.7,
                    RepeatBehavior = System.Windows.Media.Animation.RepeatBehavior.Forever,
                    AutoReverse = true
                };
                button1.BeginAnimation(Button.OpacityProperty, animation);
            }
            else
            {
                button1.BeginAnimation(Button.OpacityProperty, null);
                button1.Opacity = 1;
                button1.Effect = null;
            }

            //もしあればタブとして表示のタブも点滅させる
            foreach (var item in tabControl_main.Items)
            {
                TabItem ti = item as TabItem;
                if (ti != null && ti.Tag is string && (string)ti.Tag == "PushLike検索")
                {
                    if (emphasize)
                    {
                        var animation = new System.Windows.Media.Animation.DoubleAnimation
                        {
                            From = 1.0,
                            To = 0.1,
                            RepeatBehavior = System.Windows.Media.Animation.RepeatBehavior.Forever,
                            AutoReverse = true
                        };
                        ti.BeginAnimation(TabItem.OpacityProperty, animation);
                    }
                    else
                    {
                        ti.BeginAnimation(TabItem.OpacityProperty, null);
                        ti.Opacity = 1;
                    }
                    break;
                }
            }
        }

        public void ListFoucsOnVisibleChanged()
        {
            if (this.reserveView.listView_reserve.IsVisible == true)
            {
                this.reserveView.listView_reserve.Focus();
            }
            else if (this.recInfoView.listView_recinfo.IsVisible == true)
            {
                this.recInfoView.listView_recinfo.Focus();
            }
            else if (this.autoAddView.epgAutoAddView.listView_key.IsVisible == true)
            {
                this.autoAddView.epgAutoAddView.listView_key.Focus();
            }
            else if (this.autoAddView.manualAutoAddView.listView_key.IsVisible == true)
            {
                this.autoAddView.manualAutoAddView.listView_key.Focus();
            }
        }

    }
}
