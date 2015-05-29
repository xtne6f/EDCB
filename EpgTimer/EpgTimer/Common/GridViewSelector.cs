using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;

namespace EpgTimer
{
    class GridViewSelector
    {
        GridView gridView = null;
        Dictionary<String, GridViewColumn> columnList = null;   //カラムの一覧を保存しておく
        List<ListColumnInfo> settingList = null;                //設定情報

        public GridViewSelector(GridView gv, List<ListColumnInfo> setting)
        {
            try
            {
                gridView = gv;
                columnList = new Dictionary<String, GridViewColumn>();
                //↓本当はこれじゃダメだけど、今は大丈夫
                settingList = (setting != null ? setting : new List<ListColumnInfo>());

                foreach (GridViewColumn info in gridView.Columns)
                {
                    GridViewColumnHeader header = info.Header as GridViewColumnHeader;
                    columnList.Add((string)header.Tag, info);
                }

                if (setting == null) return;

                //設定情報がある場合は続ける
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

        public void HeaderSelectClick(object sender, RoutedEventArgs e)
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

        public void ContextMenuOpening(object sender, RoutedEventArgs e)
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

        public void SaveSize()
        {
            try
            {
                settingList.Clear();
                foreach (GridViewColumn info in gridView.Columns)
                {
                    GridViewColumnHeader header = info.Header as GridViewColumnHeader;
                    settingList.Add(new ListColumnInfo((String)header.Tag, info.Width));
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

    }
}
