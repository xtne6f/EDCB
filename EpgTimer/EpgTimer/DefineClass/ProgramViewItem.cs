using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Media;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public class ViewPanelItem<T>
    {
        protected T data = default(T);

        public ViewPanelItem()
        {
            TitleDrawErr = false;
        }
        public ViewPanelItem(T info)
        {
            data = info;
            TitleDrawErr = false;
        }
        public T _Data
        {
            get { return data; }
            set { data = value; }
        }
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

        public static List<T> GetHitDataList<S>(List<S> list, Point cursorPos) where S : ViewPanelItem<T>
        {
            try
            {
                return list.FindAll(info => info == null ? false : info.IsPicked(cursorPos)).Select(info => info._Data).ToList();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return new List<T>();
        }
        public static List<T> GetDataList<S>(ICollection<S> list) where S : ViewPanelItem<T>
        {
            return list.OfType<S>().Select(info => info._Data).ToList();
        }
    }

    public class ProgramViewItem : ViewPanelItem<EpgEventInfo>
    {
        public ProgramViewItem(EpgEventInfo info) : base(info) { }
        public EpgEventInfo EventInfo
        {
            get { return _Data; }
            set { _Data = value; }
        }

        public Brush ContentColor
        {
            get
            {
                return CommonManager.Instance.VUtil.EventDataBorderBrush(EventInfo);
            }
        }
    }

    public static class ProgramViewItemEx
    {
        public static List<EpgEventInfo> GetHitDataList(this List<ProgramViewItem> list, Point cursorPos)
        {
            return ProgramViewItem.GetHitDataList(list, cursorPos);
        }
        public static List<EpgEventInfo> GetDataList(this ICollection<ProgramViewItem> list)
        {
            return ProgramViewItem.GetDataList(list);
        }
    }
}
