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
using System.Windows.Shapes;
using System.ComponentModel;
using System.Collections.ObjectModel;
using System.Collections;
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

            //リストビュー関連の設定
            lstCtrl = new ListViewController<NotifySrvInfoItem>(this);
            lstCtrl.SetInitialSortKey(CommonUtil.GetMemberName(() => (new NotifySrvInfoItem()).Time), ListSortDirection.Descending);
            lstCtrl.SetViewSetting(listView_log, gridView_log, false, true);
        }

        private bool ReloadList()
        {
            return lstCtrl.ReloadInfoData(dataList =>
            {
                foreach (NotifySrvInfo info in CommonManager.Instance.NotifyLogList)
                {
                    NotifySrvInfoItem item = new NotifySrvInfoItem();
                    item.NotifyInfo = info;
                    dataList.Add(item);
                }
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
            Microsoft.Win32.SaveFileDialog dlg = new Microsoft.Win32.SaveFileDialog();
            dlg.DefaultExt = ".txt";
            dlg.Filter = "txt Files (.txt)|*.txt;|all Files(*.*)|*.*";

            Nullable<bool> result = dlg.ShowDialog();
            if (result == true)
            {
                StreamWriter file = new StreamWriter(dlg.FileName, false, System.Text.Encoding.GetEncoding("shift_jis") );
                lstCtrl.dataList.ForEach(info => file.Write(info.FileLogText));
                file.Close();
            }
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.Escape:
                        this.Close();
                        break;
                }
            }
            base.OnKeyDown(e);
        }

    }
}
