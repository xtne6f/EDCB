using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows.Threading;
using System.Windows.Controls.Primitives;

namespace EpgTimer.TunerReserveViewCtrl
{
    /// <summary>
    /// TunerReserveView.xaml の相互作用ロジック
    /// </summary>
    public partial class TunerReserveView : UserControl
    {
        public delegate void ProgramViewClickHandler(object sender, Point cursorPos);
        public event ScrollChangedEventHandler ScrollChanged = null;
        public event ProgramViewClickHandler LeftDoubleClick = null;

        private Point lastDownMousePos;
        private double lastDownHOffset;
        private double lastDownVOffset;
        private bool isDrag = false;
        private HwndSource scrollViewerHwndSource;
        private HwndSourceHook horizontalScrollMessageHook;

        private DispatcherTimer toolTipTimer;
        private DispatcherTimer toolTipOffTimer;
        private Point lastPopupPos;
        private ReserveViewItem lastPopupInfo;

        public TunerReserveView()
        {
            InitializeComponent();

            toolTipTimer = new DispatcherTimer(DispatcherPriority.Normal);
            toolTipTimer.Tick += new EventHandler(toolTipTimer_Tick);
            toolTipOffTimer = new DispatcherTimer(DispatcherPriority.Normal);
            toolTipOffTimer.Tick += new EventHandler(toolTipOffTimer_Tick);
            toolTipOffTimer.Interval = TimeSpan.FromSeconds(15);
        }

        public void ClearInfo()
        {
            toolTipTimer.Stop();
            toolTipOffTimer.Stop();
            toolTip.IsOpen = false;

            reserveViewPanel.ReleaseMouseCapture();
            isDrag = false;

            reserveViewPanel.Items = null;
            reserveViewPanel.Height = 0;
            reserveViewPanel.Width = 0;
            canvas.Height = 0;
            canvas.Width = 0;
        }

