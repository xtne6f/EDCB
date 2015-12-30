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
        string _lastHeaderClicked = null;
        ListSortDirection _lastDirection = ListSortDirection.Ascending;
        string _lastHeaderClicked2 = null;
        ListSortDirection _lastDirection2 = ListSortDirection.Ascending;

        List<NotifySrvInfoItem> logList = new List<NotifySrvInfoItem>();
        public NotifyLogWindow()
        {
            InitializeComponent();
        }

        private void ReloadList()
        {
            ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_log.DataContext);
            if (dataView != null)
            {
                dataView.SortDescriptions.Clear();
                dataView.Refresh();
            }

            listView_log.DataContext = null;
            logList.Clear();
            foreach (NotifySrvInfo info in CommonManager.Instance.NotifyLogList)
            {
                NotifySrvInfoItem item = new NotifySrvInfoItem();
                item.NotifyInfo = info;
                logList.Add(item);
            }
            listView_log.DataContext = logList;

            if (_lastHeaderClicked != null)
            {
                Sort(_lastHeaderClicked, _lastDirection);
            }
            else
            {
                string header = ((Binding)gridView_log.Columns[0].DisplayMemberBinding).Path.Path;
                Sort(header, _lastDirection);
                _lastHeaderClicked = header;
            }
        }

        private void Sort(string sortBy, ListSortDirection direction)
        {
            try
            {
                ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_log.DataContext);

                dataView.SortDescriptions.Clear();

                SortDescription sd = new SortDescription(sortBy, direction);
                dataView.SortDescriptions.Add(sd);
                if (_lastHeaderClicked2 != null)
                {
                    if (String.Compare(sortBy, _lastHeaderClicked2) != 0)
                    {
                        SortDescription sd2 = new SortDescription(_lastHeaderClicked2, _lastDirection2);
                        dataView.SortDescriptions.Add(sd2);
                    }
                }
                dataView.Refresh();

                Settings.Instance.ResColumnHead = sortBy;
                Settings.Instance.ResSortDirection = direction;

            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked = e.OriginalSource as GridViewColumnHeader;
            ListSortDirection direction;

            if (headerClicked != null)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {
                    string header = ((Binding)headerClicked.Column.DisplayMemberBinding).Path.Path;
                    if (String.Compare(header, _lastHeaderClicked) != 0)
                    {
                        direction = ListSortDirection.Ascending;
                        _lastHeaderClicked2 = _lastHeaderClicked;
                        _lastDirection2 = _lastDirection;
                    }
                    else
                    {
                        if (_lastDirection == ListSortDirection.Ascending)
                        {
                            direction = ListSortDirection.Descending;
                        }
                        else
                        {
                            direction = ListSortDirection.Ascending;
                        }
                    }

                    Sort(header, direction);

                    _lastHeaderClicked = header;
                    _lastDirection = direction;
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
                foreach (NotifySrvInfoItem info in logList)
                {
                    file.Write(info.FileLogText);
                }
                file.Close();
            }
        }
    }
}
