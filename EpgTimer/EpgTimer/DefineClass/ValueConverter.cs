using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;

namespace EpgTimer
{
    public class ToStringValueIsEqualConverter : IValueConverter
    {
        public static bool ProcessConvert(object v, object p)
        {
            // bool => 0,1
            string s = v == null ? null : !(v is bool) ? v.ToString() : (bool)v ? "1" : "0";
            string q = p == null ? null : !(p is bool) ? p.ToString() : (bool)v ? "1" : "0";
            return s == q;
        }
        public object Convert(object v, Type t, object p, CultureInfo c)
        {
            return ProcessConvert(v, p);
        }
        public object ConvertBack(object v, Type t, object p, CultureInfo c)
        {
            return DependencyProperty.UnsetValue;
        }
    }

    public class ToStringValueIsNotEqualConverter : IValueConverter
    {
        public object Convert(object v, Type t, object p, CultureInfo c)
        {
            return !ToStringValueIsEqualConverter.ProcessConvert(v, p);
        }
        public object ConvertBack(object v, Type t, object p, CultureInfo c)
        {
            return DependencyProperty.UnsetValue;
        }
    }

    public class ToStringValueIsZeroOrOneConverter : IValueConverter
    {
        public object Convert(object v, Type t, object p, CultureInfo c)
        {
            return ToStringValueIsEqualConverter.ProcessConvert(v, "0");
        }
        public object ConvertBack(object v, Type t, object p, CultureInfo c)
        {
            t = Nullable.GetUnderlyingType(t) ?? t;
            return !(v is bool) ? DependencyProperty.UnsetValue : t == typeof(bool) ? !(bool)v : System.Convert.ChangeType((bool)v ? 0 : 1, t);
        }
    }

    public class ToStringValueIsOneOrZeroConverter : IValueConverter
    {
        public object Convert(object v, Type t, object p, CultureInfo c)
        {
            return !ToStringValueIsEqualConverter.ProcessConvert(v, "0");
        }
        public object ConvertBack(object v, Type t, object p, CultureInfo c)
        {
            t = Nullable.GetUnderlyingType(t) ?? t;
            return !(v is bool) ? DependencyProperty.UnsetValue : t == typeof(bool) ? (bool)v : System.Convert.ChangeType((bool)v ? 1 : 0, t);
        }
    }
}
