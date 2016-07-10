using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Controls;
using System.ComponentModel;
using System.Windows.Media;
using System.Reflection;
using System.Windows;
using System.Windows.Input;

namespace EpgTimer
{
    public interface IGridViewSorterItem
    {
        ulong KeyID { get; }
        string GetValuePropertyName(string key);
    }

    public class GridViewSorter
    {
        Dictionary<GridViewColumnHeader, ListSortDirection> _multiHeaderSortDict = new Dictionary<GridViewColumnHeader, ListSortDirection>();
        Brush defaultHeaderBorderBrush;
        List<string> exceptionHeaders = null;

        /// <summary>必要なら引数に無効扱いのキーを指定する。動かせない列などあれば。</summary>
        public GridViewSorter(List<string> exception = null)
        {
            exceptionHeaders = exception == null ? new List<string>() : exception.ToList();
        }

        public void ResetSortParams()
        {
            if (IsExistSortParams == false) return;

            foreach (GridViewColumnHeader header1 in this._multiHeaderSortDict.Keys)
            {
                header1.FontWeight = FontWeights.Normal;
                header1.BorderBrush = this.defaultHeaderBorderBrush;
            }
            this._multiHeaderSortDict = new Dictionary<GridViewColumnHeader, ListSortDirection>();
        }

        /// <summary>
        /// 以前のソート状態で再ソートする。
        /// 実際にソートされるとTRUEが返る。
        /// </summary>
        public bool SortByMultiHeader(IList itemList0)
        {
            if (IsExistSortParams == false) return false;

            List<object> srcList = itemList0.OfType<object>().ToList();
            if (srcList.Count <= 1) return false;

            //リフレクション+毎回プロパティ読み出しがかなり重いので、キャッシュを使用する
            var gvCache1 = new gvCache(srcList);

            //前処理(キャッシュからDictionaryを排除するため)
            List<int> idxData = Enumerable.Range(0, srcList.Count).ToList();

            string prevPath = "";
            // Item1:first index, Item2: last index
            List<Tuple<int, int>> sortGroupList1 = null;
            for (int i1 = 0; i1 < this._multiHeaderSortDict.Count; i1++)
            {
                GridViewColumnHeader columnHeader1 = this._multiHeaderSortDict.ElementAt(i1).Key;
                ListSortDirection sortDirection1 = this._multiHeaderSortDict.ElementAt(i1).Value;
                string path = getHeaderString(columnHeader1);
                if (string.IsNullOrEmpty(path) == true) continue;

                sortGroupList1 = CreateSortedItemGroupList(prevPath, sortGroupList1, idxData, gvCache1);
                foreach (var kvp1 in sortGroupList1)
                {
                    idxData.Sort(
                        kvp1.Item1,
                        kvp1.Item2 - kvp1.Item1 + 1,
                        new ItemComparer(path, sortDirection1, gvCache1));
                }
                prevPath = path;
            }

            //後処理
            itemList0.Clear();
            foreach (var idx in idxData) itemList0.Add(srcList[idx]);

            return true;
        }

        private bool _sortByMultiHeader(IList itemList0, GridViewColumnHeader headerClicked0, bool directionSet = false, ListSortDirection direction = ListSortDirection.Ascending)
        {
            // 除外対象 空の場合、除外対象の場合
            if (headerClicked0 == null || string.IsNullOrEmpty(getHeaderString(headerClicked0))
                                                                || IsExceptionHeader(headerClicked0)) return false;

            // ソート関連のパラメータをセット
            if (this._multiHeaderSortDict.ContainsKey(headerClicked0))
            {
                ListSortDirection Direction1 = this._multiHeaderSortDict[headerClicked0];
                Direction1 = Direction1 == ListSortDirection.Ascending ?
                                        ListSortDirection.Descending : ListSortDirection.Ascending;
                Direction1 = directionSet == false ? Direction1 : direction;
                this._multiHeaderSortDict[headerClicked0] = Direction1;
            }
            else
            {
                if (!Keyboard.Modifiers.HasFlag(ModifierKeys.Control))
                {
                    this.ResetSortParams();
                }
                this.defaultHeaderBorderBrush = headerClicked0.BorderBrush;
                headerClicked0.FontWeight = FontWeights.Bold;
                headerClicked0.BorderBrush = SystemColors.HighlightBrush;
                this._multiHeaderSortDict.Add(headerClicked0, directionSet == false ? ListSortDirection.Ascending : direction);
            }

            // ソートの実行
            return this.SortByMultiHeader(itemList0);
        }

