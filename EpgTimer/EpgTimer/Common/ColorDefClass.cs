using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Media;

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
                string[] colorName = {"AliceBlue",
                "AntiqueWhite",
                "Aqua",
                "Aquamarine",
                "Azure",
                "Beige",
                "Bisque",
                "Black",
                "BlanchedAlmond",
                "Blue",
                "BlueViolet",
                "Brown",
                "BurlyWood",
                "CadetBlue" ,
                "Chartreuse",
                "Chocolate",
                "Coral",
                "CornflowerBlue",
                "Cornsilk",
                "Crimson",
                "Cyan",
                "DarkBlue",
                "DarkCyan",
                "DarkGoldenrod",
                "DarkGray",
                "DarkGreen",
                "DarkKhaki",
                "DarkMagenta",
                "DarkOliveGreen",
                "DarkOrange",
                "DarkOrchid",
                "DarkRed",
                "DarkSalmon",
                "DarkSeaGreen",
                "DarkSlateBlue",
                "DarkSlateGray",
                "DarkTurquoise",
                "DarkViolet",
                "DeepPink",
                "DeepSkyBlue",
                "DimGray",
                "DodgerBlue",
                "Firebrick",
                "FloralWhite",
                "ForestGreen",
                "Fuchsia",
                "Gainsboro",
                "GhostWhite",
                "Gold",
                "Goldenrod",
                "Gray",
                "Green",
                "GreenYellow",
                "Honeydew",
                "HotPink",
                "IndianRed",
                "Indigo",
                "Ivory",
                "Khaki",
                "Lavender",
                "LavenderBlush",
                "LawnGreen",
                "LemonChiffon",
                "LightBlue",
                "LightCoral",
                "LightCyan",
                "LightGoldenrodYellow",
                "LightGreen",
                "LightGray",
                "LightPink",
                "LightSalmon",
                "LightSeaGreen",
                "LightSkyBlue",
                "LightSlateGray",
                "LightSteelBlue",
                "LightYellow",
                "Lime",
                "LimeGreen",
                "Linen",
                "Magenta",
                "Maroon",
                "MediumAquamarine",
                "MediumBlue",
                "MediumOrchid",
                "MediumPurple",
                "MediumSeaGreen",
                "MediumSlateBlue",
                "MediumSpringGreen",
                "MediumTurquoise",
                "MediumVioletRed",
                "MidnightBlue",
                "MintCream",
                "MistyRose",
                "Moccasin",
                "NavajoWhite",
                "Navy",
                "OldLace",
                "Olive",
                "OliveDrab",
                "Orange",
                "OrangeRed",
                "Orchid",
                "PaleGoldenrod",
                "PaleGreen",
                "PaleTurquoise",
                "PaleVioletRed",
                "PapayaWhip",
                "PeachPuff",
                "Peru",
                "Pink",
                "Plum",
                "PowderBlue",
                "Purple",
                "Red",
                "RosyBrown",
                "RoyalBlue",
                "SaddleBrown",
                "Salmon",
                "SandyBrown",
                "SeaGreen",
                "SeaShell",
                "Sienna",
                "Silver",
                "SkyBlue",
                "SlateBlue",
                "SlateGray",
                "Snow",
                "SpringGreen",
                "SteelBlue",
                "Tan",
                "Teal",
                "Thistle",
                "Tomato",
                "Turquoise",
                "Violet",
                "Wheat",
                "White",
                "WhiteSmoke",
                "Yellow",
                "YellowGreen",
                "カスタム",
                };
                return colorName;
            }

        }

        public Dictionary<string, SolidColorBrush> ColorTable
        {
            get
            {
                if (colorTable == null)
                {
                    colorTable = new Dictionary<string, SolidColorBrush>();
                    colorTable.Add("AliceBlue", Brushes.AliceBlue);
                    colorTable.Add("AntiqueWhite", Brushes.AntiqueWhite);
                    colorTable.Add("Aqua", Brushes.Aqua);
                    colorTable.Add("Aquamarine", Brushes.Aquamarine);
                    colorTable.Add("Azure", Brushes.Azure);
                    colorTable.Add("Beige", Brushes.Beige);
                    colorTable.Add("Bisque", Brushes.Bisque);
                    colorTable.Add("Black", Brushes.Black);
                    colorTable.Add("BlanchedAlmond", Brushes.BlanchedAlmond);
                    colorTable.Add("Blue", Brushes.Blue);
                    colorTable.Add("BlueViolet", Brushes.BlueViolet);
                    colorTable.Add("Brown", Brushes.Brown);
                    colorTable.Add("BurlyWood", Brushes.BurlyWood);
                    colorTable.Add("CadetBlue", Brushes.CadetBlue);
                    colorTable.Add("Chartreuse", Brushes.Chartreuse);
                    colorTable.Add("Chocolate", Brushes.Chocolate);
                    colorTable.Add("Coral", Brushes.Coral);
                    colorTable.Add("CornflowerBlue", Brushes.CornflowerBlue);
                    colorTable.Add("Cornsilk", Brushes.Cornsilk);
                    colorTable.Add("Crimson", Brushes.Crimson);
                    colorTable.Add("Cyan", Brushes.Cyan);
                    colorTable.Add("DarkBlue", Brushes.DarkBlue);
                    colorTable.Add("DarkCyan", Brushes.DarkCyan);
                    colorTable.Add("DarkGoldenrod", Brushes.DarkGoldenrod);
                    colorTable.Add("DarkGray", Brushes.DarkGray);
                    colorTable.Add("DarkGreen", Brushes.DarkGreen);
                    colorTable.Add("DarkKhaki", Brushes.DarkKhaki);
                    colorTable.Add("DarkMagenta", Brushes.DarkMagenta);
                    colorTable.Add("DarkOliveGreen", Brushes.DarkOliveGreen);
                    colorTable.Add("DarkOrange", Brushes.DarkOrange);
                    colorTable.Add("DarkOrchid", Brushes.DarkOrchid);
                    colorTable.Add("DarkRed", Brushes.DarkRed);
                    colorTable.Add("DarkSalmon", Brushes.DarkSalmon);
                    colorTable.Add("DarkSeaGreen", Brushes.DarkSeaGreen);
                    colorTable.Add("DarkSlateBlue", Brushes.DarkSlateBlue);
                    colorTable.Add("DarkSlateGray", Brushes.DarkSlateGray);
                    colorTable.Add("DarkTurquoise", Brushes.DarkTurquoise);
                    colorTable.Add("DarkViolet", Brushes.DarkViolet);
                    colorTable.Add("DeepPink", Brushes.DeepPink);
                    colorTable.Add("DeepSkyBlue", Brushes.DeepSkyBlue);
                    colorTable.Add("DimGray", Brushes.DimGray);
                    colorTable.Add("DodgerBlue", Brushes.DodgerBlue);
                    colorTable.Add("Firebrick", Brushes.Firebrick);
                    colorTable.Add("FloralWhite", Brushes.FloralWhite);
                    colorTable.Add("ForestGreen", Brushes.ForestGreen);
                    colorTable.Add("Fuchsia", Brushes.Fuchsia);
                    colorTable.Add("Gainsboro", Brushes.Gainsboro);
                    colorTable.Add("GhostWhite", Brushes.GhostWhite);
                    colorTable.Add("Gold", Brushes.Gold);
                    colorTable.Add("Goldenrod", Brushes.Goldenrod);
                    colorTable.Add("Gray", Brushes.Gray);
                    colorTable.Add("Green", Brushes.Green);
                    colorTable.Add("GreenYellow", Brushes.GreenYellow);
                    colorTable.Add("Honeydew", Brushes.Honeydew);
                    colorTable.Add("HotPink", Brushes.HotPink);
                    colorTable.Add("IndianRed", Brushes.IndianRed);
                    colorTable.Add("Indigo", Brushes.Indigo);
                    colorTable.Add("Ivory", Brushes.Ivory);
                    colorTable.Add("Khaki", Brushes.Khaki);
                    colorTable.Add("Lavender", Brushes.Lavender);
                    colorTable.Add("LavenderBlush", Brushes.LavenderBlush);
                    colorTable.Add("LawnGreen", Brushes.LawnGreen);
                    colorTable.Add("LemonChiffon", Brushes.LemonChiffon);
                    colorTable.Add("LightBlue", Brushes.LightBlue);
                    colorTable.Add("LightCoral", Brushes.LightCoral);
                    colorTable.Add("LightCyan", Brushes.LightCyan);
                    colorTable.Add("LightGoldenrodYellow", Brushes.LightGoldenrodYellow);
                    colorTable.Add("LightGreen", Brushes.LightGreen);
                    colorTable.Add("LightGray", Brushes.LightGray);
                    colorTable.Add("LightPink", Brushes.LightPink);
                    colorTable.Add("LightSalmon", Brushes.LightSalmon);
                    colorTable.Add("LightSeaGreen", Brushes.LightSeaGreen);
                    colorTable.Add("LightSkyBlue", Brushes.LightSkyBlue);
                    colorTable.Add("LightSlateGray", Brushes.LightSlateGray);
                    colorTable.Add("LightSteelBlue", Brushes.LightSteelBlue);
                    colorTable.Add("LightYellow", Brushes.LightYellow);
                    colorTable.Add("Lime", Brushes.Lime);
                    colorTable.Add("LimeGreen", Brushes.LimeGreen);
                    colorTable.Add("Linen", Brushes.Linen);
                    colorTable.Add("Magenta", Brushes.Magenta);
                    colorTable.Add("Maroon", Brushes.Maroon);
                    colorTable.Add("MediumAquamarine", Brushes.MediumAquamarine);
                    colorTable.Add("MediumBlue", Brushes.MediumBlue);
                    colorTable.Add("MediumOrchid", Brushes.MediumOrchid);
                    colorTable.Add("MediumPurple", Brushes.MediumPurple);
                    colorTable.Add("MediumSeaGreen", Brushes.MediumSeaGreen);
                    colorTable.Add("MediumSlateBlue", Brushes.MediumSlateBlue);
                    colorTable.Add("MediumSpringGreen", Brushes.MediumSpringGreen);
                    colorTable.Add("MediumTurquoise", Brushes.MediumTurquoise);
                    colorTable.Add("MediumVioletRed", Brushes.MediumVioletRed);
                    colorTable.Add("MidnightBlue", Brushes.MidnightBlue);
                    colorTable.Add("MintCream", Brushes.MintCream);
                    colorTable.Add("MistyRose", Brushes.MistyRose);
                    colorTable.Add("Moccasin", Brushes.Moccasin);
                    colorTable.Add("NavajoWhite", Brushes.NavajoWhite);
                    colorTable.Add("Navy", Brushes.Navy);
                    colorTable.Add("OldLace", Brushes.OldLace);
                    colorTable.Add("Olive", Brushes.Olive);
                    colorTable.Add("OliveDrab", Brushes.OliveDrab);
                    colorTable.Add("Orange", Brushes.Orange);
                    colorTable.Add("OrangeRed", Brushes.OrangeRed);
                    colorTable.Add("Orchid", Brushes.Orchid);
                    colorTable.Add("PaleGoldenrod", Brushes.PaleGoldenrod);
                    colorTable.Add("PaleGreen", Brushes.PaleGreen);
                    colorTable.Add("PaleTurquoise", Brushes.PaleTurquoise);
                    colorTable.Add("PaleVioletRed", Brushes.PaleVioletRed);
                    colorTable.Add("PapayaWhip", Brushes.PapayaWhip);
                    colorTable.Add("PeachPuff", Brushes.PeachPuff);
                    colorTable.Add("Peru", Brushes.Peru);
                    colorTable.Add("Pink", Brushes.Pink);
                    colorTable.Add("Plum", Brushes.Plum);
                    colorTable.Add("PowderBlue", Brushes.PowderBlue);
                    colorTable.Add("Purple", Brushes.Purple);
                    colorTable.Add("Red", Brushes.Red);
                    colorTable.Add("RosyBrown", Brushes.RosyBrown);
                    colorTable.Add("RoyalBlue", Brushes.RoyalBlue);
                    colorTable.Add("SaddleBrown", Brushes.SaddleBrown);
                    colorTable.Add("Salmon", Brushes.Salmon);
                    colorTable.Add("SandyBrown", Brushes.SandyBrown);
                    colorTable.Add("SeaGreen", Brushes.SeaGreen);
                    colorTable.Add("SeaShell", Brushes.SeaShell);
                    colorTable.Add("Sienna", Brushes.Sienna);
                    colorTable.Add("Silver", Brushes.Silver);
                    colorTable.Add("SkyBlue", Brushes.SkyBlue);
                    colorTable.Add("SlateBlue", Brushes.SlateBlue);
                    colorTable.Add("SlateGray", Brushes.SlateGray);
                    colorTable.Add("Snow", Brushes.Snow);
                    colorTable.Add("SpringGreen", Brushes.SpringGreen);
                    colorTable.Add("SteelBlue", Brushes.SteelBlue);
                    colorTable.Add("Tan", Brushes.Tan);
                    colorTable.Add("Teal", Brushes.Teal);
                    colorTable.Add("Thistle", Brushes.Thistle);
                    colorTable.Add("Tomato", Brushes.Tomato);
                    colorTable.Add("Turquoise", Brushes.Turquoise);
                    colorTable.Add("Violet", Brushes.Violet);
                    colorTable.Add("Wheat", Brushes.Wheat);
                    colorTable.Add("White", Brushes.White);
                    colorTable.Add("WhiteSmoke", Brushes.WhiteSmoke);
                    colorTable.Add("Yellow", Brushes.Yellow);
                    colorTable.Add("YellowGreen", Brushes.YellowGreen);
                    colorTable.Add("カスタム", Brushes.White);
                }
                return colorTable;
            }
        }
    }
}
