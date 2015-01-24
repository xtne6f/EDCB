using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Media;
using System.Reflection;
using System.Windows;

namespace EpgTimer
{
    public class ColorDef
    {
        private Dictionary<string, SolidColorBrush> colorTable;
        private static ColorDef _instance;
        public static ColorDef Instance
        {
            get
            {
                if (_instance == null)
                    _instance = new ColorDef();
                return _instance;
            }
            set { _instance = value; }
        }

        public static string[] ColorNames
        {
            get
            {
                PropertyInfo[] props = typeof(Colors).GetProperties();
                List<string> colorName = props.Select(s => s.Name).ToList<string>();
                colorName.Add("カスタム");
                return colorName.ToArray();
            }
        }

        public Dictionary<string, SolidColorBrush> ColorTable
        {
            get
            {
                if (colorTable == null)
                {
                    colorTable = new Dictionary<string, SolidColorBrush>();

                    var brushtype = typeof(Brushes);
                    foreach (PropertyInfo prop in brushtype.GetProperties())
                    {
                        var p = brushtype.GetProperty(prop.Name);
                        colorTable.Add(prop.Name, (SolidColorBrush)p.GetValue(brushtype, null));
                    }
                    colorTable.Add("カスタム", Brushes.White);
                }
                return colorTable;
            }
        }

        public static Color ColorFromName(string name)
        {
            var colortype = typeof(Colors);
            return (Color)colortype.GetProperty(name).GetValue(colortype, null);
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

            Color color2 = Color.FromRgb((byte)r, (byte)g, (byte)b);
            
            LinearGradientBrush brush = new LinearGradientBrush();
            brush.StartPoint = new Point(0, 0.5);
            brush.EndPoint = new Point(0, 1);
            brush.GradientStops.Add(new GradientStop(color, 0.0));
            brush.GradientStops.Add(new GradientStop(color2, 1.0));
            brush.Freeze();

            return brush;
        }
    }
}
