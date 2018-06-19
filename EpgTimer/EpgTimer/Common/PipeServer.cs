using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ComponentModel;
using System.Collections;
using System.Threading;
using System.IO.Pipes;
using System.Security.AccessControl;

namespace EpgTimer
{
    public class PipeServer : IDisposable
    {
        private Thread m_ServerThread = null;
        private AutoResetEvent m_StopEvent = new AutoResetEvent(false);

        public PipeServer(string eventName, string pipeName, Func<uint, byte[], Tuple<ErrCode, byte[], uint>> cmdProc)
        {
            var eventConnect = new EventWaitHandle(false, EventResetMode.AutoReset, eventName);
            string trustee = "NT Service\\EpgTimer Service";
            try
            {
                // "EpgTimer Service"のサービスセキュリティ識別子(Service-specific SID)に対するアクセス許可を追加する
                EventWaitHandleSecurity sec = eventConnect.GetAccessControl();
                sec.AddAccessRule(new EventWaitHandleAccessRule(trustee, EventWaitHandleRights.Synchronize, AccessControlType.Allow));
                eventConnect.SetAccessControl(sec);
            }
            catch
            {
                trustee = null;
            }
            var pipe = new NamedPipeServerStream(pipeName, PipeDirection.InOut, 1, PipeTransmissionMode.Byte, PipeOptions.Asynchronous, 1024, 1024,
                                                 null, System.IO.HandleInheritability.None, trustee == null ? 0 : PipeAccessRights.ChangePermissions);
            if (trustee != null)
            {
                try
                {
                    PipeSecurity sec = pipe.GetAccessControl();
                    sec.AddAccessRule(new PipeAccessRule(trustee, PipeAccessRights.ReadWrite, AccessControlType.Allow));
                    pipe.SetAccessControl(sec);
                }
                catch
                {
                }
            }

            m_ServerThread = new Thread(new ThreadStart(() =>
            {
                using (eventConnect)
                using (pipe)
                {
                    for (;;)
                    {
                        IAsyncResult ar = pipe.BeginWaitForConnection(null, null);
                        eventConnect.Set();
                        using (ar.AsyncWaitHandle)
                        {
                            if (WaitHandle.WaitAny(new WaitHandle[] { m_StopEvent, ar.AsyncWaitHandle }) != 1)
                            {
                                pipe.Dispose();
                                ar.AsyncWaitHandle.WaitOne();
                                break;
                            }
                            pipe.EndWaitForConnection(ar);
                        }
                        if (pipe.IsConnected)
                        {
                            byte[] bHead = new byte[8];
                            if (ReadAll(pipe, bHead, 0, 8) == 8)
                            {
                                uint cmdParam = BitConverter.ToUInt32(bHead, 0);
                                byte[] cmdData = new byte[BitConverter.ToUInt32(bHead, 4)];
                                if (ReadAll(pipe, cmdData, 0, cmdData.Length) == cmdData.Length)
                                {
                                    Tuple<ErrCode, byte[], uint> res = cmdProc.Invoke(cmdParam, cmdData);
                                    BitConverter.GetBytes((uint)res.Item1).CopyTo(bHead, 0);
                                    BitConverter.GetBytes(res.Item2 == null ? 0 : res.Item2.Length).CopyTo(bHead, 4);
                                    pipe.Write(bHead, 0, 8);
                                    if (res.Item2 != null && res.Item2.Length > 0)
                                    {
                                        pipe.Write(res.Item2, 0, res.Item2.Length);
                                    }
                                }
                            }
                            pipe.WaitForPipeDrain();
                            pipe.Disconnect();
                        }
                    }
                }
            }));
            m_ServerThread.Start();
        }

        public void Dispose()
        {
            if (m_ServerThread != null)
            {
                m_StopEvent.Set();
                m_ServerThread.Join();
                m_ServerThread = null;
                m_StopEvent.Dispose();
            }
        }

        private static int ReadAll(NamedPipeServerStream s, byte[] buffer, int offset, int size)
        {
            int n = 0;
            for (int m; n < size && (m = s.Read(buffer, offset + n, size - n)) > 0; n += m) ;
            return n;
        }

    }
}
