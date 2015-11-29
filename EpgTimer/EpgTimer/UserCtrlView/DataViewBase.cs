using System;
using System.Windows;
using System.Windows.Controls;

namespace EpgTimer.UserCtrlView
{
    public class DataViewBase : UserControl
    {
        protected MenuUtil mutil = CommonManager.Instance.MUtil;
        protected ViewUtil vutil = CommonManager.Instance.VUtil;
        protected MenuManager mm = CommonManager.Instance.MM;
        protected MenuBinds mBinds = new MenuBinds();
        protected bool ReloadInfo = true;

        public virtual void UpdateInfo()
        {
            ReloadInfo = true;
            if (ReloadInfo == true && this.IsVisible == true) ReloadInfo = !ReloadInfoData();
        }
        protected virtual void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (ReloadInfo == true && this.IsVisible == true) ReloadInfo = !ReloadInfoData();
        }
        protected virtual bool ReloadInfoData() { return true; }

    }
}
