using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;
using System.Windows.Controls.Primitives;

namespace EpgTimer.EpgView
{
    /// <summary>
    /// ProgramView.xaml の相互作用ロジック
    /// </summary>
    public partial class ProgramView : UserControl
    {
        public delegate void ProgramViewClickHandler(object sender, Point cursorPos);
        public event ScrollChangedEventHandler ScrollChanged = null;
        public event ProgramViewClickHandler LeftDoubleClick = null;
        public event ProgramViewClickHandler RightClick = null;
        private List<Rectangle> reserveBorder = new List<Rectangle>();

        private Point lastDownMousePos;
        private double lastDownHOffset;
        private double lastDownVOffset;
        private bool isDrag = false;
        private bool isDragMoved = false;

        private DispatcherTimer toolTipTimer;
        private DispatcherTimer toolTipOffTimer; 
        private Popup toolTip = new Popup();
        private Point lastPopupPos;
        private ProgramViewItem lastPopupInfo;

        public ProgramView()
        {
            InitializeComponent();

            toolTipTimer = new DispatcherTimer(DispatcherPriority.Normal);
            toolTipTimer.Tick += new EventHandler(toolTipTimer_Tick);
            toolTipTimer.Interval = TimeSpan.FromMilliseconds(1500);
            toolTipOffTimer = new DispatcherTimer(DispatcherPriority.Normal);
            toolTipOffTimer.Tick += new EventHandler(toolTipOffTimer_Tick);
            toolTipOffTimer.Interval = TimeSpan.FromSeconds(10);

            toolTip.Placement = PlacementMode.MousePoint;
            toolTip.PopupAnimation = PopupAnimation.Fade;
            toolTip.PlacementTarget = canvas;
            toolTip.AllowsTransparency = true;
            toolTip.MouseLeftButtonDown += new MouseButtonEventHandler(toolTip_MouseLeftButtonDown);
            toolTip.PreviewMouseWheel += new MouseWheelEventHandler(toolTip_PreviewMouseWheel);
        }

        void toolTip_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            toolTipTimer.Stop();
            toolTipOffTimer.Stop();
            toolTip.IsOpen = false;

