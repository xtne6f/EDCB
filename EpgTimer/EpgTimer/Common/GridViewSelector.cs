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
        Func<List<ListColumnInfo>> getResetColumns = null;
        Func<List<ListColumnInfo>> getInitColumns = null;
        private bool CanResetColumns { get { return getResetColumns != null; } }
        private bool CanInitColumns { get { return getInitColumns != null; } }

        private enum gvsCmds { Reset, Init, All, Clear, Delete }
        private Dictionary<gvsCmds, MenuItem> cmdsMenu = new Dictionary<gvsCmds, MenuItem>();

        public GridViewSelector(GridView gv, Func<List<ListColumnInfo>> setting = null, Func<List<ListColumnInfo>> initColumns = null)
        {
            try
            {
                gridView = gv;
                getResetColumns = setting;
                getInitColumns = initColumns;

                //セレクト用の右クリックメニュー関係
                gridView.ColumnHeaderContextMenu = new ContextMenu();
                gridView.ColumnHeaderContextMenu.Opened += new RoutedEventHandler(ContextMenuOpening);

                //カラム操作用メニュー追加
                Action<ItemsControl, gvsCmds, string> columnMenuAdd = (menu, cmds, header) =>
                {
                    var menu1 = new MenuItem { Header = header, Tag = cmds };
                    menu1.Click += new RoutedEventHandler(HeaderCmdsExecute);
                    menu.Items.Add(menu1);
                    cmdsMenu.Add(cmds, menu1);
                };

                columnMenuAdd(gridView.ColumnHeaderContextMenu, gvsCmds.Delete, "このカラムを非表示(_V)");

                var menu_cl = new MenuItem { Header = "カラムの操作(_E)" };
                columnMenuAdd(menu_cl, gvsCmds.All, "全カラムを表示(_A)");
                columnMenuAdd(menu_cl, gvsCmds.Clear, "全カラムを非表示(_C)");
                columnMenuAdd(menu_cl, gvsCmds.Reset, "保存されている状態に戻す(_Z)");
                columnMenuAdd(menu_cl, gvsCmds.Init, "表示カラムを初期化(_I)");

                cmdsMenu[gvsCmds.Reset].Visibility = CanResetColumns == true ? Visibility.Visible : Visibility.Collapsed;
                cmdsMenu[gvsCmds.Init].Visibility = CanInitColumns == true ? Visibility.Visible : Visibility.Collapsed;

                gridView.ColumnHeaderContextMenu.Items.Add(menu_cl);
                gridView.ColumnHeaderContextMenu.Items.Add(new Separator());

                //各項目追加
                foreach (GridViewColumn info in gridView.Columns)
                {
                    var header = info.Header as GridViewColumnHeader;

                    //セレクト用のメニュー生成
                    var menu = new MenuItem();
                    menu.Uid = header.Uid;
                    menu.Tag = header.Tag == null ? header.Content : header.Tag.ToString();
                    menu.IsCheckable = true;
                    menu.Click += new RoutedEventHandler(HeaderSelectClick);
                    gridView.ColumnHeaderContextMenu.Items.Add(menu);

                    columnList.Add(menu.Uid, info);
                }

                //表示列の初期化
                cmdsMenu[gvsCmds.Reset].RaiseEvent(new RoutedEventArgs(MenuItem.ClickEvent, cmdsMenu[gvsCmds.Reset]));
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        private void HeaderSelectClick(object sender, RoutedEventArgs e)
        {
            try
            {
                GridViewColumn select = columnList[(sender as MenuItem).Uid];
                if (gridView.Columns.Contains(select) == false)
                {
                    gridView.Columns.Add(select);
                }
                else
                {
                    gridView.Columns.Remove(select);
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        private void ResetGridViewColumns(IEnumerable<ListColumnInfo> setting)
        {
            if (setting != null)
            {
                gridView.Columns.Clear();
                foreach (ListColumnInfo info in setting)
                {
                    if (info != null && columnList.ContainsKey(info.Tag))
                    {
                        columnList[info.Tag].Width = info.Width;
                        if (gridView.Columns.Contains(columnList[info.Tag]) == false)
                        {
                            gridView.Columns.Add(columnList[info.Tag]);
                        }
                    }
                }
            }
        }

        private void HeaderCmdsExecute(object sender, RoutedEventArgs e)
        {
            try
            {
                switch ((sender as MenuItem).Tag as gvsCmds?)
                {
                    case gvsCmds.Reset:
                        if (CanResetColumns == true) ResetGridViewColumns(getResetColumns());
                        break;
                    case gvsCmds.Init:
                        if (CanInitColumns == true) ResetGridViewColumns(getInitColumns());
                        break;
                    case gvsCmds.All:
                        var list = columnList.Values.Where(v => gridView.Columns.Contains(v) == false);
                        foreach (var val in list) gridView.Columns.Add(val);
                        break;
                    case gvsCmds.Clear:
                        gridView.Columns.Clear();
                        break;
                    case gvsCmds.Delete:
                        var trg = (sender as MenuItem).DataContext as MenuItem;
                        if (trg != null) trg.RaiseEvent(e);
                        break;
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        private void ContextMenuOpening(object sender, RoutedEventArgs e)
        {
            try
            {
                var ctxm =(sender as ContextMenu);
                var trg = ctxm.PlacementTarget as GridViewColumnHeader ?? new GridViewColumnHeader();

                //非表示用メニューの設定
                var menu_cmd = ctxm.Items[0] as MenuItem;
                menu_cmd.DataContext = null;

                foreach (var menuItem in ctxm.Items.OfType<MenuItem>().Where(m => columnList.ContainsKey(m.Uid) == true))
                {
                    menuItem.IsChecked = gridView.Columns.Contains(columnList[menuItem.Uid]);
                    menuItem.Header = menuItem.Tag;
                    if (menuItem.Uid == trg.Uid)
                    {
                        menuItem.Header = new TextBlock { Text = menuItem.Tag as string, FontWeight = FontWeights.Bold };
                        menu_cmd.DataContext = menuItem;
                    }
                }

                cmdsMenu[gvsCmds.Delete].IsEnabled = menu_cmd.DataContext != null;
                cmdsMenu[gvsCmds.All].IsEnabled = gridView.Columns.Count != columnList.Count;
                cmdsMenu[gvsCmds.Clear].IsEnabled = gridView.Columns.Count != 0;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public void SaveSize()
        {
            try
            {
                if (CanResetColumns == false) return;

                List<ListColumnInfo> setting = getResetColumns();
                if (setting == null) return;

                setting.Clear();
                foreach (GridViewColumn info in gridView.Columns)
                {
                    var header = info.Header as GridViewColumnHeader;
                    setting.Add(new ListColumnInfo(header.Uid, info.Width));
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

    }
}
