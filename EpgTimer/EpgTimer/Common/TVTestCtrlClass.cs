using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.Windows;
using System.Runtime.InteropServices;
using System.Net;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public class TVTestCtrlClass
    {
        private int processID = -1;
        CtrlCmdUtil cmdTvTest = new CtrlCmdUtil();
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
                if (IsOpenTVTest() == false)
                {
                    processID = FindTVTestProcess();
                    if (processID == -1)
                    {
                        Process process;
                        process = System.Diagnostics.Process.Start(Settings.Instance.TvTestExe, Settings.Instance.TvTestCmd);
                        processID = process.Id;
                        System.Threading.Thread.Sleep(Settings.Instance.TvTestOpenWait);
                    }
                }
                cmdTvTest.SetPipeSetting("Global\\TvTest_Ctrl_BonConnect_" + processID.ToString(), "\\\\.\\pipe\\TvTest_Ctrl_BonPipe_" + processID.ToString());
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

                WakeupWindow(processID);
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

                if (IsOpenTVTest() == false)
                {
                    processID = FindTVTestProcess();
                    if (processID == -1)
                    {
                        Process process;
                        process = System.Diagnostics.Process.Start(Settings.Instance.TvTestExe, Settings.Instance.TvTestCmd);
                        processID = process.Id;
                        System.Threading.Thread.Sleep(1000);
                    }
                }
                cmdTvTest.SetPipeSetting("Global\\TvTest_Ctrl_BonConnect_" + processID.ToString(), "\\\\.\\pipe\\TvTest_Ctrl_BonPipe_" + processID.ToString());
                cmdTvTest.SetConnectTimeOut(1000);

                TVTestStreamingInfo sendInfo = new TVTestStreamingInfo();
                sendInfo.enableMode = 1;
                sendInfo.ctrlID = playInfo.ctrlID;
                if (CommonManager.Instance.NWMode == false)
                {
                    sendInfo.serverIP = 0x7F000001;

                    string hostname = Dns.GetHostName();
                    IPAddress[] adrList = Dns.GetHostAddresses(hostname);
                    foreach (IPAddress address in adrList)
                    {
                        if (address.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
                        {
                            UInt32 ip = 0;
                            Int32 shift = 24;
                            foreach (string word in address.ToString().Split('.'))
                            {
                                ip |= Convert.ToUInt32(word) << shift;
                                shift -= 8;
                            }
                            sendInfo.serverIP = ip;
                            break;
                        }
                    }

                    sendInfo.serverPort = (UInt32)IniFileHandler.GetPrivateProfileInt("SET", "TCPPort", 4510, SettingPath.TimerSrvIniPath);
                }
                else
                {
                    UInt32 ip = 0;
                    Int32 shift = 24;
                    String srvIP = CommonManager.Instance.NW.ConnectedIP;
                    foreach (IPAddress address in Dns.GetHostAddresses(CommonManager.Instance.NW.ConnectedIP))
                    {
                        srvIP = address.ToString();
                    }
                    foreach (string word in srvIP.Split('.'))
                    {
                        ip |= Convert.ToUInt32(word) << shift;
                        shift -= 8;
                    }
                    sendInfo.serverIP = ip;
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

                WakeupWindow(processID);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            } 
            return true;
        }

        public bool StartStreamingPlay(String filePath, String srvIP, UInt32 srvPort)
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

                if (IsOpenTVTest() == false)
                {
                    processID = FindTVTestProcess();
                    if (processID == -1)
                    {
                        Process process;
                        process = System.Diagnostics.Process.Start(Settings.Instance.TvTestExe, Settings.Instance.TvTestCmd);
                        processID = process.Id;
                        System.Threading.Thread.Sleep(Settings.Instance.TvTestOpenWait);
                    }
                }
                cmdTvTest.SetPipeSetting("Global\\TvTest_Ctrl_BonConnect_" + processID.ToString(), "\\\\.\\pipe\\TvTest_Ctrl_BonPipe_" + processID.ToString());
                cmdTvTest.SetConnectTimeOut(1000);

                UInt32 ip = 0;
                Int32 shift = 24;
                //srvIP = CommonManager.Instance.NW.ConnectedIP;
                foreach (IPAddress address in Dns.GetHostAddresses(srvIP))
                {
                    srvIP = address.ToString();
                }

                foreach (string word in srvIP.Split('.'))
                {
                    ip |= Convert.ToUInt32(word) << shift;
                    shift -= 8;
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

                WakeupWindow(processID);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return true;
        }


        private bool IsOpenTVTest()
        {
            bool ret = false;
            if (processID != -1)
            {
                foreach (Process p in Process.GetProcesses())
                {
                    if (p.Id == processID)
                    {
                        if (String.Compare(p.ProcessName, "tvtest", true) == 0)
                        {
                            ret = true;
                        }
                        break;
                    }
                }
            }
            return ret;
        }

        private int FindTVTestProcess()
        {
            int pid = -1;
            foreach (Process p in Process.GetProcesses())
            {
                if (String.Compare(p.ProcessName, "tvtest", true) == 0)
                {
                    pid = p.Id;
                    break;
                }
            }
            return pid;
        }

        public void CloseTVTest()
        {
            if (processID != -1)
            {
                foreach (Process p in Process.GetProcesses())
                {
                    if (p.Id == processID)
                    {
                        if (String.Compare(p.ProcessName, "tvtest", true) == 0)
                        {
                            cmdTvTest.SetPipeSetting("Global\\TvTest_Ctrl_BonConnect_" + p.Id.ToString(), "\\\\.\\pipe\\TvTest_Ctrl_BonPipe_" + p.Id.ToString());
                            cmdTvTest.SetConnectTimeOut(1000);
                            cmdTvTest.SendViewAppClose();
                        }
                        break;
                    }
                }
            }
            processID = -1;
            if (Settings.Instance.NwTvMode == true)
            {
                cmd.SendNwTVClose();
            }
        }

        // 外部プロセスのウィンドウを起動する
        private void WakeupWindow(int processID)
        {
            foreach (Process p in Process.GetProcesses())
            {
                if (p.Id == processID)
                {
                    if (IsIconic(p.MainWindowHandle))
                    {
                        ShowWindowAsync(p.MainWindowHandle, SW_RESTORE);
                    }

                    // メイン・ウィンドウを最前面に表示する
                    SetForegroundWindow(p.MainWindowHandle);
                    break;
                }
            }
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
