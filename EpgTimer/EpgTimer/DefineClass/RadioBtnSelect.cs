using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Controls;

namespace EpgTimer
{
    class RadioBtnSelect
    {
        public List<RadioButton> Items { get; private set; }

        public RadioBtnSelect(params RadioButton[] radioButtonList) 
        {
            Items = radioButtonList.ToList();
        }

        /// <summary>未選択は-1。</summary>
        public int Value 
        {
            get { return Items.FindIndex(btn => btn.IsChecked == true); }
            set
            {
                for (int i = 0; i < Items.Count; i++)
                {
                    Items[i].IsChecked = (i == value);
                }
            }
        }
    }
}
