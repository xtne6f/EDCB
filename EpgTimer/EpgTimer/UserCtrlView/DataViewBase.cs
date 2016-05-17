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
        protected string[] status = { "", "", "", "" };
        protected bool ReloadInfo = true;

        public virtual void UpdateInfo(bool reload = true)
        {
            ReloadInfo |= reload;
            if (ReloadInfo == true && this.IsVisible == true)
            {
                ReloadInfo = !ReloadInfoData();
                UpdateStatus();
            }
        }
        protected virtual bool ReloadInfoData() { return true; }
        protected void UpdateStatus(int mode = 0)
        {
            if (Settings.Instance.DisplayStatus == false) return;
            UpdateStatusData(mode);
            RefreshStatus();
        }
        protected virtual void UpdateStatusData(int mode = 0) { }
        protected void RefreshStatus(bool force = false)
        {
            if (this.IsVisible == true || force == true)
            {
                CommonManager.Instance.StatusSet(status[1], status[2]);
            }
        }
        protected virtual void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            UpdateInfo(false);
            RefreshStatus();
        }
    }
}
