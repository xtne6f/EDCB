using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using System.Windows.Media;

namespace EpgTimer
{
    public class ListViewHorizontalMouseScroller
    {
        private HwndSource hwndSource;
        private HwndSourceHook messageHook;

        public void OnMouseEnter(ListView listView, bool scrollAuto, double scrollSize)
        {
            OnMouseLeave();
            if (scrollAuto || scrollSize != 0)
            {
                hwndSource = PresentationSource.FromVisual(listView) as HwndSource;
                if (hwndSource != null)
                {
                    messageHook = (IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled) =>
                    {
                        const int WM_MOUSEHWHEEL = 0x020E;
                        if (msg == WM_MOUSEHWHEEL)
                        {
                            double delta = (short)((wParam.ToInt64() >> 16) & 0xFFFF);
                            if (delta != 0)
                            {
                                //負のとき左方向
                                delta = scrollAuto ? delta : scrollSize * (delta < 0 ? -1 : 1);
                                for (int depth = 1; depth <= 3; depth++)
                                {
                                    //なるべく浅いところにあるScrollViewerを探す
                                    ScrollViewer scroll = FindScrollViewer(listView, depth);
                                    if (scroll != null)
                                    {
                                        scroll.ScrollToHorizontalOffset(scroll.HorizontalOffset + delta);
                                        handled = true;
                                        break;
                                    }
                                }
                            }
                        }
                        return IntPtr.Zero;
                    };
                    hwndSource.AddHook(messageHook);
                }
            }
        }

        public void OnMouseLeave()
        {
            if (messageHook != null)
            {
                hwndSource.RemoveHook(messageHook);
                messageHook = null;
                hwndSource = null;
            }
        }

        private ScrollViewer FindScrollViewer(DependencyObject obj, int depth)
        {
            ScrollViewer scroll = obj as ScrollViewer;
            if (scroll == null && --depth >= 0)
            {
                for (int i = 0; scroll == null && i < VisualTreeHelper.GetChildrenCount(obj); i++)
                {
                    scroll = FindScrollViewer(VisualTreeHelper.GetChild(obj, i), depth);
                }
            }
            return scroll;
        }
    }
}
