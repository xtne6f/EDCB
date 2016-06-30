using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ComponentModel;

namespace EpgTimer
{
    public class EpgCaptime : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        private bool selected = false;

        private void NotifyPropertyChanged(String info)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(info));
            }
        }

        public bool IsSelected
        {
            get
            {
                return this.selected;
            }
            set
            {
                this.selected = value;
                NotifyPropertyChanged("IsSelected");
            }
        }

        public string Time
        {
            get;
            set;
        }
        public bool BSBasicOnly
        {
            get;
            set;
        }
        public bool CS1BasicOnly
        {
            get;
            set;
        }
        public bool CS2BasicOnly
        {
            get;
            set;
        }
        public bool CS3BasicOnly
        {
            get;
            set;
        }

        public string ViewTime
        {
            get
            {
                int i = Time.IndexOf('w');
                if (i >= 0)
                {
                    uint wday;
                    uint.TryParse(Time.Substring(i + 1), out wday);
                    return "日月火水木金土"[(int)(wday % 7)] + " " + Time.Substring(0, i);
                }
                return Time;
            }
        }

        public string ViewBasicOnly
        {
            get
            {
                return (BSBasicOnly ? "基" : "詳") + "," + (CS1BasicOnly ? "基" : "詳") + "," + (CS2BasicOnly ? "基" : "詳") + "," + (CS3BasicOnly ? "基" : "詳");
            }
        }
    }
}
