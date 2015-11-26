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
        protected ViewUtil vutil = CommonManager.Instance.VUtil;
 
        public static readonly DependencyProperty BackgroundProperty =
            Panel.BackgroundProperty.AddOwner(typeof(PanelBase));

        public Brush Background
        {
            set { SetValue(BackgroundProperty, value); }
            get { return (Brush)GetValue(BackgroundProperty); }
        }

        public virtual void ClearInfo() { }
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
        protected virtual bool IsPopupEnabled { get { return false; } }
        protected virtual object GetPopupItem(Point cursorPos) { return null; }
        protected virtual void SetPopup(object item) { return; }
        protected virtual FrameworkElement PopUp { get { return new FrameworkElement(); } }
        protected ScrollViewer scroll;
        protected PanelBase viewPanel;
        protected Canvas cnvs;

        public virtual void ClearInfo()
        {
            viewPanel.ReleaseMouseCapture();
            isDrag = false;
            isDragMoved = false;

            cnvs.Height = 0;
            cnvs.Width = 0;
            viewPanel.ClearInfo();
            viewPanel.Height = 0;
            viewPanel.Width = 0;

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

                Point itemPos = Mouse.GetPosition(viewPanel);
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

        protected virtual void scrollViewer_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            if (ScrollChanged != null)
            {
                scroll.ScrollToHorizontalOffset(Math.Floor(scroll.HorizontalOffset));
                scroll.ScrollToVerticalOffset(Math.Floor(scroll.VerticalOffset));
                ScrollChanged(this, e);
            }
        }
        protected virtual void viewPanel_MouseMove(object sender, MouseEventArgs e)
        {
            try
            {
                if (sender is PanelBase)
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
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
        protected virtual void viewPanel_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            try
            {
                lastDownMousePos = Mouse.GetPosition(null);
                lastDownHOffset = scroll.HorizontalOffset;
                lastDownVOffset = scroll.VerticalOffset;
                viewPanel.CaptureMouse();
                isDrag = true;
                isDragMoved = false;

                if (e.ClickCount == 2)
                {
                    Point cursorPos = Mouse.GetPosition(viewPanel);
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
        protected virtual void viewPanel_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            try
            {
                viewPanel.ReleaseMouseCapture();
                isDrag = false;
                if (isDragMoved == false)
                {
                    if (IsSingleClickOpen == true)
                    {
                        Point cursorPos = Mouse.GetPosition(viewPanel);
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

        protected virtual void viewPanel_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            viewPanel.ReleaseMouseCapture();
            isDrag = false;
            lastDownMousePos = Mouse.GetPosition(null);
            lastDownHOffset = scroll.HorizontalOffset;
            lastDownVOffset = scroll.VerticalOffset;
            if (e.ClickCount == 1)
            {
                Point cursorPos = Mouse.GetPosition(viewPanel);
                if (RightClick != null)
                {
                    RightClick(sender, cursorPos);
                }
            }
        }
        protected virtual void viewPanel_MouseLeave(object sender, MouseEventArgs e)
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
                    TimeSpan RemainTime = TimeSpan.FromSeconds(Settings.Instance.DisplayNotifyJumpTime);
                    int Brinks = 3;
                    bool IsDisplay = false;
                    notifyTimer.Tick += (sender, e) =>
                    {
                        RemainTime -= notifyTimer.Interval;
                        if (RemainTime <= TimeSpan.FromSeconds(0))
                        {
                            cnvs.Children.Remove(rect);
                            notifyTimer.Stop();
                        }
                        else if (IsDisplay == false)
                        {
                            cnvs.Children.Add(rect);
                            IsDisplay = true;
                        }
                        else if (Brinks > 0)
                        {
                            cnvs.Children.Remove(rect);
                            IsDisplay = false;
                            Brinks--;
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
