using System;
using System.Collections.Generic;
using System.Linq;

namespace EpgTimer
{
    public static class CopyObj
    {
        //static CopyData(src,dest)を用意して、拡張メソッドを追加するため用。
        //ジェネリックのまま拡張メソッドをpublicにするわけにはいかないけど、1行で書けたらいいのに‥。

        //public static List<クラス名> Clone(this List<クラス名> src) { return CopyObj.Clone(src, CopyData); }
        //public static クラス名 Clone(this クラス名 src) { return CopyObj.Clone(src, CopyData); }
        //public static void CopyTo(this クラス名 src, クラス名 dest) { CopyObj.CopyTo(src, dest, CopyData); }

        public static List<T> Clone<T>(List<T> src, Action<T, T> CopyData) where T : class, new()
        {
            if (src == null) return null;
            return src.Select(item => Clone(item, CopyData)).ToList();
        }
        public static T Clone<T>(T src, Action<T, T> CopyData) where T : class, new()
        {
            if (src == null) return null;
            T copyobj = new T();
            CopyTo(src, copyobj, CopyData);
            return copyobj;
        }
        public static void CopyTo<T>(T src, T dest, Action<T, T> CopyData)
        {
            if (src == null || dest == null) return;
            CopyData(src, dest);
        }

        //static EqualsValue(src,dest)を用意して、拡張メソッドを追加するため用。
        //public static bool EqualsTo(this IList<クラス名> src,  IList<RecSettingData> dest) { return CopyObj.EqualsTo(src, dest, EqualsValue); }
        //public static bool EqualsTo(this クラス名 src, クラス名 dest) { return CopyObj.EqualsTo(src, dest, EqualsValue); }

        public static bool EqualsTo<T>(IList<T> src, IList<T> dest, Func<T, T, bool> EqualsValue) where T : class, new()
        {
            if (src == null || dest == null || src.Count != dest.Count) return false;
            return src.Zip(dest, (s, d) => EqualsValue(s, d)).All(r => r == true);
        }
        public static bool EqualsTo<T>(T src, T dest, Func<T, T, bool> EqualsValue)
        {
            if (src == null || dest == null) return false;
            return EqualsValue(src, dest);
        }
    }
}
