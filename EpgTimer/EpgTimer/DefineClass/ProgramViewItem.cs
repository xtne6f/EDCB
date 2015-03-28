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
}
