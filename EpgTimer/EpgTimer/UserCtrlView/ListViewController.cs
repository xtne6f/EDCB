using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.ComponentModel;

using System.Reflection;

namespace EpgTimer
{
    public class GridViewColumnList : List<GridViewColumn> { }

    public class ListViewController<T> where T : class
    {
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private ViewUtil vutil = CommonManager.Instance.VUtil;

        public GridViewSelector gvSelector { get; set; }
        public GridViewSorter<T> gvSorter { get; set; }
        public List<T> dataList { get; set; }

        private ListBoxItem ClickTarget = null;

        private string column_SavePath = null;
        private string sort_HeaderSavePath = null;
        private string sort_DirectionSavePath = null;
        private bool IsSortViewOnReload = false;

        private string initialSortKey = null;
        private ListSortDirection initialDirection = ListSortDirection.Ascending;

        private ListView listView = null;
        private GridView gridView = null;

        private Control Owner;

        public ListViewController(Control owner)
        {
            Owner = owner;
            dataList = new List<T>();
        }

        public void SetSavePath(string columnSavePath, string sortHeaderSavePath = null, string sortDirectionSavePath = null)
        {
            column_SavePath = columnSavePath;
            sort_HeaderSavePath = sortHeaderSavePath;
            sort_DirectionSavePath = sortDirectionSavePath;

            initialSortKey = Settings.Instance.GetSettings(sort_HeaderSavePath) as string;
            object direction = Settings.Instance.GetSettings(sort_DirectionSavePath);
            if (direction != null) initialDirection = (ListSortDirection)direction;
        }
        public void SetInitialSortKey(string sortKsy, ListSortDirection sortDirection = ListSortDirection.Ascending)
        {
            initialSortKey = sortKsy;
            initialDirection = sortDirection;
        }
        public void SetViewSetting(ListView lv, GridView gv, bool isSortOnReload
            , List<GridViewColumn> cols_source = null, RoutedEventHandler headerclick = null)
        {
            listView = lv;
            gridView = gv;
            IsSortViewOnReload = isSortOnReload;
            if (IsSortViewOnReload == false)
            {
                this.gvSorter = new GridViewSorter<T>();
            }

            //グリッド列の差し込み。クリアせずに追加する。
            if (cols_source != null)
            {
                cols_source.ForEach(col => gridView.Columns.Add(col));
            }
            
            var hclick = headerclick != null ? headerclick : (sender, e) => this.GridViewHeaderClickSort(e);
            foreach (GridViewColumn info in gridView.Columns)
            {
                var header = info.Header as GridViewColumnHeader;
                header.Click += new RoutedEventHandler(hclick);
                if (header.ToolTip == null)
                {
                    header.ToolTip = "Ctrl+Click(マルチ・ソート)、Shift+Click(解除)";
                }
            }

            gvSelector = new GridViewSelector(gv, this.columnSaveList);

            //アイテムの無い場所でクリックしたとき、選択を解除する。
            listView.MouseLeftButtonUp += new MouseButtonEventHandler((sender, e) =>
            {
                if (listView.InputHitTest(e.GetPosition(listView)) is ScrollViewer)//本当にこれで良いのだろうか？
                {
                    listView.UnselectAll();
                }
            });

            //Escapeキーで選択を解除する。
            listView.KeyDown += new KeyEventHandler((sender, e) =>
            {
                if (Keyboard.Modifiers == ModifierKeys.None)
                {
                    switch (e.Key)
                    {
                        case Key.Escape:
                            if (listView.SelectedItem != null)
                            {
                                listView.UnselectAll();
                                e.Handled = true;
                            }
                            break;
                    }
                }
            });
        }

        public void SaveViewDataToSettings()
        {
            try
            {
                gvSelector.SaveSize(this.columnSaveList);
                if (gvSorter != null)
                {
                    Settings.Instance.SetSettings(sort_HeaderSavePath, this.gvSorter.LastHeader);
                    Settings.Instance.SetSettings(sort_DirectionSavePath, this.gvSorter.LastDirection);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
        private List<ListColumnInfo> columnSaveList
        {
            get { return Settings.Instance.GetSettings(column_SavePath) as List<ListColumnInfo>; }
        }

        public bool ReloadInfoData(Func<List<T>, bool> reloadData)
        {
            try
            {
                //更新前の選択情報の保存
                var oldItems = new ListViewSelectedKeeper(listView, true);

                listView.ItemsSource = null;
                dataList.Clear();

                if (CommonManager.Instance.IsConnected == false) return false;

                if (reloadData(dataList) == false) return false;

                if (IsSortViewOnReload == true)
                {
                    if (this.gvSorter != null)
                    {
                        this.gvSorter.SortByMultiHeader(dataList);
                    }
                    else
                    {
                        this.gvSorter = new GridViewSorter<T>();
                        this.gvSorter.SortByMultiHeaderWithKey(dataList, gridView.Columns,
                            initialSortKey, true, initialDirection);
                    }
                }
                else
                {
                    this.gvSorter.ResetSortParams();
                }

                listView.ItemsSource = dataList;

                //選択情報の復元
                oldItems.RestoreListViewSelected();
                return true;
            }
            catch (Exception ex)
            {
                Owner.Dispatcher.BeginInvoke(new Action(() =>
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }), null);
                return false;
            }
        }

        public bool GridViewHeaderClickSort(RoutedEventArgs e)
        {
            try
            {
                if (gvSorter == null) return false;

                GridViewColumnHeader headerClicked1 = e.OriginalSource as GridViewColumnHeader;
                if (headerClicked1 != null)
                {
                    if (headerClicked1.Role != GridViewColumnHeaderRole.Padding)
                    {
                        // ソートの実行。無効列の場合ソートはされないが、shiftクリックのマルチソート解除は実行する。
                        if (gvSorter.SortByMultiHeader(dataList, headerClicked1) == true)
                        {
                            listView.Items.Refresh();
                            return true;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return false;
        }

        public List<T> GetSelectedItemsList()
        {
            return listView.SelectedItems.OfType<T>().ToList();
        }
        public T SelectSingleItem(bool notSelectionChange = false)
        {
            if (listView == null) return null;
            if (listView.SelectedItems.Count <= 1) return listView.SelectedItem as T;//SingleMode用

            object item;
            if (ClickTarget == null)
            {
                //保存情報が無ければ、最後に選択したもの。
                item = listView.SelectedItems[listView.SelectedItems.Count - 1];
            }
            else
            {
                item = ClickTarget.Content;
            }
            if (notSelectionChange == false)
            {
                listView.UnselectAll();
                listView.SelectedItem = item;
            }
            return item as T;
        }

        public void SetCtxmTargetSave(ContextMenu ctxm)
        {
            //コンテキストメニューを開いたとき、アイテムがあればそれを保存する。無ければキャスト出来ずNULLになる。
            ctxm.Opened += (sender, e) => ClickTarget = (sender as ContextMenu).PlacementTarget as ListBoxItem;
            ctxm.Closed += (sender, e) => ClickTarget = null;
        }

        //リストのチェックボックスからの呼び出し
        public void ChgOnOffFromCheckbox(object hitItem, RoutedCommand cmd)
        {
            if (listView.SelectedItems.Contains(hitItem) == false)
            {
                listView.SelectedItem = hitItem;
            }
            cmd.Execute(listView, this.Owner);
        }
    }
}
