using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Media;

namespace EpgTimer
{
    public class PanelItem
    {
        public double Width { get; set; }
        public double Height { get; set; }
        public double LeftPos { get; set; }
        public double TopPos { get; set; }

        public bool TitleDrawErr { get; set; }

        public bool IsPicked(Point cursorPos)
        {
            return LeftPos <= cursorPos.X && cursorPos.X < LeftPos + Width &&
                    TopPos <= cursorPos.Y && cursorPos.Y < TopPos + Height;
        }
    }

    public class PanelItem<T> : PanelItem
    {
        public T Data { get; protected set; }
        public PanelItem(T info) : base() { Data = info; }
    }

    public static class ViewPanelItemEx
    {
        public static List<T> GetHitDataList<T>(this IEnumerable<PanelItem<T>> list, Point cursorPos)
        {
            return list.Where(info => info != null && info.IsPicked(cursorPos)).Select(info => info.Data).ToList();
        }
        public static List<T> GetDataList<T>(this IEnumerable<PanelItem<T>> list)
        {
            return list.Where(info => info != null).Select(info => info.Data).ToList();
        }
    }

    public class ProgramViewItem : PanelItem<EpgEventInfo>
    {
        public ProgramViewItem(EpgEventInfo info) : base(info) { }
        public EpgEventInfo EventInfo { get { return Data; } protected set { Data = value; } }

        public Brush ContentColor
        {
            get
            {
                return ViewUtil.EpgDataContentBrush(EventInfo);
            }
        }
        public Brush BorderBrush
        {
            get
            {
                return CommonManager.Instance.EpgBorderColor;
            }
        }
    }

}
