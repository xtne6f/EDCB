using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

namespace EpgTimer.UserCtrlView
{
    public class HideableWindowSet
    {
        public double Top = -100;
        public double Left = -100;
        public double Width = -100;
        public double Height = -100;
        public bool Pinned = true;

        public void SetToWindow<T>(HideableWindow<T> win)
        {
            if (Top != -100) win.Top = Top;
            if (Left != -100) win.Left = Left;
            if (Width > 0) win.Width = Width;
            if (Height > 0) win.Height = Height;
            win.Pinned = Pinned;
        }
        public void GetFromWindow<T>(HideableWindow<T> win, bool force = false)
        {
            if (force == true || win.Visibility == Visibility.Visible && win.WindowState == WindowState.Normal)
            {
                Top = win.Top;
                Left = win.Left;
                Width = win.Width;
                Height = win.Height;
            }
            Pinned = win.Pinned;
        }
    }

    //HideableWindow全体管理用
    public class HideableWindowBase : Window
    {
        protected static MainWindow mainWindow { get { return ViewUtil.MainWindow; } }
        protected static MenuManager mm { get { return CommonManager.Instance.MM; } }
        protected MenuBinds mBinds = new MenuBinds();
    }
    public class HideableWindow<T> : HideableWindowBase
    {
        public static void RefreshMenus()
        {
            foreach (var win in Application.Current.Windows.OfType<HideableWindow<T>>())
            {
                win.RefreshMenu();
            }
        }
        public virtual void RefreshMenu() { }

        //リロード関係
        protected bool ReloadInfo = false;
        protected virtual void ReloadInfoData() { }
        public static void UpdatesInfo(bool reload = true)
        {
            foreach (var win in Application.Current.Windows.OfType<HideableWindow<T>>())
            {
                win.UpdateInfo(reload);
            }
        }
        public virtual void UpdateInfo(bool reload = true)
        {
            ReloadInfo |= reload;
            ReloadInfoData();
        }
        protected virtual void Window_Activated(object sender, EventArgs e)
        {
            ReloadInfoData();
        }

        /// <summary>番組表などへジャンプした際に最小化したWindow</summary>
        protected static string buttonID = "[]";
        protected static HideableWindow<T> hideWindow = null;
        public static bool HasHideWindow { get { return hideWindow != null; } }
        protected static void SetHideWindow(HideableWindow<T> win)
        {
            // 情報を保持は最新のもの1つだけ
            hideWindow = win;
            mainWindow.EmphasizeButton(HasHideWindow, buttonID);
        }
        public static void RestoreHideWindow()
        {
            // 最小化したWindowを復帰
            if (HasHideWindow == true)
            {
                hideWindow.Show();
                hideWindow.WindowState = WindowState.Normal;
            }
        }
        protected void JumpTabAndHide(CtxmCode code, object item)
        {
            if (mainWindow.IsVisible == false || mainWindow.WindowState == WindowState.Minimized)
            {
                mainWindow.RestoreMinimizedWindow();
            }
            mainWindow.Dispatcher.BeginInvoke(new Action(() =>
            {
                SetHideWindow(this);
                MinimizeWindows();

                BlackoutWindow.SelectedData = item;
                mainWindow.moveTo_tabItem(code);
            }));
        }
        public static void MinimizeWindows()
        {
            foreach (var win in Application.Current.Windows.OfType<HideableWindow<T>>())
            {
                win.WindowState = WindowState.Minimized;
            }
        }

        protected virtual void Window_Closed(object sender, EventArgs e)
        {
            if (hideWindow == this) SetHideWindow(null);

            //フォーカスがおかしくなるときがあるので、とりあえずの対応
            if (Application.Current.Windows.OfType<HideableWindowBase>().Count() == 0)
            {
                mainWindow.Activate();
            }

            if (AllClosing == false)
            {
                Settings.SaveToXmlFile();//検索ワード、ウィンドウ位置の保存
                if (mainWindow.IsActive == true)
                {
                    mainWindow.ListFoucsOnVisibleChanged();
                }
            }
        }
        protected static bool AllClosing = false;
        public static void CloseWindows(bool IsSave = false)
        {
            AllClosing = true;

            foreach (var win in Application.Current.Windows.OfType<HideableWindow<T>>())
            {
                win.Close();
            }

            if (IsSave == true) Settings.SaveToXmlFile();

            AllClosing = false;
        }
        protected void Window_StateChanged(object sender, EventArgs e)
        {
            if (this.WindowState != WindowState.Minimized)
            {
                if (hideWindow == this) SetHideWindow(null);
            }
        }

        //前面表示関係
        protected CheckBox chkboxPinned;
        public bool Pinned
        { 
            get { return chkboxPinned.IsChecked == true; }
            set { chkboxPinned.IsChecked = value; }
        }
        public static void UpdatesParentStatus()
        {
            foreach (var win in Application.Current.Windows.OfType<HideableWindow<T>>())
            {
                win.UpdateParentStatus();
            }
        }
        public void UpdateParentStatus()
        {
            checkBox_windowPinned_Checked(chkboxPinned, null);
        }
        protected void checkBox_windowPinned_Checked(object sender, RoutedEventArgs e)
        {
            this.Owner = (sender as CheckBox).IsChecked == true && mainWindow.IsVisible == true ? mainWindow : null;
        }
    }
}
