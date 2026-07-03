using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class ContentKindInfo
    {
        public ushort ID
        {
            get
            {
                return (ushort)(Nibble1 << 8 | Nibble2);
            }
        }
        public byte Nibble1
        {
            get;
            set;
        }
        public byte Nibble2
        {
            get;
            set;
        }
        public string ListBoxView
        {
            get
            {
                string name;
                if (ID == 0xFEFF)
                {
                    //表示用の特殊値
                    name = "(全ジャンル)";
                }
                else if (CommonManager.Instance.ContentKindDictionary.TryGetValue((ushort)(ID | 0xFF), out name) == false)
                {
                    name = "(0x" + Nibble1.ToString("X2") + ")";
                }
                if (Nibble2 != 0xFF)
                {
                    string subName;
                    if (CommonManager.Instance.ContentKindDictionary.TryGetValue(ID, out subName) == false)
                    {
                        subName = "(0x" + Nibble2.ToString("X2") + ")";
                    }
                    name += " - " + subName;
                }
                return name;
            }
        }
        public ContentKindInfo DeepClone()
        {
            return (ContentKindInfo)MemberwiseClone();
        }
        public override string ToString()
        {
            string name;
            if (CommonManager.Instance.ContentKindDictionary.TryGetValue(ID, out name) == false)
            {
                name = "(0x" + (Nibble2 != 0xFF ? Nibble2 : Nibble1).ToString("X2") + ")";
            }
            return (Nibble2 != 0xFF ? "  " : "") + name;
        }
    }
}
