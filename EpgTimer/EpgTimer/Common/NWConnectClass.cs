using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

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

        private IPAddress connectedIP = null;
        private UInt32 connectedPort = 0;

        private CtrlCmdUtil cmd = null;

        public bool IsConnected
        {
            get
            {
                return connectFlag;
            }
        }

        public IPAddress ConnectedIP
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
                connectedIP = srvIP;
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

        private static int ReadAll(Stream s, byte[] buffer, int offset, int size)
        {
            int n = 0;
            for (int m; n < size && (m = s.Read(buffer, offset + n, size - n)) > 0; n += m) ;
            return n;
        }

        private void StartPolling(TcpClient client, IPAddress srvIP, uint srvPort, uint targetCount)
        {
            //巡回カウンタがtargetCountよりも大きくなる新しい通知を待ち受ける
            var w = new CtrlCmdWriter(new MemoryStream());
            w.Write((ushort)0);
            w.Write(targetCount);
            byte[] bHead = new byte[8 + w.Stream.Length];
            BitConverter.GetBytes((uint)CtrlCmd.CMD_EPG_SRV_GET_STATUS_NOTIFY2).CopyTo(bHead, 0);
            BitConverter.GetBytes((uint)w.Stream.Length).CopyTo(bHead, 4);
            w.Stream.Close();
            w.Stream.ToArray().CopyTo(bHead, 8);

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
            stream.Write(bHead, 0, bHead.Length);
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
                    if (readSize > 0 && ReadAll(stream, bHead, readSize, 8 - readSize) == 8 - readSize)
                    {
                        CMD_STREAM stCmd = new CMD_STREAM();
                        stCmd.uiParam = BitConverter.ToUInt32(bHead, 0);
                        stCmd.uiSize = BitConverter.ToUInt32(bHead, 4);
                        stCmd.bData = new byte[stCmd.uiSize];
                        if (ReadAll(stream, stCmd.bData, 0, stCmd.bData.Length) == stCmd.bData.Length && stCmd.uiParam == (uint)ErrCode.CMD_SUCCESS)
                        {
                            //通常の通知コマンドに変換
                            stCmd.uiParam = (uint)CtrlCmd.CMD_TIMER_GUI_SRV_STATUS_NOTIFY2;
                            cmdProc.Invoke(stCmd, new CMD_STREAM());
                            targetCount = stCmd.uiSize;
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

            NetworkStream stream = client.GetStream();

            CMD_STREAM stCmd = new CMD_STREAM();
            CMD_STREAM stRes = new CMD_STREAM();
            //コマンド受信
            if (cmdProc != null)
            {
                byte[] bHead = new byte[8];

                if (ReadAll(stream, bHead, 0, 8) == 8)
                {
                    stCmd.uiParam = BitConverter.ToUInt32(bHead, 0);
                    stCmd.uiSize = BitConverter.ToUInt32(bHead, 4);
                    stCmd.bData = new byte[stCmd.uiSize];
                    if (ReadAll(stream, stCmd.bData, 0, stCmd.bData.Length) == stCmd.bData.Length)
                    {
                        cmdProc.Invoke(stCmd, stRes);
                        BitConverter.GetBytes(stRes.uiParam).CopyTo(bHead, 0);
                        BitConverter.GetBytes(stRes.uiSize).CopyTo(bHead, 4);
                        stream.Write(bHead, 0, 8);
                        if (stRes.uiSize > 0)
                        {
                            stream.Write(stRes.bData, 0, (int)stRes.uiSize);
                        }
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
