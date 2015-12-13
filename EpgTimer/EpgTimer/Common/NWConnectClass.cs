using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

using System.Net;
using System.Net.Sockets;
using System.IO;
using System.Threading;

namespace EpgTimer
{
    public class NWConnect
    {
        private Action<CMD_STREAM, CMD_STREAM> cmdProc = null;

        private bool connectFlag;
        private TcpListener server = null;
        private TcpClient pollingClient = null;

        private String connectedIP;
        private UInt32 connectedPort = 0;

        private CtrlCmdUtil cmd = null;

        public bool IsConnected
        {
            get
            {
                return connectFlag;
            }
        }

        public String ConnectedIP
        {
            get
            {
                return connectedIP;
            }
        }

        public UInt32 ConnectedPort
        {
            get
            {
                return connectedPort;
            }
        }

        public NWConnect(CtrlCmdUtil ctrlCmd)
        {
            connectFlag = false;
            cmd = ctrlCmd;
        }

        public static void SendMagicPacket(byte[] physicalAddress)
        {
            SendMagicPacket(IPAddress.Broadcast, physicalAddress);
        }

        private static void SendMagicPacket(IPAddress broad, byte[] physicalAddress)
        {
            MemoryStream stream = new MemoryStream();
            BinaryWriter writer = new BinaryWriter(stream);
            for (int i = 0; i < 6; i++)
            {
                writer.Write((byte)0xff);
            }
            for (int i = 0; i < 16; i++)
            {
                writer.Write(physicalAddress);
            }

            UdpClient client = new UdpClient();
            client.EnableBroadcast = true;
            client.Send(stream.ToArray(), (int)stream.Position, new IPEndPoint(broad, 0));
        }

        public bool ConnectServer(IPAddress srvIP, UInt32 srvPort, UInt32 waitPort, Action<CMD_STREAM, CMD_STREAM> pfnCmdProc)
        {
            connectFlag = false;

            cmdProc = pfnCmdProc;
            StartTCPServer(waitPort);
            pollingClient = null;

            cmd.SetSendMode(true);

            cmd.SetNWSetting(srvIP.ToString(), srvPort);

            var status = new NotifySrvInfo();
            if (waitPort == 0 && cmd.SendGetNotifySrvStatus(ref status) != ErrCode.CMD_SUCCESS ||
                waitPort != 0 && cmd.SendRegistTCP(waitPort) != ErrCode.CMD_SUCCESS)
            {
                //サーバが存在しないかロングポーリングに未対応
                return false;
            }
            else
            {
                connectFlag = true;
                connectedIP = srvIP.ToString();
                connectedPort = srvPort;
                if (waitPort == 0)
                {
                    pollingClient = new TcpClient();
                    StartPolling(pollingClient, srvIP, srvPort, 0);
                }
                return true;
            }
        }

        private bool StartTCPServer(UInt32 port)
        {
            if (server != null)
            {
                return true;
            }
            if (port != 0)
            {
                server = new TcpListener(IPAddress.Any, (int)port);
                server.Start();
                server.BeginAcceptTcpClient(new AsyncCallback(DoAcceptTcpClientCallback), server);
            }

            return true;
        }

