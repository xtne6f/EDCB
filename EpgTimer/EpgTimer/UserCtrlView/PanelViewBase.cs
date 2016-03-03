using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;

namespace EpgTimer.UserCtrlView
{
    public class PanelBase : FrameworkElement //とりあえず。TunerReservePanelとEpgViewPanel結構実装違う
    {
        public static readonly DependencyProperty BackgroundProperty =
            Panel.BackgroundProperty.AddOwner(typeof(PanelBase));

        public Brush Background
        {
            set { SetValue(BackgroundProperty, value); }
            get { return (Brush)GetValue(BackgroundProperty); }
        }

        public class ItemFont
        {
            public string FamilyName { get; private set; }
            public bool IsBold { get; private set; }
            public GlyphTypeface GlyphType { get; private set; }
            public ushort[] GlyphIndexCache { get; private set; }
            public float[] GlyphWidthCache { get; private set; }

            public ItemFont(string familyName, bool isBold)
            {
                FamilyName = familyName;
                IsBold = isBold;
                GlyphTypeface glyphType = null;
                if ((new Typeface(new FontFamily(FamilyName),
                                  FontStyles.Normal,
                                  IsBold ? FontWeights.Bold : FontWeights.Normal,
                                  FontStretches.Normal)).TryGetGlyphTypeface(out glyphType) == false)
                {
                    (new Typeface(new FontFamily(System.Drawing.SystemFonts.DefaultFont.Name),
                                  FontStyles.Normal,
                                  IsBold ? FontWeights.Bold : FontWeights.Normal,
                                  FontStretches.Normal)).TryGetGlyphTypeface(out glyphType);
                }
                GlyphType = glyphType;
            }
            public void PrepareCache()
            {
                if (GlyphIndexCache == null)
                {
                    GlyphIndexCache = new ushort[ushort.MaxValue + 1];
                    GlyphWidthCache = new float[ushort.MaxValue + 1];
                }
            }
            public void ClearCache()
            {
                GlyphIndexCache = null;
                GlyphWidthCache = null;
            }
        }
       
    }
    
    public class PanelViewBase : UserControl
    {
        public delegate void PanelViewClickHandler(object sender, Point cursorPos);
        public event PanelViewClickHandler RightClick = null;
        public event PanelViewClickHandler LeftDoubleClick = null;
        public event ScrollChangedEventHandler ScrollChanged = null;

        protected Point lastDownMousePos;
        protected double lastDownHOffset;
        protected double lastDownVOffset;
        protected bool isDrag = false;
        protected bool isDragMoved = false;

        protected Point lastPopupPos;
        protected object lastPopupInfo = null;

        protected virtual bool IsSingleClickOpen { get { return false; } }
        protected virtual double DragScroll { get { return 1; } }
        protected virtual bool IsMouseScrollAuto { get { return false; } }
        protected virtual double ScrollSize { get { return 240; } }
        protected virtual bool IsPopupEnabled { get { return false; } }
        protected virtual object GetPopupItem(Point cursorPos) { return null; }
        protected virtual void SetPopup(object item) { return; }
        protected virtual FrameworkElement PopUp { get { return new FrameworkElement(); } }
        protected ScrollViewer scroll;
        protected Canvas cnvs;

        public virtual void ClearInfo()
        {
            cnvs.ReleaseMouseCapture();
            isDrag = false;
            isDragMoved = false;

            cnvs.Height = 0;
            cnvs.Width = 0;

            PopupClear();
        }

        protected virtual void PopupClear()
        {
            if (PopUp != null) PopUp.Visibility = Visibility.Hidden;
            lastPopupInfo = null;
            lastPopupPos = new Point(-1, -1);
        }

        protected virtual void PopUpWork(bool reset = false)
        {
            if (IsPopupEnabled == false) return;

            if (reset == true)
            {
                PopupClear();
            }

            Point cursorPos = Mouse.GetPosition(scroll);
            if (lastPopupPos != cursorPos)
            {
                lastPopupPos = cursorPos;

                if (cursorPos.X < 0 || cursorPos.Y < 0 ||
                    scroll.ViewportWidth < cursorPos.X || scroll.ViewportHeight < cursorPos.Y)
                {
                    PopupClear();
                    return;
                }

                Point itemPos = Mouse.GetPosition(cnvs);
                object item = GetPopupItem(itemPos);

                if (item != lastPopupInfo)
                {
                    lastPopupInfo = item;

                    if (item == null)
                    {
                        PopupClear();
                        return;
                    }

                    SetPopup(item);
                    PopUp.Visibility = Visibility.Visible;
                }
            }
        }

