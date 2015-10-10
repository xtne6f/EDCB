using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.Windows;
using System.Runtime.InteropServices;
using System.Net;

namespace EpgTimer
{
    public class TVTestCtrlClass
    {
        Process process = null;
        string processType;
        private CtrlCmdUtil cmd = null;

        public TVTestCtrlClass(CtrlCmdUtil ctrlCmd)
        {
            cmd = ctrlCmd;
        }

        public bool SetLiveCh(UInt16 ONID, UInt16 TSID, UInt16 SID)
        {
            try
            {
                if (Settings.Instance.TvTestExe.Length == 0)
                {
                    MessageBox.Show("TVTest.exeのパスが設定されていません");
                    return false;
                }
                // TVTestのパスが録画用アプリと一致する場合はViewアプリとして扱う
                bool isView = CommonManager.Instance.NWMode == false &&
                              Settings.Instance.NwTvMode == false &&
                              string.Compare(IniFileHandler.GetPrivateProfileString("SET", "RecExePath", "", SettingPath.CommonIniPath), Settings.Instance.TvTestExe, true) == 0;
                OpenTVTest(Settings.Instance.TvTestOpenWait, isView ? "View" : "TvTest");
                var cmdTvTest = new CtrlCmdUtil();
                cmdTvTest.SetPipeSetting("Global\\" + processType + "_Ctrl_BonConnect_" + process.Id, "\\\\.\\pipe\\" + processType + "_Ctrl_BonPipe_" + process.Id);
                cmdTvTest.SetConnectTimeOut(1000);

                if (Settings.Instance.NwTvMode == true)
                {
                    SetChInfo chInfo = new SetChInfo();
                    chInfo.useSID = 1;
                    chInfo.useBonCh = 0;
                    chInfo.ONID = ONID;
                    chInfo.TSID = TSID;
                    chInfo.SID = SID;

                    UInt32 nwMode = 0;
                    String nwBonDriver = "BonDriver_UDP.dll";
                    if (Settings.Instance.NwTvModeUDP == true)
                    {
                        nwMode += 1;
                    }
                    if (Settings.Instance.NwTvModeTCP == true)
                    {
                        nwMode += 2;
                        nwBonDriver = "BonDriver_TCP.dll";
                    }
                    
                    if (cmd.SendNwTVMode(nwMode) == ErrCode.CMD_SUCCESS)
                    {
                        if (cmd.SendNwTVSetCh(chInfo) == ErrCode.CMD_SUCCESS)
                        {
                            String val = "";
                            for (int i = 0; i < 10; i++)
                            {
                                if (cmdTvTest.SendViewGetBonDrivere(ref val) != ErrCode.CMD_SUCCESS)
                                {
                                    System.Threading.Thread.Sleep(1000);
                                    continue;
                                }
                                if (String.Compare(val, nwBonDriver, true) != 0)
                                {
                                    cmdTvTest.SendViewSetBonDrivere(nwBonDriver);
                                }
                                break;
                            }
                        }
                    }
                }
                else
                {
                    UInt64 key = ((UInt64)ONID) << 32 |
                        ((UInt64)TSID) << 16 |
                        ((UInt64)SID);
                    TvTestChChgInfo chInfo = new TvTestChChgInfo();
                    ErrCode err = cmd.SendGetChgChTVTest(key, ref chInfo);
                    if (err == ErrCode.CMD_SUCCESS)
                    {
                        String val = "";
                        for (int i = 0; i < 10; i++)
                        {
                            if (cmdTvTest.SendViewGetBonDrivere(ref val) != ErrCode.CMD_SUCCESS)
                            {
                                System.Threading.Thread.Sleep(1000);
                                continue;
                            }
                            // 識別用IDが設定されたViewアプリは弄らない
                            if (processType == "View")
                            {
                                int id = -1;
                                if (cmdTvTest.SendViewGetID(ref id) != ErrCode.CMD_SUCCESS || id >= 0)
                                {
                                    break;
                                }
                            }
                            if (String.Compare(val, chInfo.bonDriver, true) != 0)
                            {
                                if (cmdTvTest.SendViewSetBonDrivere(chInfo.bonDriver) == ErrCode.CMD_SUCCESS)
                                {
                                    System.Threading.Thread.Sleep(Settings.Instance.TvTestChgBonWait);
                                    cmdTvTest.SendViewSetCh(chInfo.chInfo);
                                }
                            }
                            else
                            {
                                cmdTvTest.SendViewSetCh(chInfo.chInfo);
                            }
                            break;
                        }
                    }
                    else if (err == ErrCode.CMD_ERR_CONNECT)
                    {
                        MessageBox.Show("サーバーに接続できませんでした");
                        return false;
                    }
                    else
                    {
                        MessageBox.Show("指定サービスを受信できるBonDriverが設定されていません。");
                        return false;
                    }
                }

                WakeupWindow(process.MainWindowHandle);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            } 
            return true;
        }

