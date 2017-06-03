using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Controls;
using System.ComponentModel;
using System.Windows.Media;
using System.Reflection;
using System.Windows;
using System.Windows.Input;
using System.Windows.Data;

namespace EpgTimer {
    public class GridViewSorter<T> {

        Dictionary<GridViewColumnHeader, ListSortDirection> _multiHeaderSortDict = new Dictionary<GridViewColumnHeader, ListSortDirection>();
        Brush defaultHeaderBorderBrush;

        public void resetSortParams() {
            if (this._multiHeaderSortDict.Count > 0) {
                foreach (GridViewColumnHeader header1 in this._multiHeaderSortDict.Keys) {
                    header1.FontWeight = FontWeights.Normal;
                    header1.BorderBrush = this.defaultHeaderBorderBrush;
                }
                this._multiHeaderSortDict = new Dictionary<GridViewColumnHeader, ListSortDirection>();
            }
        }

        public void SortByMultiHeader(List<T> itemList0) {
            string prevHeader1 = "";
            // key:first index, value: last index
            Dictionary<int, int> sortGroupDict1 = new Dictionary<int, int>();
            for (int i1 = 0; i1 < this._multiHeaderSortDict.Count; i1++) {
                GridViewColumnHeader columnHeader1 = this._multiHeaderSortDict.ElementAt(i1).Key;
                ListSortDirection sortDirection1 = this._multiHeaderSortDict.ElementAt(i1).Value;
                string header = "";
                if (columnHeader1.Column.DisplayMemberBinding != null) {
                    header = ((Binding)columnHeader1.Column.DisplayMemberBinding).Path.Path;
                }
                sortGroupDict1 = this.createSortedItemGroupDict(itemList0, prevHeader1, sortGroupDict1);
                foreach (KeyValuePair<int, int> kvp1 in sortGroupDict1) {
                    itemList0.Sort(
                        kvp1.Key,
                        kvp1.Value - kvp1.Key + 1,
                        new ItemComparer(header, sortDirection1));
                }
                prevHeader1 = header;
            }
        }

        public void SortByMultiHeader(List<T> itemList0, GridViewColumnHeader headerClicked0) {
            if (string.IsNullOrEmpty(headerClicked0.Content.ToString())) { return; }
            //
            // ソート関連のパラメータをセット
            //
            if (this._multiHeaderSortDict.ContainsKey(headerClicked0)) {
                ListSortDirection lastListSortDirection1 = this._multiHeaderSortDict[headerClicked0];
                if (lastListSortDirection1 == ListSortDirection.Ascending) {
                    this._multiHeaderSortDict[headerClicked0] = ListSortDirection.Descending;
                } else {
                    this._multiHeaderSortDict[headerClicked0] = ListSortDirection.Ascending;
                }
            } else {
                if (!Keyboard.Modifiers.HasFlag(ModifierKeys.Control)) {
                    this.resetSortParams();
                }
                this.defaultHeaderBorderBrush = headerClicked0.BorderBrush;
                headerClicked0.FontWeight = FontWeights.Bold;
                headerClicked0.BorderBrush = SystemColors.HighlightBrush;
                this._multiHeaderSortDict.Add(headerClicked0, ListSortDirection.Ascending);
            }
            //
            // ソートの実行
            //
            this.SortByMultiHeader(itemList0);
        }

        Dictionary<int, int> createSortedItemGroupDict(List<T> itemList0, string prevHeader0, Dictionary<int, int> prevSortGroupDict0) {
            Dictionary<int, int> sortGroupDict1 = new Dictionary<int, int>();
            if (prevHeader0 == "") {    // 最初
                sortGroupDict1.Add(0, itemList0.Count - 1);
            } else {
                PropertyInfo pi1 = typeof(T).GetProperty(prevHeader0);
                foreach (KeyValuePair<int, int> kvp1 in prevSortGroupDict0) {
                    string prevVal1 = "";
                    int startIndex1 = kvp1.Key;
                    int i_First1 = kvp1.Key;
                    int i_Last1 = kvp1.Value;
                    for (int i1 = i_First1; i1 <= i_Last1; i1++) {
                        string val1 = pi1.GetValue(itemList0[i1], null).ToString();
                        if (i1 != i_First1 && val1 != prevVal1) {  // 値が変化
                            sortGroupDict1.Add(startIndex1, i1 - 1);
                            startIndex1 = i1;
                        }
                        prevVal1 = val1;
                        if (i1 == i_Last1) { // last
                            sortGroupDict1.Add(startIndex1, i1);
                        }
                    }
                }
            }
            return sortGroupDict1;
        }

        public bool isExistSortParams {
            get { return (this._multiHeaderSortDict.Count > 0); }
        }

        class ItemComparer : IComparer<T> {
            string sortBy;
            ListSortDirection direction;

            public ItemComparer(string sortBy0, ListSortDirection direction0) {
                this.sortBy = sortBy0;
                this.direction = direction0;
            }

            public int Compare(T x1, T x2) {
                int cmprResult1 = 0;

                PropertyInfo pi1 = typeof(T).GetProperty(this.sortBy);
                string val1 = pi1.GetValue(x1, null).ToString();
                string val2 = pi1.GetValue(x2, null).ToString();
                double d1, d2;
                if (double.TryParse(val1, out d1) && double.TryParse(val2, out d2)) {   // 数値？
                    cmprResult1 = d1.CompareTo(d2);
                } else {
                    cmprResult1 = val1.CompareTo(val2);
                }
                // 降順
                if (this.direction == ListSortDirection.Descending) {
                    cmprResult1 *= -1;
                }
                // 比較結果が同じ
                if (cmprResult1 == 0) {
                    // 再読み込みなどで並びが変わるのを防ぐ
                    Type type1 = x1.GetType();
                    if (type1 == typeof(SearchItem)) {
                        SearchItem x1_1 = x1 as SearchItem;
                        SearchItem x2_1 = x2 as SearchItem;
                        cmprResult1 = x1_1.EventInfo.event_id.CompareTo(x2_1.EventInfo.event_id);
                    }
                }

                return cmprResult1;
            }

        }

    }
}
