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
        List<ListColumnInfo> settingList = new List<ListColumnInfo>();                              //設定情報

        public GridViewSelector(GridView gv, List<ListColumnInfo> setting = null)
        {
            try
            {
                gridView = gv;
                foreach (GridViewColumn info in gridView.Columns)
                {
                    GridViewColumnHeader header = info.Header as GridViewColumnHeader;
                    columnList.Add((string)header.Tag, info);
                }

                if (gridView.ColumnHeaderContextMenu != null)
                {
                    gridView.ColumnHeaderContextMenu.Opened += new RoutedEventHandler(ContextMenuOpening);
                    foreach (MenuItem menu in gridView.ColumnHeaderContextMenu.Items)
                    {
                        menu.Click += new RoutedEventHandler(HeaderSelectClick);
                    }
                }

                if (setting == null) return;

                //設定情報がある場合は続ける
                settingList.AddRange(setting);
                gridView.Columns.Clear();
                foreach (ListColumnInfo info in settingList)
                {
                    columnList[info.Tag].Width = info.Width;
                    gridView.Columns.Add(columnList[info.Tag]);
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
                    gridView.Columns.Add(columnList[menuItem.Name]);
                    settingList.Add(new ListColumnInfo(menuItem.Name, Double.NaN));
                }
                else
                {
                    foreach (ListColumnInfo info in settingList)
                    {
                        if (info.Tag.CompareTo(menuItem.Name) == 0)
                        {
                            gridView.Columns.Remove(columnList[menuItem.Name]);
                            settingList.Remove(info);
                            break;
                        }
                    }
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
                foreach (MenuItem item in (sender as ContextMenu).Items)
                {
                    item.IsChecked = settingList.Exists(setinfo => setinfo.Tag == item.Name);
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
                    setting.Add(new ListColumnInfo((String)header.Tag, info.Width));
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

    }
}