        public bool StartTimeShift(UInt32 reserveID)
        {
            try
            {
                if (Settings.Instance.TvTestExe.Length == 0)
                {
                    MessageBox.Show("TVTest.exeのパスが設定されていません");
                    return false;
                }
                if (CommonManager.Instance.NWMode == false)
                {
                    if (IniFileHandler.GetPrivateProfileInt("SET", "EnableTCPSrv", 0, SettingPath.TimerSrvIniPath) == 0)
                    {
                        MessageBox.Show("追っかけ再生を行うには、動作設定でネットワーク接続を許可する必要があります。");
                        return false;
                    }
                }
                NWPlayTimeShiftInfo playInfo = new NWPlayTimeShiftInfo();
                ErrCode err = cmd.SendNwTimeShiftOpen(reserveID, ref playInfo);
                if (err == ErrCode.CMD_ERR_CONNECT)
                {
                    MessageBox.Show("サーバーに接続できませんでした");
                    return false;
                }
                else if( err != ErrCode.CMD_SUCCESS )
                {
                    MessageBox.Show("まだ録画が開始されていません");
                    return false;
                }

                OpenTVTest(1000, "TvTest");
                var cmdTvTest = new CtrlCmdUtil();
                cmdTvTest.SetPipeSetting("Global\\TvTest_Ctrl_BonConnect_" + process.Id, "\\\\.\\pipe\\TvTest_Ctrl_BonPipe_" + process.Id);
                cmdTvTest.SetConnectTimeOut(1000);

                TVTestStreamingInfo sendInfo = new TVTestStreamingInfo();
                sendInfo.enableMode = 1;
                sendInfo.ctrlID = playInfo.ctrlID;
                if (CommonManager.Instance.NWMode == false)
                {
                    sendInfo.serverIP = 0x7F000001;
                    // 原作はここで自ホスト名を取得して解決したアドレスを格納している。(ないとは思うが)不具合があれば戻すこと
                    sendInfo.serverPort = (UInt32)IniFileHandler.GetPrivateProfileInt("SET", "TCPPort", 4510, SettingPath.TimerSrvIniPath);
                }
                else
                {
                    sendInfo.serverIP = 0x7F000001;
                    IPAddress srvIP = CommonManager.Instance.NW.ConnectedIP;
                    if (srvIP != null && srvIP.GetAddressBytes().Length == 4)
                    {
                        byte[] oct = srvIP.GetAddressBytes();
                        sendInfo.serverIP = (uint)oct[0] << 24 | (uint)oct[1] << 16 | (uint)oct[2] << 8 | oct[3];
                    }
                    sendInfo.serverPort = CommonManager.Instance.NW.ConnectedPort;
                }
                sendInfo.filePath = playInfo.filePath;
                if (Settings.Instance.NwTvModeUDP == true)
                {
                    sendInfo.udpSend = 1;
                }
                if (Settings.Instance.NwTvModeTCP == true)
                {
                    sendInfo.tcpSend = 1;
                }
                for (int i = 0; i < 10 && cmdTvTest.SendViewSetStreamingInfo(sendInfo) != ErrCode.CMD_SUCCESS; i++)
                {
                    System.Threading.Thread.Sleep(1000);
                }

                WakeupWindow(process.MainWindowHandle);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            } 
            return true;
        }