        private void StartPolling(TcpClient client, IPAddress srvIP, uint srvPort, uint targetCount)
        {
            //巡回カウンタがtargetCountよりも大きくなる新しい通知を待ち受ける
            var w = new CtrlCmdWriter(new MemoryStream());
            w.Write((ushort)0);
            w.Write(targetCount);
            byte[] bData = w.Stream.ToArray();
            byte[] bHead = new byte[8];
            Array.Copy(BitConverter.GetBytes((uint)CtrlCmd.CMD_EPG_SRV_GET_STATUS_NOTIFY2), 0, bHead, 0, 4);
            Array.Copy(BitConverter.GetBytes(bData.Length), 0, bHead, 4, 4);

            try
            {
                client.Connect(srvIP, (int)srvPort);
            }
            catch (SocketException ex)
            {
                System.Diagnostics.Trace.WriteLine(ex);
                Interlocked.CompareExchange(ref pollingClient, null, client);
                return;
            }
            NetworkStream stream = client.GetStream();
            stream.Write(bHead, 0, 8);
            stream.Write(bData, 0, bData.Length);
            stream.BeginRead(bHead, 0, 8, (IAsyncResult ar) =>
            {
                using (client)
                {
                    int readSize = 0;
                    try
                    {
                        readSize = stream.EndRead(ar);
                    }
                    catch (IOException ex)
                    {
                        System.Diagnostics.Trace.WriteLine(ex);
                    }
                    if (readSize == 8)
                    {
                        CMD_STREAM stCmd = new CMD_STREAM();
                        stCmd.uiParam = BitConverter.ToUInt32(bHead, 0);
                        stCmd.uiSize = BitConverter.ToUInt32(bHead, 4);
                        if (stCmd.uiSize > 0)
                        {
                            stCmd.bData = new byte[stCmd.uiSize];
                            for (stCmd.uiSize = 0; stCmd.uiSize != stCmd.bData.Length; stCmd.uiSize += (uint)readSize)
                            {
                                readSize = stream.Read(stCmd.bData, (int)stCmd.uiSize, stCmd.bData.Length - (int)stCmd.uiSize);
                                if (readSize == 0) break;
                            }
                            if (stCmd.uiSize == stCmd.bData.Length && stCmd.uiParam == (uint)ErrCode.CMD_SUCCESS)
                            {
                                //通常の通知コマンドに変換
                                stCmd.uiParam = (uint)CtrlCmd.CMD_TIMER_GUI_SRV_STATUS_NOTIFY2;
                                cmdProc.Invoke(stCmd, new CMD_STREAM());
                                targetCount = stCmd.uiSize;
                            }
                        }
                    }
                }
                //pollingClientが置きかわっていなければ引き続き待ち受ける
                var nextClient = new TcpClient();
                if (Interlocked.CompareExchange(ref pollingClient, nextClient, client) == client)
                {
                    StartPolling(nextClient, srvIP, srvPort, targetCount);
                }
            }, null);
        }

        public void DoAcceptTcpClientCallback(IAsyncResult ar)
        {
            TcpListener listener = (TcpListener)ar.AsyncState;

            TcpClient client = listener.EndAcceptTcpClient(ar);
            client.ReceiveBufferSize = 1024 * 1024;

            NetworkStream stream = client.GetStream();

            CMD_STREAM stCmd = new CMD_STREAM();
            CMD_STREAM stRes = new CMD_STREAM();
            //コマンド受信
            if (cmdProc != null)
            {
                byte[] bHead = new byte[8];

                if (stream.Read(bHead, 0, bHead.Length) == 8)
                {
                    stCmd.uiParam = BitConverter.ToUInt32(bHead, 0);
                    stCmd.uiSize = BitConverter.ToUInt32(bHead, 4);
                    if (stCmd.uiSize > 0)
                    {
                        stCmd.bData = new Byte[stCmd.uiSize];
                    }
                    int readSize = 0;
                    while (readSize < stCmd.uiSize)
                    {
                        readSize += stream.Read(stCmd.bData, readSize, (int)stCmd.uiSize);
                    }
                    cmdProc.Invoke(stCmd, stRes);

                    Array.Copy(BitConverter.GetBytes(stRes.uiParam), 0, bHead, 0, sizeof(uint));
                    Array.Copy(BitConverter.GetBytes(stRes.uiSize), 0, bHead, 4, sizeof(uint));
                    stream.Write(bHead, 0, 8);
                    if (stRes.uiSize > 0)
                    {
                        stream.Write(stRes.bData, 0, (int)stRes.uiSize);
                    }
                }
            }
            else
            {
                stRes.uiSize = 0;
                stRes.uiParam = 1;
            }
            stream.Dispose();
            client.Client.Close();

            server.BeginAcceptTcpClient(new AsyncCallback(DoAcceptTcpClientCallback), server);
        }
    
    }
}
