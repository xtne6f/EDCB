using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.IO;
using System.Threading;

namespace EpgTimer
{
    public sealed class NWConnect : IDisposable
    {
        private Thread workerThread = null;
        private AutoResetEvent stopEvent = null;

        public static bool SendMagicPacket(byte[] physicalAddress, out int ifCount, out int ifTotal)
        {
            byte[] dgram = new byte[6 + physicalAddress.Length * 16];
            for (int i = 0; i < 6; i++)
            {
                dgram[i] = 0xff;
            }
            for (int i = 0; i < 16; i++)
            {
                physicalAddress.CopyTo(dgram, 6 + physicalAddress.Length * i);
            }

            ifCount = 0;
            ifTotal = 0;
            NetworkInterface[] adapters;
            try
            {
                adapters = NetworkInterface.GetAllNetworkInterfaces();
            }
            catch (NetworkInformationException)
            {
                return false;
            }

            foreach (NetworkInterface adapter in adapters)
            {
                if (adapter.NetworkInterfaceType != NetworkInterfaceType.Loopback &&
                    adapter.OperationalStatus == OperationalStatus.Up)
                {
                    foreach (IPAddress addr in adapter.GetIPProperties().UnicastAddresses.Select(a => a.Address))
                    {
                        if (addr.AddressFamily == AddressFamily.InterNetwork)
                        {
                            //プライベートネットワークのみ
                            byte[] addrBytes = addr.GetAddressBytes();
                            if ((addrBytes[0] == 192 && addrBytes[1] == 168) ||
                                (addrBytes[0] == 172 && addrBytes[1] >> 4 == 1) ||
                                (addrBytes[0] == 10) ||
                                (addrBytes[0] == 169 && addrBytes[1] == 254))
                            {
                                //Winsockのsendto()の説明によると"If the address pointed to by the to parameter contains the INADDR_BROADCAST address and
                                //intended port, then the broadcast will be sent out on all interfaces to that port."だが恐らく嘘で、実際には1つのI/Fから送信される。
                                //送信元I/Fを限定して個別に送信させるため、送信先をブロードキャストアドレスにするか、明示的にbindして送信元を示す必要がある。
                                //送信元Portはエフェメラルでよい
                                try
                                {
                                    using (var client = new UdpClient(new IPEndPoint(addr, 0)))
                                    {
                                        client.EnableBroadcast = true;
                                        //送信先Portは7か9が多い
                                        client.Send(dgram, dgram.Length, new IPEndPoint(IPAddress.Broadcast, 9));
                                    }
                                    ifCount++;
                                }
                                catch (SocketException) { }
                                ifTotal++;
                            }
                        }
                        else if (addr.AddressFamily == AddressFamily.InterNetworkV6 && addr.IsIPv6LinkLocal)
                        {
                            //同一リンク内
                            try
                            {
                                using (var client = new UdpClient(new IPEndPoint(addr, 0)))
                                {
                                    client.Send(dgram, dgram.Length, new IPEndPoint(IPAddress.Parse("ff02::1"), 9));
                                }
                                ifCount++;
                            }
                            catch (SocketException) { }
                            ifTotal++;
                        }
                    }
                }
            }
            return true;
        }

