using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Media;
using System.Reflection;

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
    }
}
