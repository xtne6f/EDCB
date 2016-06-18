using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Media;

namespace EpgTimer
{
    public class ViewPanelItemBase
    {
        public ViewPanelItemBase() { TitleDrawErr = false; }

        public double Width { get; set; }
        public double Height { get; set; }
        public double LeftPos { get; set; }
        public double TopPos { get; set; }

        public double HeightDef { get; set; }
        public double TopPosDef { get; set; }

        public bool TitleDrawErr { get; set; }

        public bool IsPicked(Point cursorPos)
        {
            return LeftPos <= cursorPos.X && cursorPos.X < LeftPos + Width &&
                    TopPos <= cursorPos.Y && cursorPos.Y < TopPos + Height;
        }
    }

    public class ViewPanelItem<T> : ViewPanelItemBase
    {
        protected T _data = default(T);
        public T Data { get { return _data; } set { _data = value; } }
        public ViewPanelItem(T info) : base() { _data = info; }
    }

    public static class ViewPanelItemEx
    {
        public static List<T> GetHitDataList<T>(this IEnumerable<ViewPanelItem<T>> list, Point cursorPos)
        {
            return list.Where(info => info != null && info.IsPicked(cursorPos)).Select(info => info.Data).ToList();
        }
        public static List<T> GetDataList<T>(this IEnumerable<ViewPanelItem<T>> list)
        {
            return list.Where(info => info != null).Select(info => info.Data).ToList();
        }
    }

    public class ProgramViewItem : ViewPanelItem<EpgEventInfo>
    {
        public ProgramViewItem(EpgEventInfo info) : base(info) { }
        public EpgEventInfo EventInfo { get { return _data; } set { _data = value; } }

        public Brush ContentColor
        {
            get
            {
                return ViewUtil.EpgDataContentBrush(EventInfo);
            }
        }
    }

}