        void toolTip_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ClickCount == 2)
            {
                toolTipTimer.Stop();
                toolTipOffTimer.Stop();
                toolTip.IsOpen = false;

                if (LeftDoubleClick != null)
                {
                    LeftDoubleClick(sender, lastPopupPos);
                    e.Handled = true;
                }
            }
        }

        void toolTip_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            toolTipTimer.Stop();
            toolTipOffTimer.Stop();
            toolTip.IsOpen = false;
        }

        void toolTipOffTimer_Tick(object sender, EventArgs e)
        {
            toolTipOffTimer.Stop();
            toolTip.IsOpen = false;
        }

        void toolTipTimer_Tick(object sender, EventArgs e)
        {
            toolTipTimer.Stop();
            try
            {
                if (Settings.Instance.NoToolTip == true)
                {
                    return;
                } 
                if (reserveViewPanel.Items != null)
                {
                    if (MainWindow.GetWindow(this).IsActive == false)
                    {
                        return;
                    }
                    Point cursorPos2 = Mouse.GetPosition(scrollViewer);
                    if (cursorPos2.X < 0 || cursorPos2.Y < 0 ||
                        scrollViewer.ViewportWidth < cursorPos2.X || scrollViewer.ViewportHeight < cursorPos2.Y)
                    {
                        return;
                    }
                    Point cursorPos = Mouse.GetPosition(reserveViewPanel);
                    foreach (ReserveViewItem info in reserveViewPanel.Items)
                    {
                        if (info.LeftPos <= cursorPos.X && cursorPos.X < info.LeftPos + info.Width)
                        {
                            if (info.TopPos <= cursorPos.Y && cursorPos.Y < info.TopPos + info.Height)
                            {
                                if (info.TitleDrawErr == true)
                                {
                                    string view = new CommonManager.TimeDuration(true, info.ReserveInfo.StartTime,
                                                                                 true, info.ReserveInfo.DurationSecond) + "\r\n";
                                    view += info.ReserveInfo.StationName;
                                    view += " (" + CommonManager.ConvertNetworkNameText(info.ReserveInfo.OriginalNetworkID) + ")" + "\r\n";

                                    view += info.ReserveInfo.Title;

                                    toolTipTextBlock.Text = view;
                                    toolTipTextBlock.Background = new SolidColorBrush(Color.FromRgb(
                                        Settings.Instance.EpgSettingList[0].EpgTipsBackColorR,
                                        Settings.Instance.EpgSettingList[0].EpgTipsBackColorG,
                                        Settings.Instance.EpgSettingList[0].EpgTipsBackColorB));
                                    toolTipTextBlock.Foreground = new SolidColorBrush(Color.FromRgb(
                                        Settings.Instance.EpgSettingList[0].EpgTipsForeColorR,
                                        Settings.Instance.EpgSettingList[0].EpgTipsForeColorG,
                                        Settings.Instance.EpgSettingList[0].EpgTipsForeColorB));
                                    toolTip.IsOpen = true;
                                    toolTipOffTimer.Start();

                                    lastPopupInfo = info;
                                    lastPopupPos = cursorPos;
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        public void SetReserveList(List<ReserveViewItem> programList, double width, double height)
        {
            try
            {
                canvas.Height = Math.Ceiling(height);
                canvas.Width = Math.Ceiling(width);
                reserveViewPanel.Height = Math.Ceiling(height);
                reserveViewPanel.Width = Math.Ceiling(width);
                reserveViewPanel.Items = programList;
                reserveViewPanel.InvalidateVisual();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void reserveViewPanel_MouseMove(object sender, MouseEventArgs e)
        {
            try
            {
                {
                    if (e.LeftButton == MouseButtonState.Pressed && isDrag == true)
                    {
                        toolTipTimer.Stop();
                        toolTipOffTimer.Stop();
                        toolTip.IsOpen = false;

                        Point CursorPos = Mouse.GetPosition(null);
                        double MoveX = lastDownMousePos.X - CursorPos.X;
                        double MoveY = lastDownMousePos.Y - CursorPos.Y;

                        double OffsetH = 0;
                        double OffsetV = 0;
                        MoveX *= Settings.Instance.EpgSettingList[0].DragScroll;
                        MoveY *= Settings.Instance.EpgSettingList[0].DragScroll;
                        OffsetH = lastDownHOffset + MoveX;
                        OffsetV = lastDownVOffset + MoveY;
                        if (OffsetH < 0)
                        {
                            OffsetH = 0;
                        }
                        if (OffsetV < 0)
                        {
                            OffsetV = 0;
                        }

                        scrollViewer.ScrollToHorizontalOffset(Math.Floor(OffsetH));
                        scrollViewer.ScrollToVerticalOffset(Math.Floor(OffsetV));
                    }
                    else
                    {
                        Point CursorPos = Mouse.GetPosition(reserveViewPanel);
                        if (lastPopupPos != CursorPos)
                        {
                            toolTipTimer.Stop();
                            toolTipOffTimer.Stop();
                            if (toolTip.IsOpen == true)
                            {
                                toolTip.IsOpen = false;
                                lastDownMousePos = Mouse.GetPosition(null);
                                lastDownHOffset = scrollViewer.HorizontalOffset;
                                lastDownVOffset = scrollViewer.VerticalOffset;
                                if (e.LeftButton == MouseButtonState.Pressed)
                                {
                                    reserveViewPanel.CaptureMouse();
                                    isDrag = true;
                                }

                            }

                            toolTipTimer.Interval = TimeSpan.FromMilliseconds(Settings.Instance.EpgSettingList[0].EpgToolTipViewWait);
                            toolTipTimer.Start();
                            lastPopupPos = CursorPos;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void reserveViewPanel_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            try
            {
                toolTipTimer.Stop();
                toolTipOffTimer.Stop();
                toolTip.IsOpen = false;

                lastDownMousePos = Mouse.GetPosition(null);
                lastDownHOffset = scrollViewer.HorizontalOffset;
                lastDownVOffset = scrollViewer.VerticalOffset;
                reserveViewPanel.CaptureMouse();
                isDrag = true;

                if (e.ClickCount == 2)
                {
                    Point cursorPos = Mouse.GetPosition(reserveViewPanel);
                    if (LeftDoubleClick != null)
                    {
                        LeftDoubleClick(sender, cursorPos);
                        e.Handled = true;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void reserveViewPanel_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            try
            {
                reserveViewPanel.ReleaseMouseCapture();
                isDrag = false;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void scrollViewer_MouseEnter(object sender, MouseEventArgs e)
        {
            if (horizontalScrollMessageHook == null &&
                (Settings.Instance.EpgSettingList[0].MouseHorizontalScrollAuto ||
                 Settings.Instance.EpgSettingList[0].HorizontalScrollSize != 0))
            {
                scrollViewerHwndSource = PresentationSource.FromVisual(scrollViewer) as HwndSource;
                if (scrollViewerHwndSource != null)
                {
                    horizontalScrollMessageHook = (IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled) =>
                    {
                        const int WM_MOUSEHWHEEL = 0x020E;
                        if (msg == WM_MOUSEHWHEEL)
                        {
                            toolTipTimer.Stop();
                            toolTipOffTimer.Stop();
                            toolTip.IsOpen = false;

                            double delta = (short)((wParam.ToInt64() >> 16) & 0xFFFF);
                            if (delta != 0)
                            {
                                //負のとき左方向
                                delta = Settings.Instance.EpgSettingList[0].MouseHorizontalScrollAuto ? delta :
                                            Settings.Instance.EpgSettingList[0].HorizontalScrollSize * (delta < 0 ? -1 : 1);
                                scrollViewer.ScrollToHorizontalOffset(scrollViewer.HorizontalOffset + delta);
                            }
                            handled = true;
                        }
                        return IntPtr.Zero;
                    };
                    scrollViewerHwndSource.AddHook(horizontalScrollMessageHook);
                }
            }
        }

        private void scrollViewer_MouseLeave(object sender, MouseEventArgs e)
        {
            if (horizontalScrollMessageHook != null)
            {
                scrollViewerHwndSource.RemoveHook(horizontalScrollMessageHook);
                horizontalScrollMessageHook = null;
                scrollViewerHwndSource = null;
            }
        }

        private void scrollViewer_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            var ps = PresentationSource.FromVisual(this);
            if (ps != null)
            {
                //スクロール位置を物理ピクセルに合わせる
                Matrix m = ps.CompositionTarget.TransformToDevice;
                scrollViewer.ScrollToHorizontalOffset(Math.Floor(scrollViewer.HorizontalOffset * m.M11) / m.M11);
                scrollViewer.ScrollToVerticalOffset(Math.Floor(scrollViewer.VerticalOffset * m.M22) / m.M22);
            }
            if (ScrollChanged != null)
            {
                ScrollChanged(this, e);
            }
        }

        private void reserveViewPanel_MouseRightButtonUp(object sender, MouseButtonEventArgs e)
        {
            toolTipTimer.Stop();
            toolTipOffTimer.Stop();
            toolTip.IsOpen = false;

            reserveViewPanel.ReleaseMouseCapture();
            isDrag = false;
        }

        void UserControl_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            toolTipTimer.Stop();
            toolTipOffTimer.Stop();
            toolTip.IsOpen = false;

            if (e.Delta != 0)
            {
                //負のとき下方向
                double delta = Settings.Instance.EpgSettingList[0].MouseScrollAuto ? e.Delta :
                                   Settings.Instance.EpgSettingList[0].ScrollSize * (e.Delta < 0 ? -1 : 1);
                scrollViewer.ScrollToVerticalOffset(scrollViewer.VerticalOffset - delta);
            }
            e.Handled = true;
        }
    }

}
