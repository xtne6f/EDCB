using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.ComponentModel;

using System.Reflection;

namespace EpgTimer
{
    public class ListViewController<T> where T : class
    {
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private ViewUtil vutil = CommonManager.Instance.VUtil;

        public GridViewSelector gvSelector { get; set; }
        public GridViewSorter<T> gvSorter { get; set; }
        public List<T> dataList { get; set; }

        private string column_SavePath = null;
        private string sort_HeaderSavePath = null;
        private string sort_DirectionSavePath = null;
        private List<string> gridHeaderExceptionList = new List<string>();
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
            , string[] gvHeaderException = null, RoutedEventHandler headerclick = null)
        {
            listView = lv;
            gridView = gv;
            IsSortViewOnReload = isSortOnReload;
            if (gvHeaderException != null)
            {
                gridHeaderExceptionList = new List<string>(gvHeaderException);
            }
            if (IsSortViewOnReload == false)
            {
                this.gvSorter = new GridViewSorter<T>(gridHeaderExceptionList);
            }

            var hclick = headerclick != null ? headerclick : (sender, e) => this.GridViewHeaderClickSort(e);
            foreach (GridViewColumn info in gridView.Columns)
            {
                GridViewColumnHeader header = info.Header as GridViewColumnHeader;
                header.Click += new RoutedEventHandler(hclick);
                if (header.ToolTip == null && gridHeaderExceptionList.All(str => str != header.Tag as string) == true)
                {
                    header.ToolTip = "Ctrl+Click(マルチ・ソート)、Shift+Click(解除)";
                }
            }

            gvSelector = new GridViewSelector(gv, this.columnSaveList);
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

                if (vutil.EpgTimerNWNotConnect() == true) return false;

                if (reloadData(dataList) == false) return false;

                if (IsSortViewOnReload == true)
                {
                    if (this.gvSorter != null)
                    {
                        this.gvSorter.SortByMultiHeader(dataList);
                    }
                    else
                    {
                        this.gvSorter = new GridViewSorter<T>(gridHeaderExceptionList);
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
            return mutil.SelectSingleItem(listView, notSelectionChange) as T;
        }
    }
}
