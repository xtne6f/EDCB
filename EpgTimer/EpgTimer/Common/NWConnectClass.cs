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
        private CMD_CALLBACK_PROC cmdProc = null;
        private object cmdParam = null;

        private bool connectFlag;
        private UInt32 serverPort;
        private TcpListener server = null;

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

        public bool ConnectServer(String srvIP, UInt32 srvPort, UInt32 waitPort, CMD_CALLBACK_PROC pfnCmdProc, object pParam)
        {
            if (srvIP.Length == 0)
            {
                return false;
            }
            connectFlag = false;

            cmdProc = pfnCmdProc;
            cmdParam = pParam;
            StartTCPServer(waitPort);

            cmd.SetSendMode(true);

            String wIPAddress = srvIP;
            IPAddress[] IpAddress = Dns.GetHostAddresses(srvIP);

            foreach (IPAddress wIP in IpAddress)
            {
                if (!wIP.IsIPv6LinkLocal)
                {
                    wIPAddress = wIP.ToString();
                    break;
                }
            }

            cmd.SetNWSetting(wIPAddress, srvPort);

            //cmd.SetNWSetting(srvIP, srvPort);
            if (cmd.SendRegistTCP(waitPort) != 1)
            {
                return false;
            }
            else
            {
                connectFlag = true;
                connectedIP = srvIP;
                connectedPort = srvPort;
                return true;
            }
        }

        private bool StartTCPServer(UInt32 port)
        {
            if (serverPort == port && server != null)
            {
                return true;
            }
            if (server != null)
            {
                server.Stop();
                server = null;
            }
            serverPort = port;
            server = new TcpListener(IPAddress.Any, (int)port);
            server.Start();
            server.BeginAcceptTcpClient(new AsyncCallback(DoAcceptTcpClientCallback), server);

            return true;
        }

        private bool StopTCPServer()
        {
            if (server != null)
            {
                server.Stop();
                server = null;
            } 
            return true;
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
                    cmdProc.Invoke(cmdParam, stCmd, ref stRes);

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
