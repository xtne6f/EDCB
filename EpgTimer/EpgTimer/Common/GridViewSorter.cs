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
    public class GridViewSorter<T>
    {
        Dictionary<GridViewColumnHeader, ListSortDirection> _multiHeaderSortDict = new Dictionary<GridViewColumnHeader, ListSortDirection>();
        Brush defaultHeaderBorderBrush;
        List<string> exceptionHeaders = null;

        //リフレクション+毎回プロパティ読み出しがかなり重いので、キャッシュを使用する
        static gvCache gvCache1;

        /// <summary>
        /// 必要なら引数に無効扱いのキーを指定する。動かせない列などあれば。
        /// </summary>
        public GridViewSorter(List<string> exception = null)
        { exceptionHeaders = exception == null ? new List<string>() : exception.ToList(); }

        public void ResetSortParams() {
            if (this._multiHeaderSortDict.Count > 0) {
                foreach (GridViewColumnHeader header1 in this._multiHeaderSortDict.Keys) {
                    header1.FontWeight = FontWeights.Normal;
                    header1.BorderBrush = this.defaultHeaderBorderBrush;
                }
                this._multiHeaderSortDict = new Dictionary<GridViewColumnHeader, ListSortDirection>();
            }
        }

        /// <summary>
        /// 以前のソート状態で再ソートする。
        /// 実際にソートされるとTRUEが返る。
        /// </summary>
        public bool SortByMultiHeader(List<T> itemList0)
        {
            //キャッシュ初期化
            gvCache1 = new gvCache(itemList0);

            //前処理(キャッシュからDictionaryを排除するため)
            List<int> idxData = Enumerable.Range(0, itemList0.Count).ToList();

            string prevHeader1 = "";
            // Item1:first index, Item2: last index
            List<Tuple<int, int>> sortGroupList1 = null;
            for (int i1 = 0; i1 < this._multiHeaderSortDict.Count; i1++) {
                GridViewColumnHeader columnHeader1 = this._multiHeaderSortDict.ElementAt(i1).Key;
                ListSortDirection sortDirection1 = this._multiHeaderSortDict.ElementAt(i1).Value;
                string path = getPathString(columnHeader1);
                if (string.IsNullOrEmpty(path) == true) continue;

                sortGroupList1 = CreateSortedItemGroupList(prevHeader1, sortGroupList1);
                foreach (var kvp1 in sortGroupList1) {
                    idxData.Sort(
                        kvp1.Item1,
                        kvp1.Item2 - kvp1.Item1 + 1,
                        new ItemComparer(path, sortDirection1));
                }
                prevHeader1 = path;
            }

            //後処理
            List<T> srcList = itemList0.ToList();
            itemList0.Clear();
            itemList0.AddRange(idxData.Select(idx => srcList[idx]));

            //キャッシュクリア(staticなのでクリアしておく)
            gvCache1 = null;

            return this._multiHeaderSortDict.Count != 0;
        }

        private bool _sortByMultiHeader(List<T> itemList0, GridViewColumnHeader headerClicked0, bool directionSet = false, ListSortDirection direction = ListSortDirection.Ascending)
        {
            // 除外対象 空の場合、除外対象の場合
            if (headerClicked0 == null || string.IsNullOrEmpty(headerClicked0.Content.ToString())) { return false; }
            if (getHeaderString(headerClicked0) == "" || IsExceptionHeader(headerClicked0)) { return false; }

            //
            // ソート関連のパラメータをセット
            //
            if (this._multiHeaderSortDict.ContainsKey(headerClicked0)) {
                ListSortDirection Direction1 = this._multiHeaderSortDict[headerClicked0];
                Direction1 = Direction1 == ListSortDirection.Ascending ? 
                                        ListSortDirection.Descending : ListSortDirection.Ascending;
                Direction1 = directionSet == false ? Direction1 : direction;
                this._multiHeaderSortDict[headerClicked0] = Direction1;
            } else {
                if (!Keyboard.Modifiers.HasFlag(ModifierKeys.Control)) {
                    this.ResetSortParams();
                }
                this.defaultHeaderBorderBrush = headerClicked0.BorderBrush;
                headerClicked0.FontWeight = FontWeights.Bold;
                headerClicked0.BorderBrush = SystemColors.HighlightBrush;
                this._multiHeaderSortDict.Add(headerClicked0, directionSet == false ? ListSortDirection.Ascending : direction);
            }
            //
            // ソートの実行
            //
            return this.SortByMultiHeader(itemList0);
        }

        /// <summary>
        /// ヘッダーを指定してソートする。Ctrlクリックでソートヘッダを追加、Shiftクリックでヘッダ選択を解除できる。
        /// 実際にソートされるとTRUEが返る。
        /// </summary>
        public bool SortByMultiHeader(List<T> itemList0, GridViewColumnHeader headerClicked0)
        {
            if (Keyboard.Modifiers.HasFlag(ModifierKeys.Shift))
            {
                this.ResetSortParams();
                return false;
            }
            return _sortByMultiHeader(itemList0, headerClicked0);
        }

        /// <summary>
        /// キーを指定してソートする。主に初期化用。キーがColumsに存在していなければ何もしない。
        /// 実際にソートされるとTRUEが返る。
        /// </summary>
        public bool SortByMultiHeaderWithKey(List<T> itemList0, GridViewColumnCollection Columns, string Key, bool directionSet = false, ListSortDirection direction = ListSortDirection.Ascending)
        {
            List<GridViewColumnHeader> headers = Columns.Select(item => (GridViewColumnHeader)item.Header).ToList();
            return _sortByMultiHeader(itemList0, headers.Find(item => getHeaderString(item) == Key), directionSet, direction);
        }

        private string getHeaderString(GridViewColumnHeader columnHeader1)
        {
            if (columnHeader1 == null || columnHeader1.Tag is string == false) return "";
            //
            return columnHeader1.Tag as string;
        }

        private string getPathString(GridViewColumnHeader columnHeader1)
        {
            return CtrlCmdDefEx.GetValuePropertyName(typeof(T), getHeaderString(columnHeader1));
        }

        /// <summary>ヘッダが無効扱いのキーを持っていたらtrueを返す。</summary>
        private bool IsExceptionHeader(GridViewColumnHeader headerClicked0)
        {
            return (exceptionHeaders.FindIndex(str => str.CompareTo(getHeaderString(headerClicked0)) == 0) != -1);
        }

        /// <summary>
        /// 最後にソートしたヘッダを返す。マルチソートの場合、最後に追加されたヘッダを返す。
        /// </summary>
        public string LastHeader
        {
            get
            {
                if (IsExistSortParams == false) return "";
                return getHeaderString(this._multiHeaderSortDict.ElementAt(_multiHeaderSortDict.Count-1).Key);
            }
        }
        /// <summary>
        /// 最後にソートしたソート方向を返す。マルチソートの場合、最後に追加されたヘッダのソート方向を返す。
        /// </summary>
        public ListSortDirection LastDirection
        {
            get
            {
                if (IsExistSortParams == false) return ListSortDirection.Ascending;
                return this._multiHeaderSortDict.ElementAt(_multiHeaderSortDict.Count - 1).Value;
            }
        }

        private List<Tuple<int, int>> CreateSortedItemGroupList(string prevHeader0, List<Tuple<int, int>> prevSortGroupList0)
        {
            var sortGroupList1 = new List<Tuple<int, int>>();
            if (prevSortGroupList0 == null){    // 最初
                sortGroupList1.Add(new Tuple<int, int>(0, gvCache1.GetIDCache().Count - 1));
            } else {
                List<IComparable> values = gvCache1.GetDataCache(prevHeader0);
                foreach (var kvp1 in prevSortGroupList0) {
                    IComparable prevVal1 = null;
                    int startIndex1 = kvp1.Item1;
                    int i_First1 = kvp1.Item1;
                    int i_Last1 = kvp1.Item2;
                    for (int i1 = i_First1; i1 <= i_Last1; i1++) {
                        IComparable val1 = values[i1];
                        if (i1 != i_First1 && NullableEqualsTo(val1, prevVal1) == false)
                        {  // 値が変化
                            sortGroupList1.Add(new Tuple<int, int>(startIndex1, i1 - 1));
                            startIndex1 = i1;
                        }
                        prevVal1 = val1;
                        if (i1 == i_Last1) { // last
                            sortGroupList1.Add(new Tuple<int, int>(startIndex1, i1));
                        }
                    }
                }
            }
            return sortGroupList1;
        }

        private bool NullableEqualsTo(IComparable val1, IComparable val2)
        {
            return val1 == null ? (val2 == null ? true : false) : val1.CompareTo(val2) == 0;
        }

        public bool IsExistSortParams {
            get { return (this._multiHeaderSortDict.Count > 0); }
        }

        class ItemComparer : IComparer<int>
        {
            string sortBy;
            ListSortDirection direction;
            List<IComparable> values;
            List<ulong> keys;

            public ItemComparer(string sortBy0, ListSortDirection direction0) {
                this.sortBy = sortBy0;
                this.direction = direction0;
                this.values = gvCache1.GetDataCache(sortBy0);
                this.keys = gvCache1.GetIDCache();
            }

            public int Compare(int x1, int x2)
            {
                int cmprResult1 = 0;
                IComparable val1 = values[x1];
                IComparable val2 = values[x2];
                double d1, d2;
                if (val1 == null) {
                    cmprResult1 = (val2 == null ? 0 : -1);
                } else if (double.TryParse(val1 as string, out d1) && double.TryParse(val2 as string, out d2)) {   // 数値？
                    cmprResult1 = d1.CompareTo(d2);
                } else {
                    cmprResult1 = val1.CompareTo(val2);
                }
                // 比較結果が同じ // 再読み込みなどで並びが変わるのを防ぐ
                if (cmprResult1 == 0) {
                    cmprResult1 = keys[x1].CompareTo(keys[x2]);
                }
                // 降順
                if (this.direction == ListSortDirection.Descending) {
                    cmprResult1 *= -1;
                }
                return cmprResult1;
            }
        }

        //キャッシュ
        class gvCache
        {
            static Func<object, ulong> getKey = CtrlCmdDefEx.GetKeyFunc(typeof(T));

            Dictionary<string, List<IComparable>> dataCache = new Dictionary<string, List<IComparable>>();
            List<ulong> idCache = new List<ulong>();
            List<T> list;

            public gvCache(List<T> itemList)
            {
                list = itemList;
                idCache.AddRange(list.Select(item =>
                {
                    try { return getKey(item); }
                    catch { return ulong.MinValue; }
                }));
            }
            public List<ulong> GetIDCache()
            {
                return idCache;
            }
            public List<IComparable> GetDataCache(string strKey)
            {
                if (dataCache.ContainsKey(strKey) == false)
                {
                    PropertyInfo pi1 = typeof(T).GetProperty(strKey);

                    var dCache1 = new List<IComparable>();
                    if (pi1.PropertyType == typeof(List<string>))//ドロップリスト対応
                    {
                        dCache1.AddRange(list.Select(item =>
                        {
                            var data = pi1.GetValue(item, null) as List<string>;
                            return data == null || data.Count == 0 ? null : data[0];
                        }));
                    }
                    else if (pi1.PropertyType.GetInterface("IComparable") != null)
                    {
                        dCache1.AddRange(list.Select(item => pi1.GetValue(item, null) as IComparable));
                    }
                    else// その他イレギュラー等はとりあえずstringにする。
                    {
                        dCache1.AddRange(list.Select(item =>
                        {
                            object obj = pi1.GetValue(item, null);
                            return obj == null ? null : obj.ToString();
                        }));
                    }
                    dataCache.Add(strKey, dCache1);
                }
                return dataCache[strKey];
            }
        }
    }

}
