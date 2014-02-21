using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class TunerInfo
    {
        public String BonDriver
        {
            get;
            set;
        }
        public String TunerNum
        {
            get;
            set;
        }
        public String EPGNum
        {
            get;
            set;
        }
        public bool IsEpgCap
        {
            get;
            set;
        }
        public override string ToString()
        {
            return BonDriver;
        }
    }
}
