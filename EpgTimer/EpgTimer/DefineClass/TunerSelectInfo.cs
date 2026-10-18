using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class TunerSelectInfo
    {
        public TunerSelectInfo(string name, uint id)
        {
            Name = name;
            ID = id;
        }
        public string Name
        {
            get;
            set;
        }
        public uint ID
        {
            get;
            set;
        }
        public override string ToString()
        {
            string view = "";
            if (ID == 0)
            {
                view = "自動";
            }
            else
            {
                view = "ID:" + ID.ToString("X8") + " (" + Name + ")";
            }
            return view;
        }
    }
}