        // PopUp が画面内に収まるように調整する
        protected void UpdatePopupPosition(double LeftPos, double TopPos)
        {
            // offsetHが正のとき右にはみ出している
            double offsetH = LeftPos + PopUp.ActualWidth - (scroll.ContentHorizontalOffset + scroll.ActualWidth - 18);
            // 右にはみ出した分だけ左にずらす
            double left = LeftPos - Math.Max(0, offsetH);
            // 左にはみ出てる場合はscrollエリアの左端から表示する
            Canvas.SetLeft(PopUp, Math.Floor(Math.Max(left, scroll.ContentHorizontalOffset)));

            // offsetVが正のとき下にはみ出している
            double offsetV = TopPos + PopUp.ActualHeight - (scroll.ContentVerticalOffset + scroll.ActualHeight - 18);
            // 下にはみ出した分だけ上にずらす
            double top = TopPos - Math.Max(0, offsetV);
            // 上にはみ出てる場合はscrollエリアの上端から表示する
            Canvas.SetTop(PopUp, Math.Floor(Math.Max(top, scroll.ContentVerticalOffset)));
        }

        // PopUp の ActualWidth と ActualHeight を取得するために SizeChanged イベントを捕捉する
        protected virtual void popupItem_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            double LeftPos = 0;
            double TopPos = 0;
            if (lastPopupInfo is ProgramViewItem)
            {
                var popup = lastPopupInfo as ProgramViewItem;
                LeftPos = popup.LeftPos;
                TopPos = popup.TopPos;
            }
            else if (lastPopupInfo is ReserveViewItem)
            {
                var popup = lastPopupInfo as ReserveViewItem;
                LeftPos = popup.LeftPos;
                TopPos = popup.TopPos;
            }
            UpdatePopupPosition(LeftPos, TopPos);
        }

