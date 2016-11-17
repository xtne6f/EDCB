using System;
using System.Windows;
using System.Windows.Controls;

namespace EpgTimer.UserCtrlView
{
    public class DataViewBase : UserControl
    {
        protected static MenuManager mm { get { return CommonManager.Instance.MM; } }
        protected MenuBinds mBinds = new MenuBinds();
        protected string[] status = { "", "", "", "" };
        protected bool ReloadInfoFlg = true;

        public virtual void UpdateInfo(bool reload = true)
        {
            ReloadInfoFlg |= reload;
            ReloadInfo();
        }
        protected virtual void ReloadInfo()
        {
            if (ReloadInfoFlg == true && this.IsVisible == true)
            {
                ReloadInfoFlg = !ReloadInfoData();
                UpdateSelectViewItem();
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
                StatusManager.StatusSet(status[1], status[2], target:this);
            }
        }
        protected virtual void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            ReloadInfo();
            RefreshStatus();
        }

        //選択アイテムの更新関係
        public void SelectViewItem(UInt64 id, bool force = false)
        {
            if (this.IsVisible == true || force == true)
            {
                SelectViewItemData(id);
            }
        }
        protected virtual void SelectViewItemData(UInt64 id) { }     //DialogWindow -> ListViewView
        protected virtual void UpdateSelectViewItem() { }   //ListViewView -> DialogWindow  ( ->ListViewView )
    }
}
