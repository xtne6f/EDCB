using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public class ReserveViewItem : ViewPanelItem<ReserveData>
    {
        public ReserveViewItem(ReserveData info) : base(info) { }
        public ReserveData ReserveInfo
        {
            get { return _Data; }
            set { _Data = value; }
        }
    }

    public static class ReserveViewItemEx
    {
        public static List<ReserveData> GetHitDataList(this List<ReserveViewItem> list, Point cursorPos)
        {
            return ReserveViewItem.GetHitDataList(list, cursorPos);
        }
        public static List<ReserveData> GetDataList(this ICollection<ReserveViewItem> list)
        {
            return ReserveViewItem.GetDataList(list);
        }
    }
}