        /// <summary>
        /// ヘッダーを指定してソートする。Ctrlクリックでソートヘッダを追加、Shiftクリックでヘッダ選択を解除できる。
        /// 実際にソートされるとTRUEが返る。
        /// </summary>
        public bool SortByMultiHeader(IList itemList0, GridViewColumnHeader headerClicked0)
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
        public bool SortByMultiHeaderWithKey(IList itemList0, GridViewColumnCollection Columns, string Key, bool directionSet = false, ListSortDirection direction = ListSortDirection.Ascending)
        {
            return _sortByMultiHeader(itemList0, Columns.Select(item => (GridViewColumnHeader)item.Header)
                        .FirstOrDefault(header => getHeaderString(header) == Key), directionSet, direction);
        }

        private string getHeaderString(GridViewColumnHeader columnHeader1)
        {
            return columnHeader1.Uid;
        }

        /// <summary>ヘッダが無効扱いのキーを持っていたらtrueを返す。</summary>
        private bool IsExceptionHeader(GridViewColumnHeader headerClicked0)
        {
            return exceptionHeaders.Exists(str => str.CompareTo(getHeaderString(headerClicked0)) == 0);
        }

        /// <summary>最後にソートしたヘッダを返す。マルチソートの場合、最後に追加されたヘッダを返す。</summary>
        public string LastHeader
        {
            get
            {
                if (IsExistSortParams == false) return "";
                return getHeaderString(this._multiHeaderSortDict.ElementAt(_multiHeaderSortDict.Count - 1).Key);
            }
        }
        /// <summary>最後にソートしたソート方向を返す。マルチソートの場合、最後に追加されたヘッダのソート方向を返す。</summary>
        public ListSortDirection LastDirection
        {
            get
            {
                if (IsExistSortParams == false) return ListSortDirection.Ascending;
                return this._multiHeaderSortDict.ElementAt(_multiHeaderSortDict.Count - 1).Value;
            }
        }