            RaiseEvent(e);
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
                }
            }
            else if (e.ClickCount == 1 && Settings.Instance.EpgInfoSingleClick == true)
            {
                toolTipTimer.Stop();
                toolTipOffTimer.Stop();
                toolTip.IsOpen = false;

                if (LeftDoubleClick != null)
                {
                    LeftDoubleClick(sender, lastPopupPos);
                }
            }
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
                if (Settings.Instance.EpgToolTip == false)
                {
                    return;
                }
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
                Point cursorPos = Mouse.GetPosition(canvas);
                foreach (UIElement child in canvas.Children)
                {
                    EpgViewPanel childPanel = child as EpgViewPanel;
                    if (childPanel != null && childPanel.Items != null && Canvas.GetLeft(child) <= cursorPos.X && cursorPos.X < Canvas.GetLeft(child) + childPanel.Width)
                    {
                        foreach (ProgramViewItem info in childPanel.Items)
                        {
                            if (info.LeftPos <= cursorPos.X && cursorPos.X < info.LeftPos + info.Width &&
                                info.TopPos <= cursorPos.Y && cursorPos.Y < info.TopPos + info.Height)
                            {
                                if (info.TitleDrawErr == false && Settings.Instance.EpgToolTipNoViewOnly == true)
                                {
                                    break;
                                }
                                String viewTip = "";

                                if (info != null)
                                {
                                    if (info.EventInfo.StartTimeFlag == 1)
                                    {
                                        viewTip += info.EventInfo.start_time.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
                                    }
                                    else
                                    {
                                        viewTip += "未定 ～ ";
                                    }
                                    if (info.EventInfo.DurationFlag == 1)
                                    {
                                        DateTime endTime = info.EventInfo.start_time + TimeSpan.FromSeconds(info.EventInfo.durationSec);
                                        viewTip += endTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss") + "\r\n";
                                    }
                                    else
                                    {
                                        viewTip += "未定\r\n";
                                    }

                                    if (info.EventInfo.ShortInfo != null)
                                    {
                                        viewTip += info.EventInfo.ShortInfo.event_name + "\r\n\r\n";
                                        viewTip += info.EventInfo.ShortInfo.text_char + "\r\n\r\n";
                                    }
                                    if (info.EventInfo.ExtInfo != null)
                                    {
                                        viewTip += info.EventInfo.ExtInfo.text_char;
                                    }
                                }
                                Border border = new Border();
                                border.Background = Brushes.DarkGray;

                                TextBlock block = new TextBlock();
                                block.Text = viewTip;
                                block.MaxWidth = 400;
                                block.TextWrapping = TextWrapping.Wrap;
                                block.Margin = new Thickness(2);

                                block.Background = CommonManager.Instance.EpgTipsBackColor;
                                block.Foreground = CommonManager.Instance.EpgTipsForeColor;
                                border.Child = block;
                                toolTip.Child = border;
                                toolTip.IsOpen = true;
                                toolTipOffTimer.Start();

                                lastPopupInfo = info;
                                lastPopupPos = cursorPos;
                            }
                        }
                        break;
                    }
                }
            }
            catch
            {
                toolTipTimer.Stop();
                toolTipOffTimer.Stop();
                toolTip.IsOpen = false;
            }
        }

        public void ClearInfo()
        {
            toolTipTimer.Stop();
            toolTipOffTimer.Stop();
            toolTip.IsOpen = false;

            foreach (Rectangle info in reserveBorder)
            {
                canvas.Children.Remove(info);
            }
            reserveBorder.Clear();
            reserveBorder = null;
            reserveBorder = new List<Rectangle>();

            canvas.ReleaseMouseCapture();
            isDrag = false;

            for (int i = 0; i < canvas.Children.Count; i++)
            {
                if (canvas.Children[i] is EpgViewPanel)
                {
                    canvas.Children.RemoveAt(i--);
                }
            }
            canvas.Height = 0;
            canvas.Width = 0;
        }

        public void SetReserveList(List<ReserveViewItem> reserveList)
        {
            try
            {
                foreach (Rectangle info in reserveBorder)
                {
                    canvas.Children.Remove(info);
                }
                reserveBorder.Clear();

                foreach (ReserveViewItem info in reserveList)
                {
                    Rectangle rect = new Rectangle();

                    SolidColorBrush color;
                    if (info.ReserveInfo.RecSetting.RecMode == 5)
                    {
                        color = CommonManager.Instance.CustContentColorList[0x12];
                    }
                    else if (info.ReserveInfo.OverlapMode == 2)
                    {
                        color = CommonManager.Instance.CustContentColorList[0x13];
                    }
                    else if (info.ReserveInfo.OverlapMode == 1)
                    {
                        color = CommonManager.Instance.CustContentColorList[0x14];
                    }
                    else
                    {
                        color = CommonManager.Instance.CustContentColorList[0x11];
                    }

                    if (Settings.Instance.ReserveRectBackground == false)
                    {
                        rect.Opacity = 0.5;
                        rect.Effect = new System.Windows.Media.Effects.DropShadowEffect() { BlurRadius = 10 };
                        rect.Fill = System.Windows.Media.Brushes.Transparent;
                        rect.StrokeThickness = 3;

                        rect.Stroke = color;
                    }
                    else
                    {
                        rect.Opacity = 0.3;
                        rect.Effect = new System.Windows.Media.Effects.DropShadowEffect() { BlurRadius = 6 };
                        rect.Fill = color;
                    }
                    rect.Width = info.Width;
                    rect.Height = info.Height;

                    Canvas.SetLeft(rect, info.LeftPos);
                    Canvas.SetTop(rect, info.TopPos);
                    Canvas.SetZIndex(rect, 10);
                    canvas.Children.Add(rect);
                    reserveBorder.Add(rect);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void SetProgramList(List<ProgramViewItem> programList, double width, double height)
        {
            var programGroupList = new List<Tuple<double, List<ProgramViewItem>>>();
            programGroupList.Add(new Tuple<double, List<ProgramViewItem>>(width, programList));
            SetProgramList(programGroupList, height);
        }

        public void SetProgramList(List<Tuple<double, List<ProgramViewItem>>> programGroupList, double height)
        {
            try
            {
                for (int i = 0; i < canvas.Children.Count; i++)
                {
                    if (canvas.Children[i] is EpgViewPanel)
                    {
                        canvas.Children.RemoveAt(i--);
                    }
                }
                var itemFontNormal = new EpgViewPanel.ItemFont(Settings.Instance.FontName, false);
                if (itemFontNormal.GlyphType == null)
                {
                    itemFontNormal = new EpgViewPanel.ItemFont("MS UI Gothic", false);
                }
                var itemFontTitle = new EpgViewPanel.ItemFont(Settings.Instance.FontNameTitle, Settings.Instance.FontBoldTitle);
                if (itemFontTitle.GlyphType == null)
                {
                    itemFontTitle = new EpgViewPanel.ItemFont("MS UI Gothic", Settings.Instance.FontBoldTitle);
                }
                double totalWidth = 0;
                foreach (var programList in programGroupList)
                {
                    EpgViewPanel item = new EpgViewPanel();
                    item.Background = epgViewPanel.Background;
                    item.Height = Math.Ceiling(height);
                    item.Width = programList.Item1;
                    item.IsTitleIndent = Settings.Instance.EpgTitleIndent;
                    item.ItemFontNormal = itemFontNormal;
                    item.ItemFontTitle = itemFontTitle;
                    Canvas.SetLeft(item, totalWidth);
                    item.Items = programList.Item2;
                    item.InvalidateVisual();
                    canvas.Children.Add(item);
                    totalWidth += programList.Item1;
                }
                canvas.Height = Math.Ceiling(height);
                canvas.Width = totalWidth;
                itemFontNormal.ClearCache();
                itemFontTitle.ClearCache();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void canvas_MouseMove(object sender, MouseEventArgs e)
        {
            try
            {
                {
                    if (e.LeftButton == MouseButtonState.Pressed && isDrag == true)
                    {
                        isDragMoved = true;
                        toolTipTimer.Stop();
                        toolTipOffTimer.Stop();
                        toolTip.IsOpen = false;

                        Point CursorPos = Mouse.GetPosition(null);
                        double MoveX = lastDownMousePos.X - CursorPos.X;
                        double MoveY = lastDownMousePos.Y - CursorPos.Y;

                        double OffsetH = 0;
                        double OffsetV = 0;
                        MoveX *= Settings.Instance.DragScroll;
                        MoveY *= Settings.Instance.DragScroll;
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
                        Point CursorPos = Mouse.GetPosition(canvas);
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
                                    canvas.CaptureMouse();
                                    isDrag = true;
                                }

                            }

                            toolTipTimer.Interval = TimeSpan.FromMilliseconds(Settings.Instance.EpgToolTipViewWait);
                            toolTipTimer.Start();
                            lastPopupPos = CursorPos;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void canvas_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            try
            {
                toolTipTimer.Stop();
                toolTipOffTimer.Stop();
                toolTip.IsOpen = false;

                lastDownMousePos = Mouse.GetPosition(null);
                lastDownHOffset = scrollViewer.HorizontalOffset;
                lastDownVOffset = scrollViewer.VerticalOffset;
                canvas.CaptureMouse();
                isDrag = true;
                isDragMoved = false;

                if (e.ClickCount == 2)
                {
                    Point cursorPos = Mouse.GetPosition(canvas);
                    if (LeftDoubleClick != null)
                    {
                        LeftDoubleClick(sender, cursorPos);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void canvas_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            try
            {
                canvas.ReleaseMouseCapture();
                isDrag = false;
                if (isDragMoved == false)
                {
                    if (Settings.Instance.EpgInfoSingleClick == true)
                    {
                        Point cursorPos = Mouse.GetPosition(canvas);
                        if (LeftDoubleClick != null)
                        {
                            LeftDoubleClick(sender, cursorPos);
                        }
                    }
                }
                isDragMoved = false;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void scrollViewer_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            if (ScrollChanged != null)
            {
                scrollViewer.ScrollToHorizontalOffset(Math.Floor(scrollViewer.HorizontalOffset));
                scrollViewer.ScrollToVerticalOffset(Math.Floor(scrollViewer.VerticalOffset));
                ScrollChanged(this, e);
            }
        }

        private void canvas_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            toolTipTimer.Stop();
            toolTipOffTimer.Stop();
            toolTip.IsOpen = false;

            canvas.ReleaseMouseCapture();
            isDrag = false; 
            lastDownMousePos = Mouse.GetPosition(null);
            lastDownHOffset = scrollViewer.HorizontalOffset;
            lastDownVOffset = scrollViewer.VerticalOffset;
            if (e.ClickCount == 1)
            {
                Point cursorPos = Mouse.GetPosition(canvas);
                if (RightClick != null)
                {
                    RightClick(sender, cursorPos);
                }
            }
        }
    }
}
