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

        private Point lastDownMousePos;
        private double lastDownHOffset;
        private double lastDownVOffset;
        private bool isDrag = false;
        private bool isDragMoved = false;
        private HwndSource scrollViewerHwndSource;
        private HwndSourceHook horizontalScrollMessageHook;

        private DispatcherTimer toolTipTimer;
        private DispatcherTimer toolTipOffTimer; 
        private Point lastPopupPos;
        private ProgramViewItem lastPopupInfo = null;

        public ProgramView()
        {
            InitializeComponent();

            toolTipTimer = new DispatcherTimer(DispatcherPriority.Normal);
            toolTipTimer.Tick += new EventHandler(toolTipTimer_Tick);
            toolTipOffTimer = new DispatcherTimer(DispatcherPriority.Normal);
            toolTipOffTimer.Tick += new EventHandler(toolTipOffTimer_Tick);
        }

        void toolTip_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ClickCount == 2)
            {
                toolTip.IsOpen = false;
                if (LeftDoubleClick != null)
                {
                    LeftDoubleClick(sender, lastPopupPos);
                    e.Handled = true;
                }
            }
            else if (EpgSetting.EpgInfoSingleClick)
            {
                toolTip.IsOpen = false;
            }
        }

        void toolTip_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
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
                if (EpgSetting.EpgPopup || EpgSetting.EpgToolTip == false)
                {
                    return;
                }
                if (Window.GetWindow(this).IsActive == false)
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
                                if (info.TitleDrawErr == false && EpgSetting.EpgToolTipNoViewOnly)
                                {
                                    break;
                                }
                                string viewTip = "";

                                if (info != null)
                                {
                                    viewTip += new CommonManager.TimeDuration(info.EventInfo.StartTimeFlag != 0, info.EventInfo.start_time,
                                                                              info.EventInfo.DurationFlag != 0, info.EventInfo.durationSec) + "\r\n";
                                    if (info.EventInfo.ShortInfo != null)
                                    {
                                        viewTip += info.EventInfo.ShortInfo.event_name + "\r\n\r\n";
                                        viewTip += info.EventInfo.ShortInfo.text_char + "\r\n\r\n";
                                    }
                                    if (info.EventInfo.ExtInfo != null)
                                    {
                                        viewTip += CommonManager.TrimHyphenSpace(info.EventInfo.ExtInfo.text_char);
                                    }
                                }
                                toolTipTextBlock.Text = viewTip;
                                toolTipTextBlock.Background = new SolidColorBrush(Color.FromRgb(EpgSetting.EpgTipsBackColorR, EpgSetting.EpgTipsBackColorG, EpgSetting.EpgTipsBackColorB));
                                toolTipTextBlock.Foreground = new SolidColorBrush(Color.FromRgb(EpgSetting.EpgTipsForeColorR, EpgSetting.EpgTipsForeColorG, EpgSetting.EpgTipsForeColorB));
                                toolTip.IsOpen = true;
                                toolTipOffTimer.Stop();
                                toolTipOffTimer.Interval = TimeSpan.FromSeconds(10);
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
                toolTip.IsOpen = false;
            }
        }

        protected void PopupItem()
        {
            if (EpgSetting.EpgPopup == false) return;

            List<Brush> contentBrushList = null;
            ProgramViewItem info = null;

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
                    foreach (ProgramViewItem item in childPanel.Items)
                    {
                        if (item.LeftPos <= cursorPos.X && cursorPos.X < item.LeftPos + item.Width &&
                            item.TopPos <= cursorPos.Y && cursorPos.Y < item.TopPos + item.Height)
                        {
                            if (item == lastPopupInfo) return;

                            contentBrushList = childPanel.ContentBrushList;
                            info = item;
                            lastPopupInfo = info;
                            break;
                        }
                    }
                    break;
                }
            }

            if (info == null)
            {
                popupItem.Visibility = System.Windows.Visibility.Hidden;
                lastPopupInfo = null;
                return;
            }

            //この番組だけのEpgViewPanelをつくる
            popupItemPanel.Background = new SolidColorBrush(Color.FromRgb(EpgSetting.EpgBackColorR, EpgSetting.EpgBackColorG, EpgSetting.EpgBackColorB));
            popupItemPanel.Background.Freeze();
            popupItemPanel.Width = info.Width;
            var dictTitle = CommonManager.CreateReplaceDictionary(EpgSetting.EpgReplacePatternTitle);
            var dicNormal = CommonManager.CreateReplaceDictionary(EpgSetting.EpgReplacePattern);
            var itemFontTitle = new EpgViewPanel.ItemFont(EpgSetting.FontNameTitle, EpgSetting.FontBoldTitle, true);
            var itemFontNormal = new EpgViewPanel.ItemFont(EpgSetting.FontName, false, true);
            Brush brushTitle = ColorDef.CustColorBrush(EpgSetting.TitleColor1, EpgSetting.TitleCustColor1);
            Brush brushNormal = ColorDef.CustColorBrush(EpgSetting.TitleColor2, EpgSetting.TitleCustColor2);
            Canvas.SetLeft(popupItemPanel, 0);
            var items = new List<ProgramViewItem>() { new ProgramViewItem(info.EventInfo, info.Past, info.Filtered) };
            items[0].Width = info.Width;

            //テキスト全体を表示できる高さを求める
            items[0].Height = 4096;
            popupItemPanel.Initialize(items, EpgSetting.EpgBorderLeftSize, EpgSetting.EpgBorderTopSize,
                                      false, EpgSetting.EpgTitleIndent, EpgSetting.EpgExtInfoPopup,
                                      dictTitle, dicNormal, itemFontTitle, itemFontNormal,
                                      EpgSetting.FontSizeTitle, EpgSetting.FontSize, brushTitle, brushNormal,
                                      EpgSetting.EpgBackColorA, contentBrushList);
            double itemHeight = Math.Max(popupItemPanel.LastItemRenderTextHeight, info.Height);
            double topPos = info.TopPos;

            if (EpgSetting.EpgAdjustPopup)
            {
                //下方にはみ出す部分をできるだけ縮める
                double trimmableHeight = Math.Max(info.Height - popupItemPanel.LastItemRenderTextHeight, 0);
                double trimHeight = topPos + itemHeight - (scrollViewer.VerticalOffset + Math.Floor(scrollViewer.ViewportHeight));
                trimHeight = Math.Min(Math.Max(trimHeight, 0), trimmableHeight);
                trimmableHeight -= trimHeight;
                itemHeight -= trimHeight;
                //下方にはみ出すなら上げる
                topPos = Math.Min(topPos, scrollViewer.VerticalOffset + Math.Floor(scrollViewer.ViewportHeight) - itemHeight);
                //上方にはみ出すなら下げる
                topPos = Math.Max(topPos, scrollViewer.VerticalOffset);
                //ポップアップ前よりも下方に伸びる部分をできるだけ縮める
                trimHeight = topPos + itemHeight - (info.TopPos + info.Height);
                trimHeight = Math.Min(Math.Max(trimHeight, 0), trimmableHeight);
                itemHeight -= trimHeight;
            }

            items[0].Height = itemHeight;
            popupItemPanel.Height = itemHeight;
            popupItemPanel.Initialize(items, EpgSetting.EpgBorderLeftSize, EpgSetting.EpgBorderTopSize,
                                      topPos != info.TopPos, EpgSetting.EpgTitleIndent, EpgSetting.EpgExtInfoPopup,
                                      dictTitle, dicNormal, itemFontTitle, itemFontNormal,
                                      EpgSetting.FontSizeTitle, EpgSetting.FontSize, brushTitle, brushNormal,
                                      EpgSetting.EpgBackColorA, contentBrushList);
            popupItemPanel.InvalidateVisual();

            Canvas.SetLeft(popupItem, info.LeftPos);
            Canvas.SetTop(popupItem, topPos);
            popupItem.Visibility = System.Windows.Visibility.Visible;
        }

        public EpgSetting EpgSetting
        {
            get;
            set;
        }

        public void ClearInfo()
        {
            toolTipTimer.Stop();
            toolTip.IsOpen = false;
            lastPopupInfo = null;
            popupItem.Visibility = System.Windows.Visibility.Hidden;

            canvas.ReleaseMouseCapture();
            isDrag = false;

            for (int i = 0; i < canvas.Children.Count; i++)
            {
                if (canvas.Children[i] is EpgViewPanel || canvas.Children[i] is Rectangle)
                {
                    canvas.Children.RemoveAt(i--);
                }
            }
            canvasContainer.Height = canvas.Height = 0;
            canvasContainer.Width = canvas.Width = 0;
        }

        public void SetReserveList(List<ReserveViewItem> reserveList)
        {
            try
            {
                for (int i = 0; i < canvas.Children.Count; i++)
                {
                    if (canvas.Children[i] is Rectangle)
                    {
                        canvas.Children.RemoveAt(i--);
                    }
                }

                //0→50で塗りつぶしの不透明度が上がる
                int fillOpacity = Math.Min(EpgSetting.ReserveRectFillOpacity, 50) * 2;
                //50→100で枠の不透明度が下がる
                int strokeOpacity = Math.Min(100 - EpgSetting.ReserveRectFillOpacity, 50) * 2;
                //予約枠が色名指定のときは少し透過(0xA0)する
                Brush strokeNormal = ColorDef.CustColorBrush(EpgSetting.ReserveRectColorNormal, EpgSetting.ContentCustColorList[17], 0xA0, strokeOpacity);
                Brush strokeNo = ColorDef.CustColorBrush(EpgSetting.ReserveRectColorNo, EpgSetting.ContentCustColorList[18], 0xA0, strokeOpacity);
                Brush strokeNoTuner = ColorDef.CustColorBrush(EpgSetting.ReserveRectColorNoTuner, EpgSetting.ContentCustColorList[19], 0xA0, strokeOpacity);
                Brush strokeWarning = ColorDef.CustColorBrush(EpgSetting.ReserveRectColorWarning, EpgSetting.ContentCustColorList[20], 0xA0, strokeOpacity);
                Brush fillNormal = ColorDef.CustColorBrush(EpgSetting.ReserveRectColorNormal, EpgSetting.ContentCustColorList[17], 0xA0, fillOpacity);
                Brush fillNo = ColorDef.CustColorBrush(EpgSetting.ReserveRectColorNo, EpgSetting.ContentCustColorList[18], 0xA0, fillOpacity);
                Brush fillNoTuner = ColorDef.CustColorBrush(EpgSetting.ReserveRectColorNoTuner, EpgSetting.ContentCustColorList[19], 0xA0, fillOpacity);
                Brush fillWarning = ColorDef.CustColorBrush(EpgSetting.ReserveRectColorWarning, EpgSetting.ContentCustColorList[20], 0xA0, fillOpacity);
                var blurEffect = new System.Windows.Media.Effects.DropShadowEffect() { BlurRadius = 10 };
                blurEffect.Freeze();
                var dashArray = new DoubleCollection() { 2.5, 1.5 };
                dashArray.Freeze();

                foreach (ReserveViewItem info in reserveList)
                {
                    Rectangle rect = new Rectangle();
                    Rectangle fillOnlyRect = EpgSetting.ReserveRectFillWithShadow ? null : new Rectangle();
                    Rectangle fillRect = fillOnlyRect ?? rect;

                    if (info.ReserveInfo.RecSetting.IsNoRec())
                    {
                        rect.Stroke = strokeNo;
                        fillRect.Fill = fillNo;
                    }
                    else if (info.ReserveInfo.OverlapMode == 2)
                    {
                        rect.Stroke = strokeNoTuner;
                        fillRect.Fill = fillNoTuner;
                    }
                    else if (info.ReserveInfo.OverlapMode == 1)
                    {
                        rect.Stroke = strokeWarning;
                        fillRect.Fill = fillWarning;
                    }
                    else
                    {
                        rect.Stroke = strokeNormal;
                        fillRect.Fill = fillNormal;
                    }

                    rect.Effect = blurEffect;
                    rect.StrokeThickness = 3;
                    if (info.ReserveInfo.RecSetting.GetRecMode() == 4)
                    {
                        rect.StrokeDashArray = dashArray;
                        rect.StrokeDashCap = PenLineCap.Round;
                    }
                    rect.Width = info.Width;
                    rect.Height = info.Height;
                    rect.IsHitTestVisible = false;
                    fillRect.Width = info.Width;
                    fillRect.Height = info.Height;
                    fillRect.IsHitTestVisible = false;

                    Canvas.SetLeft(rect, info.LeftPos);
                    Canvas.SetTop(rect, info.TopPos);
                    Canvas.SetZIndex(rect, 10);
                    canvas.Children.Add(rect);

                    if (fillOnlyRect != null)
                    {
                        Canvas.SetLeft(fillOnlyRect, info.LeftPos);
                        Canvas.SetTop(fillOnlyRect, info.TopPos);
                        Canvas.SetZIndex(fillOnlyRect, 9);
                        canvas.Children.Add(fillOnlyRect);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
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
                var itemFontNormal = new EpgViewPanel.ItemFont(EpgSetting.FontName, false, false);
                var itemFontTitle = new EpgViewPanel.ItemFont(EpgSetting.FontNameTitle, EpgSetting.FontBoldTitle, false);
                var background = new SolidColorBrush(Color.FromRgb(EpgSetting.EpgBackColorR, EpgSetting.EpgBackColorG, EpgSetting.EpgBackColorB));
                //フリーズしないとかなり重い
                background.Freeze();
                var dictTitle = CommonManager.CreateReplaceDictionary(EpgSetting.EpgReplacePatternTitle);
                var dicNormal = CommonManager.CreateReplaceDictionary(EpgSetting.EpgReplacePattern);
                Brush brushTitle = ColorDef.CustColorBrush(EpgSetting.TitleColor1, EpgSetting.TitleCustColor1);
                Brush brushNormal = ColorDef.CustColorBrush(EpgSetting.TitleColor2, EpgSetting.TitleCustColor2);
                //ジャンル別の背景ブラシ
                var contentBrushList = new List<Brush>();
                for (int i = 0; i < EpgSetting.ContentColorList.Count; i++)
                {
                    SolidColorBrush brush = ColorDef.CustColorBrush(EpgSetting.ContentColorList[i], EpgSetting.ContentCustColorList[i]);
                    contentBrushList.Add(EpgSetting.EpgGradation ? (Brush)ColorDef.GradientBrush(brush.Color) : brush);
                }
                double totalWidth = 0;
                foreach (var programList in programGroupList)
                {
                    EpgViewPanel item = new EpgViewPanel();
                    item.Background = background;
                    item.Height = Math.Ceiling(height);
                    item.Width = programList.Item1;
                    Canvas.SetLeft(item, totalWidth);
                    item.Initialize(programList.Item2, EpgSetting.EpgBorderLeftSize, EpgSetting.EpgBorderTopSize,
                                    false, EpgSetting.EpgTitleIndent, EpgSetting.EpgExtInfoTable,
                                    dictTitle, dicNormal, itemFontTitle, itemFontNormal,
                                    EpgSetting.FontSizeTitle, EpgSetting.FontSize, brushTitle, brushNormal,
                                    EpgSetting.EpgBackColorA, contentBrushList);
                    item.InvalidateVisual();
                    canvas.Children.Add(item);
                    totalWidth += programList.Item1;
                }
                //包含するCanvasにも直接適用する(Bindingは遅延するため)。GridやStackPanelで包含してもよいはずだが
                //非表示中にここを通るとなぜか包含側のActualHeight(Width)が正しく更新されず、結果スクロール不能になる
                canvasContainer.Height = canvas.Height = Math.Ceiling(height);
                canvasContainer.Width = canvas.Width = totalWidth;
                itemFontNormal.ClearCache();
                itemFontTitle.ClearCache();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
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
                        toolTip.IsOpen = false;

                        Point CursorPos = Mouse.GetPosition(null);
                        double MoveX = lastDownMousePos.X - CursorPos.X;
                        double MoveY = lastDownMousePos.Y - CursorPos.Y;

                        double OffsetH = 0;
                        double OffsetV = 0;
                        MoveX *= EpgSetting.DragScroll;
                        MoveY *= EpgSetting.DragScroll;
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

                            toolTipTimer.Interval = TimeSpan.FromMilliseconds(EpgSetting.EpgToolTipViewWait);
                            toolTipTimer.Start();
                            PopupItem();
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

        private void canvas_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            try
            {
                toolTipTimer.Stop();
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
                        e.Handled = true;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
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
                    if (EpgSetting.EpgInfoSingleClick)
                    {
                        Point cursorPos = Mouse.GetPosition(canvas);
                        if (LeftDoubleClick != null)
                        {
                            LeftDoubleClick(sender, cursorPos);
                            e.Handled = true;
                        }
                    }
                }
                isDragMoved = false;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void scrollViewer_MouseEnter(object sender, MouseEventArgs e)
        {
            if (horizontalScrollMessageHook == null && (EpgSetting.MouseHorizontalScrollAuto || EpgSetting.HorizontalScrollSize != 0))
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
                            toolTip.IsOpen = false;

                            double delta = (short)((wParam.ToInt64() >> 16) & 0xFFFF);
                            if (delta != 0)
                            {
                                //負のとき左方向
                                delta = EpgSetting.MouseHorizontalScrollAuto ? delta : EpgSetting.HorizontalScrollSize * (delta < 0 ? -1 : 1);
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

        private void canvas_MouseRightButtonUp(object sender, MouseButtonEventArgs e)
        {
            toolTipTimer.Stop();
            toolTip.IsOpen = false;

            canvas.ReleaseMouseCapture();
            isDrag = false; 
        }

        private void canvas_MouseLeave(object sender, MouseEventArgs e)
        {
            if (EpgSetting.EpgPopup)
            {
                popupItem.Visibility = System.Windows.Visibility.Hidden;
                lastPopupInfo = null;
                lastPopupPos = new Point(-1, -1);
            }
        }

        void UserControl_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            toolTipTimer.Stop();
            toolTip.IsOpen = false;

            if (e.Delta != 0)
            {
                //負のとき下方向
                double delta = EpgSetting.MouseScrollAuto ? e.Delta : EpgSetting.ScrollSize * (e.Delta < 0 ? -1 : 1);
                scrollViewer.ScrollToVerticalOffset(scrollViewer.VerticalOffset - delta);
            }
            e.Handled = true;
        }
    }
}
