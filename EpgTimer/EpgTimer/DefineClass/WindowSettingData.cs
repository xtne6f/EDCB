using System.Windows;

namespace EpgTimer
{
    public class WindowSetData
    {
        public double? Top = null;
        public double? Left = null;
        public double? Width = null;
        public double? Height = null;
        public WindowState LastWindowState = WindowState.Normal;

        public bool Pinned = true;
        public bool DataChange = false;

        public virtual void SetSizeToWindow(Window wnd, bool noMinimized = true)
        {
            if (Top != null) wnd.Top = (double)Top;
            if (Left != null) wnd.Left = (double)Left;
            if (Width != null) wnd.Width = (double)Width;
            if (Height != null) wnd.Height = (double)Height;
            if (noMinimized == false || LastWindowState != WindowState.Minimized) wnd.WindowState = LastWindowState;
        }
        public virtual void GetSizeFromWindow(Window wnd, bool noMinimized = true)
        {
            if (wnd.WindowState == WindowState.Normal)
            {
                Top = wnd.Top;
                Left = wnd.Left;
                if (wnd.Width >= 1) Width = wnd.Width;
                if (wnd.Height >= 1) Height = wnd.Height;
            }
            if (noMinimized == false || wnd.WindowState != WindowState.Minimized) LastWindowState = wnd.WindowState;
        }
    }

    public class WindowSettingData : TypeDataSet<Window, WindowSetData>
    {
        public virtual void SetSizeToWindow(Window wnd, bool noMinimized = true)
        {
            this[wnd].SetSizeToWindow(wnd);
        }
        public virtual void GetSizeFromWindow(Window wnd, bool noMinimized = true)
        {
            this[wnd].GetSizeFromWindow(wnd, noMinimized);
        }
    }
}
