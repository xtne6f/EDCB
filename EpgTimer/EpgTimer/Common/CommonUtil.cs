using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Threading;
using System.Runtime.InteropServices;
using System.Linq.Expressions;
using System.Windows.Media;

namespace EpgTimer
{
    class CommonUtil
    {
        [DllImport("user32.dll")]
        static extern bool GetLastInputInfo(ref LASTINPUTINFO plii);

        [DllImport("kernel32.dll")]
        static extern uint GetTickCount();

        // Struct we'll need to pass to the function
        internal struct LASTINPUTINFO
        {
            public uint cbSize;
            public uint dwTime;
        }

        public static int NumBits(long bits)
        {
            bits = (bits & 0x55555555) + (bits >> 1 & 0x55555555);
            bits = (bits & 0x33333333) + (bits >> 2 & 0x33333333);
            bits = (bits & 0x0f0f0f0f) + (bits >> 4 & 0x0f0f0f0f);
            bits = (bits & 0x00ff00ff) + (bits >> 8 & 0x00ff00ff);
            return (int)((bits & 0x0000ffff) + (bits >> 16 & 0x0000ffff));
        }
        
        public static int GetIdleTimeSec()
        {
            // The number of ticks that passed since last input
            uint IdleTicks = 0;

            // Set the struct
            LASTINPUTINFO LastInputInfo = new LASTINPUTINFO();
            LastInputInfo.cbSize = (uint)Marshal.SizeOf(LastInputInfo);
            LastInputInfo.dwTime = 0;

            // If we have a value from the function
            if (GetLastInputInfo(ref LastInputInfo))
            {
                // Number of idle ticks = system uptime ticks - number of ticks at last input
                IdleTicks = unchecked(GetTickCount() - LastInputInfo.dwTime);
            }
            return (int)(IdleTicks / 1000);
        }

        /// <summary>�����o����Ԃ��B</summary>
        public static string NameOf<T>(Expression<Func<T>> e)
        {
            var member = (MemberExpression)e.Body;
            return member.Member.Name;
        }

        /// <summary>���X�g�ɂ��ĕԂ��B(return new List&lt;T&gt; { item })</summary>
        public static List<T> ToList<T>(T item)
        {
            return new List<T> { item };
        }

        /// <summary>�񓯊��̃��b�Z�[�W�{�b�N�X��\��</summary>
        public static void DispatcherMsgBoxShow(string message, string caption = "", MessageBoxButton button = MessageBoxButton.OK, MessageBoxImage icon = MessageBoxImage.None)
        {
            Dispatcher.CurrentDispatcher.BeginInvoke(new Action(() => MessageBox.Show(message, caption, button, icon)));
        }

        /// <summary>�E�B���h�E������Ύ擾����</summary>
        public static Window GetTopWindow(Visual obj)
        {
            if (obj == null) return null;
            var topWindow = PresentationSource.FromVisual(obj);
            return topWindow == null ? null : topWindow.RootVisual as Window;
        }

        /// <summary>�������𐧌����A������ꍇ�͏ȗ��L����t�^����</summary>
        public static string LimitLenString(string s, int max_len, string tag = "...")
        {
            if (string.IsNullOrEmpty(s) == false && s.Length > max_len)
            {
                tag = tag ?? "";
                max_len = Math.Max(max_len, 0);
                s = s.Substring(0, Math.Max(0, max_len - tag.Length)) + tag.Substring(0, Math.Min(max_len, tag.Length));
            }
            return s;
        }

    }
}
