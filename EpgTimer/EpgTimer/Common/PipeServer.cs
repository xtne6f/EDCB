using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ComponentModel;
using System.Collections;
using System.Threading;
using System.Runtime.InteropServices;
using System.IO.Pipes;

namespace EpgTimer
{
    public class PipeServer
    {
        private Thread m_ServerThread = null;
        private AutoResetEvent m_PulseEvent = null;
        private bool m_StopFlag = false;

        ~PipeServer()
        {
            StopServer();
        }

        public bool StartServer(string strEventName, string strPipeName, Func<uint, byte[], Tuple<ErrCode, byte[], uint>> pfnCmdProc)
        {
            if (pfnCmdProc == null || strEventName.Length == 0 || strPipeName.Length == 0)
            {
                return false;
            }
            if (m_ServerThread != null)
            {
                return false;
            }

            m_StopFlag = false;
            m_PulseEvent = new AutoResetEvent(false);
            m_ServerThread = new Thread(new ThreadStart(() =>
            {
                using (EventWaitHandle eventConnect = new EventWaitHandle(false, EventResetMode.AutoReset, strEventName))
                using (NamedPipeServerStream pipe = new NamedPipeServerStream(
                           strPipeName.Substring(strPipeName.StartsWith("\\\\.\\pipe\\", StringComparison.OrdinalIgnoreCase) ? 9 : 0),
                           PipeDirection.InOut, 1, PipeTransmissionMode.Byte, PipeOptions.Asynchronous))
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
                            if (pipe.Read(bHead, 0, 8) == 8)
                            {
                                uint cmdParam = BitConverter.ToUInt32(bHead, 0);
                                byte[] cmdData = new byte[BitConverter.ToUInt32(bHead, 4)];
                                if (cmdData.Length == 0 || pipe.Read(cmdData, 0, cmdData.Length) == cmdData.Length)
                                {
                                    Tuple<ErrCode, byte[], uint> res = pfnCmdProc.Invoke(cmdParam, cmdData);
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
            }
        }


    }
}
