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
        protected ListView listView = null;
        public T oldItem = null;
        public List<T> oldItems = null;

        public ListViewSelectedKeeper(ListView list, bool DoStoringNow = false)
        {
            listView = list;
            if (DoStoringNow) StoreListViewSelected();
        }

        private Func<T, T, bool> SetFunc()
        {
            //コンストラクタでやった方がいいのかもだけど、ReserveItemがwhere:newの条件外なので少し面倒
            switch (oldItem.GetType().Name)
            {
                case "ReserveItem":
                    return (i1, i2) =>
                        (i1 as ReserveItem).ReserveInfo.ReserveID == (i2 as ReserveItem).ReserveInfo.ReserveID;
                case "RecInfoItem":
                    return (i1, i2) =>
                        (i1 as RecInfoItem).RecInfo.ID == (i2 as RecInfoItem).RecInfo.ID;
                case "EpgAutoDataItem":
                    return (i1, i2) =>
                        (i1 as EpgAutoDataItem).EpgAutoAddInfo.dataID == (i2 as EpgAutoDataItem).EpgAutoAddInfo.dataID;
                case "SearchItem":
                    return (i1, i2) =>
                        CommonManager.EqualsPg((i1 as SearchItem).EventInfo, (i2 as SearchItem).EventInfo);
                default:
                    return (i1, i2) => i1 == i2;
            }
        }

        public void StoreListViewSelected()
        {
            if (listView != null && listView.SelectedItem != null)
            {
                oldItem = (T)listView.SelectedItem;
                oldItems = listView.SelectedItems.Cast<T>().ToList();
            }
        }

        public void RestoreListViewSelected()
        {
            try
            {
                if (listView != null && oldItem != null && oldItems != null)
                {
                    Func<T, T, bool> compareFunc = SetFunc();

                    //このUnselectAll()は無いと正しく復元出来ない状況があり得る
                    listView.UnselectAll();

                    foreach (T item in listView.Items)
                    {
                        if (compareFunc(item, oldItem) == true)
                        {
                            listView.SelectedItem = item;
                            listView.ScrollIntoView(item);
                        }
                    }

                    foreach (T oldItem1 in oldItems)
                    {
                        foreach (T item in listView.Items)
                        {
                            if (compareFunc(item, oldItem1) == true)
                            {
                                listView.SelectedItems.Add(item);
                                break;
                            }
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