        public bool ConnectServer(IPAddress srvIP, uint srvPort, uint waitPort, Func<uint, byte[], Tuple<ErrCode, byte[], uint>> cmdProc)
        {
            StopWorker();

            if (srvIP != null && waitPort != 0)
            {
                //待ち受け方式(互換のために残しているだけ)
                var server = new TcpListener(srvIP.AddressFamily == AddressFamily.InterNetworkV6 ? IPAddress.IPv6Any : IPAddress.Any, (int)waitPort);
                server.Start();
                stopEvent = new AutoResetEvent(false);
                workerThread = new Thread(() =>
                {
                    for (;;)
                    {
                        TcpClient client;
                        IAsyncResult ar = server.BeginAcceptTcpClient(null, null);
                        using (ar.AsyncWaitHandle)
                        {
                            if (WaitHandle.WaitAny(new WaitHandle[] { stopEvent, ar.AsyncWaitHandle }) != 1)
                            {
                                server.Stop();
                                ar.AsyncWaitHandle.WaitOne();
                                break;
                            }
                            client = server.EndAcceptTcpClient(ar);
                        }
                        using (client)
                        using (NetworkStream stream = client.GetStream())
                        {
                            byte[] bHead = new byte[8];
                            if (ReadAll(stream, bHead, 0, 8) == 8)
                            {
                                uint cmdParam = BitConverter.ToUInt32(bHead, 0);
                                byte[] cmdData = new byte[BitConverter.ToUInt32(bHead, 4)];
                                if (ReadAll(stream, cmdData, 0, cmdData.Length) == cmdData.Length)
                                {
                                    Tuple<ErrCode, byte[], uint> res = cmdProc(cmdParam, cmdData);
                                    BitConverter.GetBytes((uint)res.Item1).CopyTo(bHead, 0);
                                    BitConverter.GetBytes(res.Item2 == null ? 0 : res.Item2.Length).CopyTo(bHead, 4);
                                    stream.Write(bHead, 0, 8);
                                    if (res.Item2 != null && res.Item2.Length > 0)
                                    {
                                        stream.Write(res.Item2, 0, res.Item2.Length);
                                    }
                                }
                            }
                        }
                    }
                    server.Stop();
                });
                workerThread.Start();
            }

            var cmd = new CtrlCmdUtil();

            cmd.SetSendMode(true);

            cmd.SetNWSetting(srvIP, srvPort);

            var status = new NotifySrvInfo();
            if (waitPort == 0 && cmd.SendGetNotifySrvStatus(ref status) != ErrCode.CMD_SUCCESS ||
                waitPort != 0 && cmd.SendRegistTCP(waitPort) != ErrCode.CMD_SUCCESS)
            {
                //サーバが存在しないかロングポーリングに未対応
                StopWorker();
                return false;
            }
            else if (waitPort == 0)
            {
                //ロングポーリング方式
                stopEvent = new AutoResetEvent(false);
                workerThread = new Thread(() =>
                {
                    uint targetCount = 0;
                    for (;;)
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

                        using (var client = new TcpClient(srvIP.AddressFamily))
                        {
                            try
                            {
                                client.Connect(srvIP, (int)srvPort);
                            }
                            catch (SocketException ex)
                            {
                                System.Diagnostics.Trace.WriteLine(ex);
                                //少し待つ
                                if (stopEvent.WaitOne(5000)) break;
                                continue;
                            }
                            using (NetworkStream stream = client.GetStream())
                            {
                                stream.Write(bHead, 0, bHead.Length);
                                int readSize;
                                IAsyncResult ar = stream.BeginRead(bHead, 0, 8, null, null);
                                using (ar.AsyncWaitHandle)
                                {
                                    if (WaitHandle.WaitAny(new WaitHandle[] { stopEvent, ar.AsyncWaitHandle }) != 1)
                                    {
                                        stream.Dispose();
                                        ar.AsyncWaitHandle.WaitOne();
                                        break;
                                    }
                                    try
                                    {
                                        readSize = stream.EndRead(ar);
                                    }
                                    catch (IOException ex)
                                    {
                                        System.Diagnostics.Trace.WriteLine(ex);
                                        //少し待つ
                                        if (stopEvent.WaitOne(5000)) break;
                                        continue;
                                    }
                                }
                                if (readSize > 0 && ReadAll(stream, bHead, readSize, 8 - readSize) == 8 - readSize)
                                {
                                    uint cmdParam = BitConverter.ToUInt32(bHead, 0);
                                    byte[] cmdData = new byte[BitConverter.ToUInt32(bHead, 4)];
                                    if (ReadAll(stream, cmdData, 0, cmdData.Length) == cmdData.Length && cmdParam == (uint)ErrCode.CMD_SUCCESS)
                                    {
                                        //通常の通知コマンドに変換
                                        targetCount = cmdProc((uint)CtrlCmd.CMD_TIMER_GUI_SRV_STATUS_NOTIFY2, cmdData).Item3;
                                    }
                                }
                            }
                        }
                    }
                });
                workerThread.Start();
            }
            return true;
        }

        private static int ReadAll(Stream s, byte[] buffer, int offset, int size)
        {
            int n = 0;
            for (int m; n < size && (m = s.Read(buffer, offset + n, size - n)) > 0; n += m) ;
            return n;
        }

        public void Dispose()
        {
            StopWorker();
        }

        private void StopWorker()
        {
            if (workerThread != null)
            {
                stopEvent.Set();
                workerThread.Join();
                workerThread = null;
                stopEvent.Dispose();
            }
        }
    }
}
