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

            if (Settings.Instance.NoStyle == 0)
            {
                ResourceDictionary rd = new ResourceDictionary();
                rd.MergedDictionaries.Add(
                    Application.LoadComponent(new Uri("/PresentationFramework.Aero, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/aero.normalcolor.xaml", UriKind.Relative)) as ResourceDictionary
                    //Application.LoadComponent(new Uri("/PresentationFramework.Classic, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35, ProcessorArchitecture=MSIL;component/themes/Classic.xaml", UriKind.Relative)) as ResourceDictionary
                    );
                this.Resources = rd;
            }
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
            listView_log.DataContext = logList;

            if (this.gridViewSorter.IsExistSortParams)
            {
                this.gridViewSorter.SortByMultiHeader(this.logList);
            }
            else
            {
                this.gridViewSorter.ResetSortParams();
                this.gridViewSorter.SortByMultiHeaderWithKey(this.logList, gridView_log.Columns, "Time", true, ListSortDirection.Descending);
            }
            listView_log.Items.Refresh();
        }

        GridViewSorter<NotifySrvInfoItem> gridViewSorter = new GridViewSorter<NotifySrvInfoItem>();

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked = e.OriginalSource as GridViewColumnHeader;
            if (headerClicked != null)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {
                    this.gridViewSorter.SortByMultiHeader(this.logList, headerClicked);
                    listView_log.Items.Refresh();
                }
            }
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
