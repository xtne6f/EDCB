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

        public NotifyLogWindow()
        {
            InitializeComponent();
            textBox_logMax.Text = Settings.Instance.NotifyLogMax.ToString();
        }

        private void ReloadList()
        {
            string notifyLog = "";
            if (CommonManager.CreateSrvCtrl().SendGetNotifyLog(Math.Max(Settings.Instance.NotifyLogMax, 1), ref notifyLog) == ErrCode.CMD_SUCCESS)
            {
                //サーバに保存されたログを使う
                listView_log.ItemsSource = notifyLog.Split(new char[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries).Select(text => new NotifySrvInfoItem(text)).ToList();
                textBox_logMax.IsEnabled = true;
            }
            else
            {
                //クライアントで蓄積したログを使う
                listView_log.ItemsSource = CommonManager.Instance.NotifyLogList.Select(info => new NotifySrvInfoItem(info)).ToList();
                textBox_logMax.IsEnabled = false;
            }

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
                ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_log.ItemsSource);

                dataView.SortDescriptions.Clear();

                SortDescription sd = new SortDescription(sortBy, direction);
                dataView.SortDescriptions.Add(sd);
                if (_lastHeaderClicked2 != null)
                {
                    if (sortBy != _lastHeaderClicked2)
                    {
                        SortDescription sd2 = new SortDescription(_lastHeaderClicked2, _lastDirection2);
                        dataView.SortDescriptions.Add(sd2);
                    }
                }
                dataView.Refresh();
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
                    if (header != _lastHeaderClicked)
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

            if (listView_log.ItemsSource != null && dlg.ShowDialog() == true)
            {
                using (var file = new StreamWriter(dlg.FileName, false, Encoding.Unicode))
                {
                    foreach (NotifySrvInfoItem info in listView_log.ItemsSource)
                    {
                        file.WriteLine(info);
                    }
                }
            }
        }

        private void textBox_logMax_TextChanged(object sender, TextChangedEventArgs e)
        {
            int logMax;
            int.TryParse(textBox_logMax.Text, out logMax);
            Settings.Instance.NotifyLogMax = logMax;
        }
    }
}
