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
        private bool basiconly = false;

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

        public string txtBasicOnly
        {
            get
            {
                if (this.basiconly)
                {
                    return "基本のみ";
                }
                else
                {
                    return "";
                }
            }
        }

        public bool IsBasicOnly
        {
            get
            {
                return this.basiconly;
            }
            set
            {
                this.basiconly = value;
                NotifyPropertyChanged("IsBasicOnly");
            }
        }

        public string Time
        {
            get;
            set;
        }
    }
}
