using System;
using System.Reflection;

namespace EpgTimer
{
    public class GridViewSorterItem : IGridViewSorterItem
    {
        public virtual ulong KeyID { get { return (ulong)(this.GetHashCode()); } }
        public string GetValuePropertyName(string key)
        {
            //ソート用の代替プロパティには"Value"を後ろに付けることにする。
            //呼び出し回数多くないのでとりあえずこれで。
            PropertyInfo pInfo = this.GetType().GetProperty(key + "Value");
            return pInfo == null ? key : pInfo.Name;
        }
    }
}
