using System;
using System.Linq;
using System.Windows;
using System.ComponentModel;
using System.IO;
using System.Windows.Controls;

namespace EpgTimer
{
    /// <summary>
    /// NotifyLogWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class NotifyLogWindow : NotifyLogWindowBase
    {
        private ListViewController<NotifySrvInfoItem> lstCtrl;
        public NotifyLogWindow()
        {
            InitializeComponent();

            try
            {
                base.SetParam(false, checkBox_windowPinned);

                this.KeyDown += ViewUtil.KeyDown_Escape_Close();

                this.Loaded += (sender, e) => UpdateInfo();
                this.button_reload.Click += (sender, e) => ReloadInfoData();
                this.button_clear.Click += (sender, e) =>
                {
                    CommonManager.Instance.NotifyLogList.Clear();
                    ReloadInfoData();
                };

                //リストビュー関連の設定
                lstCtrl = new ListViewController<NotifySrvInfoItem>(this);
                lstCtrl.SetInitialSortKey(CommonUtil.NameOf(() => (new NotifySrvInfoItem()).Time), ListSortDirection.Descending);
                lstCtrl.SetViewSetting(listView_log, gridView_log, false, true);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        protected override bool ReloadInfoData()
        {
            return lstCtrl.ReloadInfoData(dataList =>
            {
                dataList.AddRange(CommonManager.Instance.NotifyLogList.Select(info => new NotifySrvInfoItem(info)));
                return true;
            });
        }
        private void button_save_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var dlg = new Microsoft.Win32.SaveFileDialog();
                dlg.DefaultExt = ".txt";
                dlg.Filter = "txt Files (.txt)|*.txt;|all Files(*.*)|*.*";
                if (dlg.ShowDialog() == true)
                {
                    using (var file = new StreamWriter(dlg.FileName))
                    {
                        lstCtrl.dataList.ForEach(info => file.Write(info.FileLogText));
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        private void checkBox_autoReload_Checked(object sender, RoutedEventArgs e)
        {
            Settings.Instance.NotifyWindowAutoReload = checkBox_autoReload.IsChecked == true;
            if (Settings.Instance.NotifyWindowAutoReload == true) UpdateInfo();
        }
    }
    public class NotifyLogWindowBase : AttendantDataWindow<NotifyLogWindow> { }
}
