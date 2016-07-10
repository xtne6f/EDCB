using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Input;
using System.ComponentModel;
using System.IO;

namespace EpgTimer
{
    /// <summary>
    /// NotifyLogWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class NotifyLogWindow : Window
    {
        private ListViewController<NotifySrvInfoItem> lstCtrl;
        public NotifyLogWindow()
        {
            InitializeComponent();

            this.KeyDown += ViewUtil.KeyDown_Escape_Close();

            //リストビュー関連の設定
            lstCtrl = new ListViewController<NotifySrvInfoItem>(this);
            lstCtrl.SetInitialSortKey(CommonUtil.NameOf(() => (new NotifySrvInfoItem()).Time), ListSortDirection.Descending);
            lstCtrl.SetViewSetting(listView_log, gridView_log, false, true);
        }
        private bool ReloadList()
        {
            return lstCtrl.ReloadInfoData(dataList =>
            {
                dataList.AddRange(CommonManager.Instance.NotifyLogList.Select(info => new NotifySrvInfoItem(info)));
                return true;
            });
        }
        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            ReloadList();
        }
        private void button_clear_Click(object sender, RoutedEventArgs e)
        {
            CommonManager.Instance.NotifyLogList.Clear();
            ReloadList();
        }
        private void button_save_Click(object sender, RoutedEventArgs e)
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
    }
}
