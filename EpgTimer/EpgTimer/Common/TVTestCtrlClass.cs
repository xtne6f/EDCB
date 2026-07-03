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

        public bool SetLiveCh(ushort ONID, ushort TSID, ushort SID)
        {
            try
            {
                if (Settings.Instance.TvTestExe.Length == 0)
                {
                    MessageBox.Show("TVTest.exeのパスが設定されていません");
                    return false;
                }
                OpenTVTest(Settings.Instance.TvTestOpenWait, Settings.Instance.NwTvMode == false);
                var cmdTvTest = new CtrlCmdUtil();
                cmdTvTest.SetPipeSetting("Global\\" + processType + "_Ctrl_BonConnect_" + process.Id, processType + "_Ctrl_BonPipe_" + process.Id);
                cmdTvTest.SetConnectTimeOut(1000);

                if (Settings.Instance.NwTvMode == true)
                {
                    SetChInfo chInfo = new SetChInfo();
                    chInfo.useSID = 1;
                    chInfo.useBonCh = 0;
                    chInfo.ONID = ONID;
                    chInfo.TSID = TSID;
                    chInfo.SID = SID;

                    uint nwMode = 0;
                    string nwBonDriver = "BonDriver_UDP.dll";
                    if (Settings.Instance.NwTvModeUDP == true)
                    {
                        nwMode += 1;
                    }
                    if (Settings.Instance.NwTvModeTCP == true)
                    {
                        nwMode += 2;
                        nwBonDriver = Settings.Instance.NwTvModePipe ? "BonDriver_NetworkPipe.dll" : "BonDriver_TCP.dll";
                    }
                    
                    if (CommonManager.CreateSrvCtrl().SendNwTVMode(nwMode) == ErrCode.CMD_SUCCESS)
                    {
                        if (CommonManager.CreateSrvCtrl().SendNwTVSetCh(chInfo) == ErrCode.CMD_SUCCESS)
                        {
                            string val = "";
                            for (int i = 0; i < 10; i++)
                            {
                                if (cmdTvTest.SendViewGetBonDrivere(ref val) != ErrCode.CMD_SUCCESS)
                                {
                                    System.Threading.Thread.Sleep(1000);
                                    continue;
                                }
                                if (val.Equals(nwBonDriver, StringComparison.OrdinalIgnoreCase) == false)
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
                    ulong key = CommonManager.Create64Key(ONID, TSID, SID);
                    TvTestChChgInfo chInfo = new TvTestChChgInfo();
                    ErrCode err = CommonManager.CreateSrvCtrl().SendGetChgChTVTest(key, ref chInfo);
                    if (err == ErrCode.CMD_SUCCESS)
                    {
                        string val = "";
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
                            if (val.Equals(chInfo.bonDriver, StringComparison.OrdinalIgnoreCase) == false)
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
                    else
                    {
                        MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "指定サービスを受信できるBonDriverが設定されていません。");
                        return false;
                    }
                }

                WakeupWindow(process.MainWindowHandle);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
            return true;
        }

        public bool StartStreamingPlay(string filePath, uint reserveID)
        {
            try
            {
                if (Settings.Instance.TvTestExe.Length == 0)
                {
                    MessageBox.Show("TVTest.exeのパスが設定されていません");
                    return false;
                }
                if (CommonManager.Instance.NWMode == false && (Settings.Instance.NwTvModeTCP == false || Settings.Instance.NwTvModePipe == false))
                {
                    if (IniFileHandler.GetPrivateProfileInt("SET", "EnableTCPSrv", 0, SettingPath.TimerSrvIniPath) == 0)
                    {
                        MessageBox.Show("動作設定でネットワーク接続を許可する必要があります。");
                        return false;
                    }
                }
                var sendInfo = new TVTestStreamingInfo();
                if (filePath == null)
                {
                    var playInfo = new NWPlayTimeShiftInfo();
                    ErrCode err = CommonManager.CreateSrvCtrl().SendNwTimeShiftOpen(reserveID, ref playInfo);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "まだ録画が開始されていません。");
                        return false;
                    }
                    sendInfo.ctrlID = playInfo.ctrlID;
                    sendInfo.filePath = playInfo.filePath;
                }
                else
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendNwPlayOpen(filePath, ref sendInfo.ctrlID);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "ファイルを開けませんでした。");
                        return false;
                    }
                }

                OpenTVTest(1000, false);
                var cmdTvTest = new CtrlCmdUtil();
                cmdTvTest.SetPipeSetting("Global\\TvTest_Ctrl_BonConnect_" + process.Id, "TvTest_Ctrl_BonPipe_" + process.Id);
                cmdTvTest.SetConnectTimeOut(1000);

                sendInfo.enableMode = 1;
                if (Settings.Instance.NwTvModeTCP && Settings.Instance.NwTvModePipe)
                {
                    // ネットワーク接続を使わない
                    sendInfo.serverIP = 1;
                    sendInfo.serverPort = 0;
                }
                else if (CommonManager.Instance.NWMode == false)
                {
                    sendInfo.serverIP = 0x7F000001;
                    // 原作はここで自ホスト名を取得して解決したアドレスを格納している。(ないとは思うが)不具合があれば戻すこと
                    sendInfo.serverPort = (uint)IniFileHandler.GetPrivateProfileInt("SET", "TCPPort", 4510, SettingPath.TimerSrvIniPath);
                }
                else
                {
                    sendInfo.serverIP = 0x7F000001;
                    IPAddress srvIP = CommonManager.Instance.NWConnectedIP;
                    if (srvIP != null && srvIP.GetAddressBytes().Length == 4)
                    {
                        byte[] oct = srvIP.GetAddressBytes();
                        sendInfo.serverIP = (uint)oct[0] << 24 | (uint)oct[1] << 16 | (uint)oct[2] << 8 | oct[3];
                    }
                    sendInfo.serverPort = CommonManager.Instance.NWConnectedPort;
                }
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
                MessageBox.Show(ex.ToString());
            }
            return true;
        }

        private void OpenTVTest(int openWait, bool acceptViewApp)
        {
            if (process == null || process.HasExited || (acceptViewApp == false && processType == "View"))
            {
                if (process != null)
                {
                    process.Dispose();
                }
                processType = "TvTest";
                process = FindTVTestProcess(processType);
                if (process == null && acceptViewApp)
                {
                    processType = "View";
                    process = FindTVTestProcess(processType);
                }
                if (process == null)
                {
                    // EpgTimerPlugInと仮定
                    processType = "TvTest";
                    process = Process.Start(Settings.Instance.TvTestExe, Settings.Instance.TvTestCmd);
                    if (acceptViewApp)
                    {
                        for (int i = 0; i < 100; i++)
                        {
                            var cmdTvTest = new CtrlCmdUtil();
                            cmdTvTest.SetPipeSetting("Global\\View_Ctrl_BonConnect_" + process.Id, "View_Ctrl_BonPipe_" + process.Id);
                            if (cmdTvTest.PipeExists())
                            {
                                // Viewアプリ(EdcbPlugIn)と判断
                                processType = "View";
                                break;
                            }
                            cmdTvTest.SetPipeSetting("Global\\TvTest_Ctrl_BonConnect_" + process.Id, "TvTest_Ctrl_BonPipe_" + process.Id);
                            if (cmdTvTest.PipeExists())
                            {
                                break;
                            }
                            System.Threading.Thread.Sleep(100);
                            openWait -= 100;
                        }
                    }
                    if (openWait > 0)
                    {
                        System.Threading.Thread.Sleep(openWait);
                    }
                }
            }
        }

        private static Process FindTVTestProcess(string type)
        {
            Process[] processes = Process.GetProcesses();
            foreach (Process p in processes)
            {
                // 原作と異なりプロセス名ではなく接続待機用イベントの有無で判断するので注意
                var cmdTvTest = new CtrlCmdUtil();
                cmdTvTest.SetPipeSetting("Global\\" + type + "_Ctrl_BonConnect_" + p.Id, type + "_Ctrl_BonPipe_" + p.Id);
                if (cmdTvTest.PipeExists())
                {
                    if (type == "View")
                    {
                        // TVTestではなさそうなViewアプリは除外する(※EpgDataCap_BonもViewアプリ)
                        if (p.ProcessName.Equals(System.IO.Path.GetFileNameWithoutExtension(Settings.Instance.TvTestExe),
                                                 StringComparison.OrdinalIgnoreCase) == false)
                        {
                            continue;
                        }
                        // 識別用IDが設定されたViewアプリは除外する
                        cmdTvTest.SetConnectTimeOut(1000);
                        int id = -1;
                        if (cmdTvTest.SendViewGetID(ref id) != ErrCode.CMD_SUCCESS || id >= 0)
                        {
                            continue;
                        }
                    }
                    foreach (Process pp in processes.Where(pp => pp != p)) { pp.Dispose(); }
                    return p;
                }
            }
            foreach (Process p in processes) { p.Dispose(); }
            return null;
        }

        public void CloseTVTest()
        {
            if (process != null)
            {
                if (process.HasExited == false)
                {
                    var cmdTvTest = new CtrlCmdUtil();
                    cmdTvTest.SetPipeSetting("Global\\" + processType + "_Ctrl_BonConnect_" + process.Id, processType + "_Ctrl_BonPipe_" + process.Id);
                    cmdTvTest.SetConnectTimeOut(1000);
                    cmdTvTest.SendViewAppClose();
                }
                process.Dispose();
            }
            process = null;
            if (Settings.Instance.NwTvMode == true)
            {
                CommonManager.CreateSrvCtrl().SendNwTVClose();
            }
        }

        // 外部プロセスのウィンドウを起動する
        private static void WakeupWindow(IntPtr windowHandle)
        {
            if (NativeMethods.IsIconic(windowHandle))
            {
                NativeMethods.ShowWindowAsync(windowHandle, SW_RESTORE);
            }
            // メイン・ウィンドウを最前面に表示する
            CommonUtil.SetForegroundWindow(windowHandle);
        }

        // ShowWindowAsync関数のパラメータに渡す定義値
        private const int SW_RESTORE = 9;  // 画面を元の大きさに戻す

        private static class NativeMethods
        {
            [DllImport("user32.dll")]
            public static extern bool ShowWindowAsync(IntPtr hWnd, int nCmdShow);

            [DllImport("user32.dll")]
            public static extern bool IsIconic(IntPtr hWnd);
        }
    }
}
