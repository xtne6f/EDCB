using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ComponentModel;
using System.Collections;
using System.Threading;
using System.IO.Pipes;

namespace EpgTimer
{
    public class PipeServer
    {
        private Thread m_ServerThread = null;
        private AutoResetEvent m_PulseEvent = null;
        private bool m_StopFlag = false;

        public bool StartServer(string eventName, string pipeName, Func<uint, byte[], Tuple<ErrCode, byte[], uint>> cmdProc)
        {
            if (m_ServerThread != null)
            {
                return false;
            }

            m_StopFlag = false;
            m_PulseEvent = new AutoResetEvent(false);
            var eventConnect = new EventWaitHandle(false, EventResetMode.AutoReset, eventName);
            var pipe = new NamedPipeServerStream(pipeName, PipeDirection.InOut, 1, PipeTransmissionMode.Byte, PipeOptions.Asynchronous);

            m_ServerThread = new Thread(new ThreadStart(() =>
            {
                using (eventConnect)
                using (pipe)
                {
                    while (m_StopFlag == false)
                    {
                        pipe.BeginWaitForConnection(asyncResult =>
                        {
                            try
                            {
                                pipe.EndWaitForConnection(asyncResult);
                                m_PulseEvent.Set();
                            }
                            catch (ObjectDisposedException)
                            {
                            }
                        }, null);
                        eventConnect.Set();
                        m_PulseEvent.WaitOne();
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

            return true;
        }

        public void StopServer()
        {
            if (m_ServerThread != null)
            {
                m_StopFlag = true;
                m_PulseEvent.Set();
                m_ServerThread.Join();
                m_ServerThread = null;
                m_PulseEvent.Dispose();
                m_PulseEvent = null;
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
