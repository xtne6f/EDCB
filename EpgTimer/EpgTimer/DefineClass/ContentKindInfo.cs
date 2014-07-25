using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class ContentKindInfo
    {
        public ContentKindInfo()
        {
        }
        public ContentKindInfo(String contentName, String subName, Byte nibble1, Byte nibble2)
        {
            this.ContentName = contentName;
            this.SubName = subName;
            this.Nibble1 = nibble1;
            this.Nibble2 = nibble2;
            this.ID = (UInt16)(((UInt16)nibble1) << 8 | nibble2);
        }
        public UInt16 ID
        {
            get;
            set;
        }
        public String ContentName
        {
            get;
            set;
        }
        public String SubName
        {
            get;
            set;
        }
        public Byte Nibble1
        {
            get;
            set;
        }
        public Byte Nibble2
        {
            get;
            set;
        }
        public String ListBoxView
        {
            get
            {
                return ContentName + (SubName == "" ? "" : " - " + SubName); 
            }
        }
        public override string ToString()
        {
            if (Nibble2 == 0xFF)
            {
                return ContentName;
            }
            else
            {
                return "  " + SubName;
            }
        }
        public String ToolTipView
        {
            get
            {
                if (Settings.Instance.NoToolTip == true)
                {
                    return null;
                }
                String viewTip = "";
                if (Nibble2 == 0xFF)
                {
                    viewTip = ContentName + "\r\n" +
                        "id : " + Nibble1.ToString() + "(0x" + Nibble1.ToString("X2") + ")";
                }
                else
                {
                    viewTip = ContentName + " : " + SubName + "\r\n" +
                        "id : " + Nibble1.ToString() + "(0x" + Nibble1.ToString("X2") + ")" + "\r\n" +
                        "sub_id : " + Nibble2.ToString() + "(0x" + Nibble2.ToString("X2") + ")";
                }
                return viewTip;
            }
        }
    }
}