        private List<Tuple<int, int>> CreateSortedItemGroupList(string prevPath
            , List<Tuple<int, int>> prevSortGroupList0, List<int> orderdIdxData, gvCache cache)
        {
            var sortGroupList1 = new List<Tuple<int, int>>();
            if (prevSortGroupList0 == null)
            { // 最初
                sortGroupList1.Add(new Tuple<int, int>(0, cache.ItemsCount - 1));
            }
            else
            {
                IComparable[] values = cache.GetCache(prevPath);
                foreach (var kvp1 in prevSortGroupList0)
                {
                    IComparable prevVal1 = null;
                    int startIndex1 = kvp1.Item1;
                    int i_First1 = kvp1.Item1;
                    int i_Last1 = kvp1.Item2;
                    for (int i1 = i_First1; i1 <= i_Last1; i1++)
                    {
                        IComparable val1 = values[orderdIdxData[i1]];
                        if (i1 != i_First1 && NullableEqualsTo(val1, prevVal1) == false)
                        {// 値が変化
                            sortGroupList1.Add(new Tuple<int, int>(startIndex1, i1 - 1));
                            startIndex1 = i1;
                        }
                        prevVal1 = val1;
                        if (i1 == i_Last1)
                        {// last
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

        public bool IsExistSortParams
        {
            get { return this._multiHeaderSortDict.Count > 0; }
        }

        class ItemComparer : IComparer<int>
        {
            int direction;
            IComparable[] values;
            double[] dvalues;
            bool[] isnumeric;
            ulong[] keys;

            public ItemComparer(string sortBy0, ListSortDirection direction0, gvCache cache)
            {
                direction = direction0 == ListSortDirection.Descending ? -1 : 1;
                cache.GetCache(sortBy0, out this.values, out this.dvalues, out this.isnumeric, out this.keys);
            }

            public int Compare(int x1, int x2)
            {
                int cmprResult1 = 0;
                if (isnumeric[x1] && isnumeric[x2])// 数値？
                {
                    cmprResult1 = dvalues[x1].CompareTo(dvalues[x2]);
                }
                else if (values[x1] == null)
                {
                    cmprResult1 = (values[x2] == null ? 0 : -1);
                }
                else
                {
                    cmprResult1 = values[x1].CompareTo(values[x2]);
                }
                // 比較結果が同じ // 再読み込みなどで並びが変わるのを防ぐ
                if (cmprResult1 == 0)
                {
                    cmprResult1 = keys[x1].CompareTo(keys[x2]);
                }
                // 降順
                cmprResult1 *= direction;

                return cmprResult1;
            }
        }

        public static Func<object, ulong> GetKeyFunc(Type t)
        {
            if (t.GetInterface(typeof(IGridViewSorterItem).Name) != null)
            {
                return info => (info as IGridViewSorterItem).KeyID;
            }
            else
            {
                return info => (ulong)info.GetHashCode();
            }
        }
        public static Func<string, string> GetValuePropertyFunc(object item)
        {
            if (item is IGridViewSorterItem)
            {
                return str => (item as IGridViewSorterItem).GetValuePropertyName(str);
            }
            else
            {
                return str => str;
            }
        }

        //キャッシュ
        class gvCache
        {
            Dictionary<string, IComparable[]> dataCache = new Dictionary<string, IComparable[]>();
            Dictionary<string, double[]> dataCacheDoubles = new Dictionary<string, double[]>();
            Dictionary<string, bool[]> dataCacheBools = new Dictionary<string, bool[]>();
            ulong[] idCache;
            List<object> list;
            private Type typeKey;
            private Func<string, string> getValueKey;

            public int ItemsCount { get { return list.Count; } }

            public gvCache(List<object> itemList)
            {
                list = itemList;
                typeKey = list[0].GetType();
                Func<object, ulong> getKey = GridViewSorter.GetKeyFunc(typeKey);
                getValueKey = GridViewSorter.GetValuePropertyFunc(list[0]);

                idCache = list.Select(item =>
                {
                    try { return getKey(item); }
                    catch { return ulong.MinValue; }
                }).ToArray();
            }
            public void GetCache(string strKey,
                out IComparable[] values, out double[] dvalues, out bool[] isnumeric, out ulong[] keys)
            {
                values = GetCache(strKey);
                dvalues = dataCacheDoubles[strKey];
                isnumeric = dataCacheBools[strKey];
                keys = idCache;
            }
            public IComparable[] GetCache(string strKey)
            {
                if (dataCache.ContainsKey(strKey) == false)
                {
                    PropertyInfo pi1 = typeKey.GetProperty(getValueKey(strKey));
                    bool isString = true;

                    IComparable[] dCache1;
                    if (pi1.PropertyType == typeof(List<string>))//ドロップリスト対応
                    {
                        dCache1 = list.Select(item =>
                        {
                            var data = pi1.GetValue(item, null) as List<string>;
                            return data == null || data.Count == 0 ? null : data[0];
                        }).ToArray();
                    }
                    else if (pi1.PropertyType.GetInterface(typeof(IComparable).Name) != null)
                    {
                        isString = pi1.PropertyType == typeof(string);
                        dCache1 = list.Select(item => pi1.GetValue(item, null) as IComparable).ToArray();
                    }
                    else// その他イレギュラー等はとりあえずstringにする。
                    {
                        dCache1 = list.Select(item =>
                        {
                            object obj = pi1.GetValue(item, null);
                            return obj == null ? null : obj.ToString();
                        }).ToArray();
                    }
                    dataCache.Add(strKey, dCache1);

                    double[] dCacheDouble1 = null;
                    bool[] dCacheFlg1 = new bool[this.ItemsCount];
                    if (isString == true)
                    {
                        dCacheDouble1 = new double[this.ItemsCount];
                        for (int i = 0; i < this.ItemsCount; i++)
                        {
                            dCacheFlg1[i] = double.TryParse(dCache1[i] as string, out dCacheDouble1[i]);
                        }
                    }
                    dataCacheDoubles.Add(strKey, dCacheDouble1);
                    dataCacheBools.Add(strKey, dCacheFlg1);
                }
                return dataCache[strKey];
            }
        }
    }
}
