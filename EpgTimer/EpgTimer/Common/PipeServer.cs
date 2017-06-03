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

        public bool StartServer(string strEventName, string strPipeName, Action<CMD_STREAM, CMD_STREAM> pfnCmdProc)
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
                                CMD_STREAM stCmd = new CMD_STREAM();
                                stCmd.uiParam = BitConverter.ToUInt32(bHead, 0);
                                stCmd.uiSize = BitConverter.ToUInt32(bHead, 4);
                                stCmd.bData = stCmd.uiSize == 0 ? null : new byte[stCmd.uiSize];
                                if (stCmd.uiSize == 0 || pipe.Read(stCmd.bData, 0, stCmd.bData.Length) == stCmd.bData.Length)
                                {
                                    CMD_STREAM stRes = new CMD_STREAM();
                                    pfnCmdProc.Invoke(stCmd, stRes);
                                    if (stRes.uiParam == (uint)ErrCode.CMD_NEXT)
                                    {
                                        // Emun用の繰り返しは対応しない
                                        throw new InvalidOperationException();
                                    }
                                    else if (stRes.uiParam != (uint)ErrCode.CMD_NO_RES)
                                    {
                                        BitConverter.GetBytes(stRes.uiParam).CopyTo(bHead, 0);
                                        BitConverter.GetBytes(stRes.uiSize).CopyTo(bHead, 4);
                                        pipe.Write(bHead, 0, 8);
                                        if (stRes.uiSize != 0 && stRes.bData != null && stRes.bData.Length >= stRes.uiSize)
                                        {
                                            pipe.Write(stRes.bData, 0, (int)stRes.uiSize);
                                        }
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
