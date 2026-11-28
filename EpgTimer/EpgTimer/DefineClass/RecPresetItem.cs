using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class RecPresetItem
    {
        public RecPresetItem()
        {
        }
        public string DisplayName
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
            return DisplayName;
        }
    }
}