        /// <summary>マウスホイールイベント呼び出し</summary>
        protected virtual void scrollViewer_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            try
            {
                e.Handled = true;

                if (IsMouseScrollAuto == true)
                {
                    scroll.ScrollToVerticalOffset(scroll.VerticalOffset - e.Delta);
                }
                else
                {
                    if (e.Delta < 0)
                    {
                        //下方向
                        scroll.ScrollToVerticalOffset(scroll.VerticalOffset + ScrollSize);
                    }
                    else
                    {
                        //上方向
                        if (scroll.VerticalOffset < ScrollSize)
                        {
                            scroll.ScrollToVerticalOffset(0);
                        }
                        else
                        {
                            scroll.ScrollToVerticalOffset(scroll.VerticalOffset - ScrollSize);
                        }
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        protected virtual void scrollViewer_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            if (ScrollChanged != null)
            {
                scroll.ScrollToHorizontalOffset(Math.Floor(scroll.HorizontalOffset));
                scroll.ScrollToVerticalOffset(Math.Floor(scroll.VerticalOffset));
                ScrollChanged(this, e);
            }
        }

        public void view_ScrollChanged(ScrollViewer main_scroll, ScrollViewer v_scroll, ScrollViewer h_scroll)
        {
            try
            {
                //時間軸の表示もスクロール
                v_scroll.ScrollToVerticalOffset(main_scroll.VerticalOffset);
                //サービス名表示もスクロール
                h_scroll.ScrollToHorizontalOffset(main_scroll.HorizontalOffset);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        protected virtual void canvas_MouseMove(object sender, MouseEventArgs e)
        {
            try
            {
                if (e.LeftButton == MouseButtonState.Pressed && isDrag == true)
                {
                    isDragMoved = true;

                    Point CursorPos = Mouse.GetPosition(null);
                    double MoveX = lastDownMousePos.X - CursorPos.X;
                    double MoveY = lastDownMousePos.Y - CursorPos.Y;

                    double OffsetH = 0;
                    double OffsetV = 0;
                    MoveX *= DragScroll;
                    MoveY *= DragScroll;
                    OffsetH = lastDownHOffset + MoveX;
                    OffsetV = lastDownVOffset + MoveY;
                    if (OffsetH < 0) OffsetH = 0;
                    if (OffsetV < 0) OffsetV = 0;

                    scroll.ScrollToHorizontalOffset(Math.Floor(OffsetH));
                    scroll.ScrollToVerticalOffset(Math.Floor(OffsetV));
                }
                else
                {
                    PopUpWork();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
        protected virtual void canvas_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            try
            {
                lastDownMousePos = Mouse.GetPosition(null);
                lastDownHOffset = scroll.HorizontalOffset;
                lastDownVOffset = scroll.VerticalOffset;
                cnvs.CaptureMouse();
                isDrag = true;
                isDragMoved = false;

                if (e.ClickCount == 2)
                {
                    Point cursorPos = Mouse.GetPosition(cnvs);
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
        protected virtual void canvas_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            try
            {
                cnvs.ReleaseMouseCapture();
                isDrag = false;
                if (isDragMoved == false)
                {
                    if (IsSingleClickOpen == true)
                    {
                        Point cursorPos = Mouse.GetPosition(cnvs);
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

        protected virtual void canvas_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            cnvs.ReleaseMouseCapture();
            isDrag = false;
            lastDownMousePos = Mouse.GetPosition(null);
            lastDownHOffset = scroll.HorizontalOffset;
            lastDownVOffset = scroll.VerticalOffset;
            if (e.ClickCount == 1)
            {
                Point cursorPos = Mouse.GetPosition(cnvs);
                if (RightClick != null)
                {
                    RightClick(sender, cursorPos);
                }
            }
        }
        protected virtual void canvas_MouseLeave(object sender, MouseEventArgs e)
        {
            if (IsPopupEnabled == true)
            {
                //右クリック時はポップアップを維持
                if (e.RightButton != MouseButtonState.Pressed)
                {
                    PopupClear();
                }
            }
        }

        public virtual void ScrollToFindItem<T>(ViewPanelItem<T> target_item, bool IsMarking)
        {
            try
            {
                //可能性低いが0では無さそう
                if (target_item == null) return;

                scroll.ScrollToHorizontalOffset(target_item.LeftPos - 100);
                scroll.ScrollToVerticalOffset(target_item.TopPos - 100);

                //マーキング要求のあるとき
                if (IsMarking == true)
                {
                    Rectangle rect = new Rectangle();

                    rect.Stroke = Brushes.Red;
                    rect.StrokeThickness = 5;
                    rect.Opacity = 1;
                    rect.Fill = Brushes.Transparent;
                    rect.Effect = new System.Windows.Media.Effects.DropShadowEffect() { BlurRadius = 10 };

                    rect.Width = target_item.Width + 20;
                    rect.Height = target_item.Height + 20;
                    rect.IsHitTestVisible = false;

                    Canvas.SetLeft(rect, target_item.LeftPos - 10);
                    Canvas.SetTop(rect, target_item.TopPos - 10);
                    Canvas.SetZIndex(rect, 20);

                    // 一定時間枠を表示する
                    var notifyTimer = new System.Windows.Threading.DispatcherTimer();
                    notifyTimer.Interval = TimeSpan.FromSeconds(0.1);
                    int Brinks = 2 * 3;
                    cnvs.Children.Add(rect);
                    var sw = System.Diagnostics.Stopwatch.StartNew();
                    notifyTimer.Tick += (sender, e) =>
                    {
                        if (sw.ElapsedMilliseconds > Settings.Instance.DisplayNotifyJumpTime * 1000)
                        {
                            notifyTimer.Stop();
                            cnvs.Children.Remove(rect);
                        }
                        else if (--Brinks >= 0)
                        {
                            rect.Visibility = (Brinks % 2) == 0 ? Visibility.Visible : Visibility.Hidden;
                        }
                    };
                    notifyTimer.Start();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

    }
}
