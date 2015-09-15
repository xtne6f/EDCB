using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Controls;
using System.ComponentModel;
using System.Windows.Media;
using System.Reflection;
using EpgTimer;
using System.Windows;
using System.Windows.Input;
using System.Windows.Data;

namespace EpgTimer {
    public class GridViewSorter<T> {

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
            // key:first index, value: last index
            Dictionary<int, int> sortGroupDict1 = new Dictionary<int, int>();
            for (int i1 = 0; i1 < this._multiHeaderSortDict.Count; i1++) {
                GridViewColumnHeader columnHeader1 = this._multiHeaderSortDict.ElementAt(i1).Key;
                ListSortDirection sortDirection1 = this._multiHeaderSortDict.ElementAt(i1).Value;
                string header = getHeaderString(columnHeader1);
                if (header == "") continue;

                sortGroupDict1 = this.createSortedItemGroupDict(itemList0, prevHeader1, sortGroupDict1);
                foreach (KeyValuePair<int, int> kvp1 in sortGroupDict1) {
                    idxData.Sort(
                        kvp1.Key,
                        kvp1.Value - kvp1.Key + 1,
                        new ItemComparer(header, sortDirection1));
                }
                prevHeader1 = header;
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
            string header = "";
            if (columnHeader1 != null)
            {
                if (columnHeader1.Tag as string != null)
                {
                    header = columnHeader1.Tag as string;
                }
                else if (columnHeader1.Column != null && columnHeader1.Column.DisplayMemberBinding != null)
                {
                    header = ((Binding)columnHeader1.Column.DisplayMemberBinding).Path.Path;
                }
            }
            return header != null ? header : "";
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

        public bool IsExistSortParams {
            get { return (this._multiHeaderSortDict.Count > 0); }
        }

        class ItemComparer : IComparer<int>
        {
            string sortBy;
            ListSortDirection direction;
            List<string> values;
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
                string val1 = values[x1];
                string val2 = values[x2];
                double d1, d2;
                if (double.TryParse(val1, out d1) && double.TryParse(val2, out d2)) {   // 数値？
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
            static Func<object, ulong> getKey = null;

            //arrayにするともう少し早くなるかも？
            Dictionary<string, List<string>> dataCache = new Dictionary<string, List<string>>();
            List<ulong> idCache = new List<ulong>();
            List<T> list;

            public gvCache(List<T> itemList)
            {
                list = itemList;

                if (getKey == null)
                {
                    if (list.Count != 0)
                    {
                        getKey = CtrlCmdDefEx.GetKeyFunc(list[0]);
                    }
                }

                if (getKey != null)
                {
                    idCache.AddRange(list.Select(item => getKey(item)));
                }
                else
                {
                    idCache.AddRange(Enumerable.Repeat<ulong>(0, list.Count));//一応ここに来ることは無いはず
                }
            }
            public List<ulong> GetIDCache()
            {
                return idCache;
            }
            public List<string> GetDataCache(string strKey)
            {
                if (dataCache.ContainsKey(strKey) == false)
                {
                    PropertyInfo pi1 = typeof(T).GetProperty(strKey);

                    var dCache1 = new List<string>();
                    if (pi1.PropertyType == typeof(List<string>))//ドロップリスト対応
                    {
                        dCache1.AddRange(list.Select(item =>
                        {
                            var data = pi1.GetValue(item, null) as List<string>;
                            return data == null || data.Count == 0 || data[0] == null ? "" : data[0];
                        }));
                    }
                    else// (t == typeof(string))//string,boolなど基本型、またはイレギュラー
                    {
                        dCache1.AddRange(list.Select(item =>
                        {
                            object obj = pi1.GetValue(item, null);
                            string data = obj == null ? "" : obj.ToString();
                            return data == null ? "" : data;
                        }));
                    }
                    dataCache.Add(strKey, dCache1);
                }
                return dataCache[strKey];
            }
        }
    }

}
