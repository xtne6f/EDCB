using System;
using System.Windows.Data;

namespace EpgTimer
{
    public class BoolConverter : IValueConverter
    {
        public object Convert(object v, Type t, object p, System.Globalization.CultureInfo c)
        {
            // bool or number => bool
            bool not = (p != null && ((string)p).IndexOf("not", StringComparison.Ordinal) >= 0);
            return (v is bool ? (bool)v : (v ?? 0).ToString() != "0") != not;
        }
        public object ConvertBack(object v, Type t, object p, System.Globalization.CultureInfo c)
        {
            // bool => bool or number
            bool not = (p != null && ((string)p).IndexOf("not", StringComparison.Ordinal) >= 0);
            return (v is bool && (bool)v) != not ? (t == typeof(bool) ? true : (object)(byte)1) : (t == typeof(bool) ? false : (object)(byte)0);
        }
    }
}
