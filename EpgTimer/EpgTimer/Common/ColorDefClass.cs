using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Media;
using System.Reflection;
using System.Windows;

namespace EpgTimer
{
    public static class ColorDef
    {
        public static SortedList<string, SolidColorBrush> BrushNames { get; private set; }

        static ColorDef()
        {
            BrushNames = new SortedList<string, SolidColorBrush>();
            foreach (PropertyInfo prop in typeof(Brushes).GetProperties())
            {
                BrushNames[prop.Name] = (SolidColorBrush)prop.GetValue(null, null);
            }
            BrushNames["カスタム"] = Brushes.Transparent;
        }

        public static SolidColorBrush BrushFromName(string name)
        {
            return BrushNames.ContainsKey(name) ? BrushNames[name] : Brushes.White;
        }
        public static Color FromUInt(uint value)
        {
            return Color.FromArgb((byte)(value >> 24), (byte)(value >> 16), (byte)(value >> 8), (byte)value);
        }
        public static uint ToUInt(Color c)
        {
            return ((uint)c.A) << 24 | ((uint)c.R) << 16 | ((uint)c.G) << 8 | (uint)c.B;
        }
        public static double GetLuminance(Color c)
        {
            return (0.298912 * c.R + 0.586611 * c.G + 0.114478 * c.B) / 255;
        }

        public static LinearGradientBrush GradientBrush(Color color, double luminance = 0.94, double saturation = 1.2)
        {
            // 彩度を上げる
            int[] numbers = {color.R, color.G, color.B};
            double n1 = numbers.Max();
            double n2 = numbers.Min();
            double n3 = n1 / (n1 - n2);
            double r = (color.R - n1) * saturation + n1;
            double g = (color.G - n1) * saturation + n1;
            double b = (color.B - n1) * saturation + n1;
            r = Math.Max(r, 0);
            g = Math.Max(g, 0);
            b = Math.Max(b, 0);

            // 明るさを下げる
            double l1 = 0.298912 * color.R + 0.586611 * color.G + 0.114478 * color.B;
            double l2 = 0.298912 * r + 0.586611 * g + 0.114478 * b;
            double f = (l2 / l1) * luminance;
            r *= f;
            g *= f;
            b *= f;
            r = Math.Min(r, 255);
            g = Math.Min(g, 255);
            b = Math.Min(b, 255);

            Color color2 = Color.FromArgb(color.A, (byte)r, (byte)g, (byte)b);
            
            LinearGradientBrush brush = new LinearGradientBrush();
            brush.StartPoint = new Point(0, 0.5);
            brush.EndPoint = new Point(0, 1);
            brush.GradientStops.Add(new GradientStop(color, 0.0));
            brush.GradientStops.Add(new GradientStop(color2, 1.0));
            brush.Freeze();

            return brush;
        }

        public static SolidColorBrush CustColorBrush(string name, uint cust, byte a = 0xFF, int opacity = 100)
        {
            SolidColorBrush brush;
            if (name == "カスタム")
            {
                Color c = FromUInt(cust);
                brush = new SolidColorBrush(Color.FromArgb((byte)(c.A * opacity / 100), c.R, c.G, c.B));
                brush.Freeze();
            }
            else
            {
                brush = BrushFromName(name);
                if (brush.Color.A != 0 && (a != 0xFF || opacity != 100))
                {
                    brush = new SolidColorBrush(Color.FromArgb((byte)(a * opacity / 100), brush.Color.R, brush.Color.G, brush.Color.B));
                    brush.Freeze();
                }
            }
            return brush;
        }
    }
}
