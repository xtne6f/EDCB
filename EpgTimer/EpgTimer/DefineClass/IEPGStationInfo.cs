using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class IEPGStationInfo
    {
        public string StationName
        {
            get;
            set;
        }
        public ulong Key
        {
            get;
            set;
        }
        public IEPGStationInfo DeepClone()
        {
            return (IEPGStationInfo)MemberwiseClone();
        }
        public override string ToString()
        {
            return StationName;
        }
    }
}
