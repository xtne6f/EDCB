using System;
using System.Windows.Media;
using System.Windows.Controls;

namespace EpgTimer
{
    public class DataListItemBase : GridViewSorterItem
    {
        public virtual TextBlock ToolTipView
        {
            get
            {
                if (Settings.Instance.NoToolTip == true) return null;
                //
                return ToolTipViewAlways;
            }
        }
        public virtual TextBlock ToolTipViewAlways
        {
            get { return ViewUtil.GetTooltipBlockStandard(ConvertInfoText()); }
        }
        public virtual String ConvertInfoText() { return ""; }

        public virtual int NowJumpingTable { set; get; }
        public virtual Brush ForeColor
        {
            get
            {
                //番組表へジャンプ時の強調表示
                switch (NowJumpingTable)
                {
                    case 1: return Brushes.Red;
                    case 2: return CommonManager.Instance.ListDefForeColor;
                }
                return CommonManager.Instance.ListDefForeColor;
            }
        }
        public virtual Brush BackColor
        {
            get
            {
                //番組表へジャンプ時の強調表示
                switch (NowJumpingTable)
                {
                    case 1: return CommonManager.Instance.ResDefBackColor;
                    case 2: return Brushes.Red;
                }
                return CommonManager.Instance.ResDefBackColor;
            }
        }
        public virtual Brush BorderBrush { get { return null; } }
    }
}
