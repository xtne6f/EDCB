﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Media;
using System.Reflection;
using System.Windows;

namespace EpgTimer
{
    public static class ColorDef
    {
        public static Color ColorFromName(string name)
        {
            try
            {
                return (Color)ColorConverter.ConvertFromString(name);//#形式でも大丈夫
                //return (Color)typeof(Colors).GetProperty(name).GetValue(null, null);
            }
            catch
            {
                return Colors.White;
            }
        }
        //未使用
        //public static SolidColorBrush BrushFromName(string name) { return new SolidColorBrush(ColorFromName(name)); }

        public static Color FromUInt(UInt32 value)
        {
            return Color.FromArgb((byte)(value >> 24), (byte)(value >> 16), (byte)(value >> 8), (byte)value);
        }
        public static UInt32 ToUInt(Color c)
        {
            return ((UInt32)c.A) << 24 | ((UInt32)c.R) << 16 | ((UInt32)c.G) << 8 | (UInt32)c.B;
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

            var color2 = Color.FromArgb(color.A, (byte)r, (byte)g, (byte)b);
            
            var brush = new LinearGradientBrush();
            brush.StartPoint = new Point(0, 0.5);
            brush.EndPoint = new Point(0, 1);
            brush.GradientStops.Add(new GradientStop(color, 0.0));
            brush.GradientStops.Add(new GradientStop(color2, 1.0));
            return brush;
        }
    }
}
