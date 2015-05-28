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
        List<NotifySrvInfoItem> logList = new List<NotifySrvInfoItem>();
        public NotifyLogWindow()
        {
            InitializeComponent();
        }

        private void ReloadList()
        {
            listView_log.DataContext = null;
            logList.Clear();
            foreach (NotifySrvInfo info in CommonManager.Instance.NotifyLogList)
            {
                NotifySrvInfoItem item = new NotifySrvInfoItem();
                item.NotifyInfo = info;
                logList.Add(item);
            }

            if (this.gridViewSorter != null)
            {
                this.gridViewSorter.SortByMultiHeader(this.logList);
            }
            else
            {
                this.gridViewSorter = new GridViewSorter<NotifySrvInfoItem>();
                this.gridViewSorter.SortByMultiHeaderWithKey(this.logList, gridView_log.Columns, "Time", true, ListSortDirection.Descending);
            }

            listView_log.DataContext = logList;
        }

        GridViewSorter<NotifySrvInfoItem> gridViewSorter = null;
        private ViewUtil vutil = CommonManager.Instance.VUtil;

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            vutil.GridViewHeaderClickSort<NotifySrvInfoItem>(e, gridViewSorter, logList, listView_log);
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
                logList.ForEach(info => file.Write(info.FileLogText));
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
