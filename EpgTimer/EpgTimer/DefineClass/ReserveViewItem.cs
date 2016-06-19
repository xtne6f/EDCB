using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Media;

namespace EpgTimer
{
    public class ReserveViewItem : ViewPanelItem<ReserveData>
    {
        public ReserveViewItem(ReserveData info) : base(info) { }
        public ReserveData ReserveInfo { get { return _data; } set { _data = value; } }

        public Brush ForeColorPriTuner
        {
            get
            {
                if (ReserveInfo == null) return Brushes.Black;

                return CommonManager.Instance.CustTunerServiceColorPri[ReserveInfo.RecSetting.Priority - 1];
            }
        }
        public Brush BackColorTuner
        {
            get
            {
                return ViewUtil.ReserveErrBrush(ReserveInfo);
            }
        }
        public String StatusTuner
        {
            get
            {
                if (ReserveInfo != null)
                {
                    if (ReserveInfo.IsOnRec() == true)
                    {
                        if (ReserveInfo.IsEnabled == false || ReserveInfo.OverlapMode == 2)
                        {
                            return "放送中*";
                        }
                        if (ReserveInfo.OverlapMode == 1)
                        {
                            return "一部のみ録画中*";
                        }
                        return "録画中*";
                    }
                }
                return "";
            }
        }
        public Brush BorderBrushTuner
        {
            get
            {
                if (ReserveInfo != null)
                {
                    if (ReserveInfo.IsOnRec() == true)
                    {
                        return CommonManager.Instance.StatRecForeColor;
                    }
                    if (ReserveInfo.IsEnabled == false)
                    {
                        return Brushes.Black;
                    }
                }
                return Brushes.DarkGray;
            }
        }
        public Brush BorderBrush
        {
            get
            {
                if (ReserveInfo != null)
                {
                    if (ReserveInfo.IsEnabled == false)
                    {
                        return CommonManager.Instance.CustContentColorList[0x12];
                    }
                    if (ReserveInfo.OverlapMode == 2)
                    {
                        return CommonManager.Instance.CustContentColorList[0x13];
                    }
                    if (ReserveInfo.OverlapMode == 1)
                    {
                        return CommonManager.Instance.CustContentColorList[0x14];
                    }
                    if (ReserveInfo.IsAutoAddInvalid == true)
                    {
                        return CommonManager.Instance.CustContentColorList[0x15];
                    }
                }
                return CommonManager.Instance.CustContentColorList[0x11];
            }
        }
    }

}
