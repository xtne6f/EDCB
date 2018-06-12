using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class IEPGStationInfo
    {
        public String StationName
        {
            get;
            set;
        }
        public UInt64 Key
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