        public bool StartStreamingPlay(String filePath, IPAddress srvIP, UInt32 srvPort)
        {
            try
            {
                if (Settings.Instance.TvTestExe.Length == 0)
                {
                    MessageBox.Show("TVTest.exeのパスが設定されていません");
                    return false;
                }

                UInt32 ctrlID = 0;
                ErrCode err = cmd.SendNwPlayOpen(filePath, ref ctrlID);
                if (err == ErrCode.CMD_ERR_CONNECT)
                {
                    MessageBox.Show("サーバーに接続できませんでした");
                    return false;
                }
                else if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show("まだ録画が開始されていません");
                    return false;
                }

                OpenTVTest(Settings.Instance.TvTestOpenWait, "TvTest");
                var cmdTvTest = new CtrlCmdUtil();
                cmdTvTest.SetPipeSetting("Global\\TvTest_Ctrl_BonConnect_" + process.Id, "\\\\.\\pipe\\TvTest_Ctrl_BonPipe_" + process.Id);
                cmdTvTest.SetConnectTimeOut(1000);

                UInt32 ip = 0x7F000001;
                if (srvIP != null && srvIP.GetAddressBytes().Length == 4)
                {
                    byte[] oct = srvIP.GetAddressBytes();
                    ip = (uint)oct[0] << 24 | (uint)oct[1] << 16 | (uint)oct[2] << 8 | oct[3];
                }

                TVTestStreamingInfo sendInfo = new TVTestStreamingInfo();
                sendInfo.enableMode = 1;
                sendInfo.ctrlID = ctrlID;
                sendInfo.serverIP = ip;
                sendInfo.serverPort = srvPort;

                if (Settings.Instance.NwTvModeUDP == true)
                {
                    sendInfo.udpSend = 1;
                }
                if (Settings.Instance.NwTvModeTCP == true)
                {
                    sendInfo.tcpSend = 1;
                }
                for (int i = 0; i < 10 && cmdTvTest.SendViewSetStreamingInfo(sendInfo) != ErrCode.CMD_SUCCESS; i++)
                {
                    System.Threading.Thread.Sleep(1000);
                }

                WakeupWindow(process.MainWindowHandle);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return true;
        }


        private void OpenTVTest(int openWait, string type)
        {
            if (process == null || process.HasExited || processType != type)
            {
                processType = type;
                process = FindTVTestProcess(type);
                if (process == null)
                {
                    process = Process.Start(Settings.Instance.TvTestExe, Settings.Instance.TvTestCmd);
                    System.Threading.Thread.Sleep(openWait);
                }
            }
        }

        private static Process FindTVTestProcess(string type)
        {
            foreach (Process p in Process.GetProcesses())
            {
                // 原作と異なりプロセス名ではなく接続待機用イベントの有無で判断するので注意
                try
                {
                    using (System.Threading.EventWaitHandle.OpenExisting(
                               "Global\\" + type + "_Ctrl_BonConnect_" + p.Id, System.Security.AccessControl.EventWaitHandleRights.Synchronize))
                    {
                    }
                    // 識別用IDが設定されたViewアプリは除外する
                    if (type == "View")
                    {
                        var cmdTvTest = new CtrlCmdUtil();
                        cmdTvTest.SetPipeSetting("Global\\View_Ctrl_BonConnect_" + p.Id, "\\\\.\\pipe\\View_Ctrl_BonPipe_" + p.Id);
                        cmdTvTest.SetConnectTimeOut(1000);
                        int id = -1;
                        if (cmdTvTest.SendViewGetID(ref id) != ErrCode.CMD_SUCCESS || id >= 0)
                        {
                            continue;
                        }
                    }
                    return p;
                }
                catch { }
            }
            return null;
        }

        public void CloseTVTest()
        {
            if (process != null && process.HasExited == false)
            {
                var cmdTvTest = new CtrlCmdUtil();
                cmdTvTest.SetPipeSetting("Global\\" + processType + "_Ctrl_BonConnect_" + process.Id, "\\\\.\\pipe\\" + processType + "_Ctrl_BonPipe_" + process.Id);
                cmdTvTest.SetConnectTimeOut(1000);
                cmdTvTest.SendViewAppClose();
            }
            process = null;
            if (Settings.Instance.NwTvMode == true)
            {
                cmd.SendNwTVClose();
            }
        }

        // 外部プロセスのウィンドウを起動する
        private static void WakeupWindow(IntPtr windowHandle)
        {
            if (IsIconic(windowHandle))
            {
                ShowWindowAsync(windowHandle, SW_RESTORE);
            }
            // メイン・ウィンドウを最前面に表示する
            SetForegroundWindow(windowHandle);
        }
        // 外部プロセスのメイン・ウィンドウを起動するためのWin32 API
        [DllImport("user32.dll")]
        private static extern bool SetForegroundWindow(IntPtr hWnd);
        [DllImport("user32.dll")]
        private static extern bool ShowWindowAsync(IntPtr hWnd, int nCmdShow);
        [DllImport("user32.dll")]
        private static extern bool IsIconic(IntPtr hWnd);
        // ShowWindowAsync関数のパラメータに渡す定義値
        private const int SW_RESTORE = 9;  // 画面を元の大きさに戻す
    }
}
