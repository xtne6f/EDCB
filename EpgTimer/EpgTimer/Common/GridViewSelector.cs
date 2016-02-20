using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;

namespace EpgTimer
{
    public class GridViewSelector
    {
        GridView gridView = null;
        Dictionary<String, GridViewColumn> columnList = new Dictionary<String, GridViewColumn>();   //カラムの一覧を保存しておく

        public GridViewSelector(GridView gv, List<ListColumnInfo> setting = null)
        {
            try
            {
                gridView = gv;

                //セレクト用の右クリックメニュー関係
                gridView.ColumnHeaderContextMenu = new ContextMenu();
                gridView.ColumnHeaderContextMenu.Opened += new RoutedEventHandler(ContextMenuOpening);
                foreach (GridViewColumn info in gridView.Columns)
                {
                    var header = info.Header as GridViewColumnHeader;

                    //セレクト用のメニュー生成
                    var menu = new MenuItem();
                    menu.Uid = header.Uid;
                    menu.Header = header.Tag == null ? header.Content : header.Tag.ToString();
                    menu.IsCheckable = true;
                    menu.Click += new RoutedEventHandler(HeaderSelectClick);
                    gridView.ColumnHeaderContextMenu.Items.Add(menu);

                    columnList.Add(menu.Uid, info);
                }

                //表示列の初期化
                if (setting != null)
                {
                    gridView.Columns.Clear();
                    foreach (ListColumnInfo info in setting)
                    {
                        if (columnList.ContainsKey(info.Tag))
                        {
                            columnList[info.Tag].Width = info.Width;
                            if (!gridView.Columns.Contains(columnList[info.Tag]))
                            {
                                gridView.Columns.Add(columnList[info.Tag]);
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void HeaderSelectClick(object sender, RoutedEventArgs e)
        {
            try
            {
                MenuItem menuItem = sender as MenuItem;
                if (menuItem.IsChecked == true)
                {
                    gridView.Columns.Add(columnList[menuItem.Uid]);
                }
                else
                {
                    gridView.Columns.Remove(columnList[menuItem.Uid]);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void ContextMenuOpening(object sender, RoutedEventArgs e)
        {
            try
            {
                foreach (MenuItem menuItem in (sender as ContextMenu).Items)
                {
                    menuItem.IsChecked = gridView.Columns.Contains(columnList[menuItem.Uid]);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void SaveSize(List<ListColumnInfo> setting)
        {
            try
            {
                if (setting == null) return;

                setting.Clear();
                foreach (GridViewColumn info in gridView.Columns)
                {
                    GridViewColumnHeader header = info.Header as GridViewColumnHeader;
                    setting.Add(new ListColumnInfo(header.Uid, info.Width));
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

    }
}
