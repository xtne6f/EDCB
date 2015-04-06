using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public class ListViewSelectedKeeper<T> where T : class
    {
        //リスト番組表で全選択状態でチャンネル選択更新してしまったりしたときなどでも大丈夫なように、
        //一応選択数の上限を設定しておく。
        public uint MaxRestoreNum = 100;
        
        protected ListView listView = null;
        public T oldItem = null;
        public List<T> oldItems = null;
        public bool allSelected = false;

        public ListViewSelectedKeeper(ListView list, bool DoStoringNow = false)
        {
            listView = list;
            if (DoStoringNow) StoreListViewSelected();
        }

        private Func<T, ulong> SetFunc()
        {
            //コンストラクタでやった方がいいのかもだけど、ReserveItemがwhere:newの条件外なので少し面倒
            switch (oldItem.GetType().Name)
            {
                case "ReserveItem":
                    return info => (info as ReserveItem).ReserveInfo.ReserveID;
                case "RecInfoItem":
                    return info => (info as RecInfoItem).RecInfo.ID;
                case "EpgAutoDataItem":
                    return info => (info as EpgAutoDataItem).EpgAutoAddInfo.dataID;
                case "ManualAutoAddDataItem":
                    return info => (info as ManualAutoAddDataItem).ManualAutoAddInfo.dataID;
                case "SearchItem":
                    return info => (info as SearchItem).EventInfo.Create64PgKey();
                default:
                    return info => (ulong)info.GetHashCode();
            }
        }

        public void StoreListViewSelected()
        {
            if (listView != null && listView.SelectedItem != null)
            {
                oldItem = (T)listView.SelectedItem;
                oldItems = listView.SelectedItems.Cast<T>().ToList();
                allSelected = (oldItems.Count == listView.Items.Count);
            }
        }

        public void RestoreListViewSelected()
        {
            try
            {
                if (listView != null && oldItem != null && oldItems != null)
                {
                    if (this.allSelected == true)
                    {
                        listView.SelectAll();
                        return;
                    }

                    //このUnselectAll()は無いと正しく復元出来ない状況があり得る
                    listView.UnselectAll();

                    //上限越えの場合は、選択を解除して終了。
                    if (oldItems.Count >= this.MaxRestoreNum) return;

                    //選択数が少ないときは逆に遅くなる気もするが、Dictionaryにしておく
                    var listKeys = new Dictionary<ulong, T>();
                    Func<T, ulong> getKey = SetFunc();

                    foreach (T listItem1 in listView.Items)
                    {
                        //重複するキーは基本的に無いという前提
                        try
                        {
                            listKeys.Add(getKey(listItem1), listItem1);
                        }
                        catch { }
                    }

                    T setItem;
                    if (listKeys.TryGetValue(getKey(oldItem), out setItem))
                    {
                        listView.SelectedItem = setItem;
                        listView.ScrollIntoView(setItem);
                    }

                    foreach (T oldItem1 in oldItems)
                    {
                        if (listKeys.TryGetValue(getKey(oldItem1), out setItem))
                        {
                            //数が多いとき、このAddが致命的に遅い
                            listView.SelectedItems.Add(setItem);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

    }

}
