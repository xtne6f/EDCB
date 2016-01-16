using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    public enum CtxmCode
    {
        ReserveView,
        TunerReserveView,
        RecInfoView,
        EpgAutoAddView,
        ManualAutoAddView,
        EpgView,
        SearchWindow,
        EditChgMenu,    //編集サブメニュー編集用
        EtcWindow       //ショートカットメニューがないダイアログなど用のダミー
    }

    public class CtxmData
    {
        public CtxmCode ctxmCode { set; get; }
        public List<CtxmItemData> Items { set; get; }

        public CtxmData() { Items = new List<CtxmItemData>(); }
//        public CtxmData(CtxmData data) { CopyData(data, this); }

            //デフォルト定義用
        public CtxmData(CtxmCode code, List<CtxmItemData> data = null)
        {
            ctxmCode= code;
            Items = new List<CtxmItemData>();
            if (data != null) Items.AddRange(data);
        }

        public CtxmData Clone() { return CopyObj.Clone(this, CopyData); }
        protected static void CopyData(CtxmData src, CtxmData dest)
        {
            dest.ctxmCode = src.ctxmCode;
            dest.Items = src.Items.Clone();
        }
    }

    public class CtxmItemData
    {
        public string Header { set; get; }
        public int ID { set; get; }
        public ICommand Command { set; get; }
        public List<CtxmItemData> Items { set; get; }

        public CtxmItemData() { Items = new List<CtxmItemData>(); }
        public CtxmItemData(CtxmItemData data) { CopyData(data, this); }

        //デフォルト定義用
        public CtxmItemData(string header, ICommand icmd = null, int id = 0)
        {
            Header = header;
            ID = id;
            Command = icmd;
            Items = new List<CtxmItemData>();
        }

        //デフォルト定義用、ヘッダ以外コピー
        public CtxmItemData(string header, CtxmItemData reference)
        {
            CopyData(reference, this); 
            Header = header;
        }

        public static List<CtxmItemData> Clone(List<CtxmItemData> src) { return CopyObj.Clone(src, CtxmItemData.CopyData); }
        public CtxmItemData Clone() { return CopyObj.Clone(this, CopyData); }
        protected static void CopyData(CtxmItemData src, CtxmItemData dest)
        {
            dest.Header = src.Header;
            dest.ID = src.ID;
            dest.Command = src.Command;
            dest.Items = src.Items.Clone();
        }
    }

    public static class CtxmItemDataEx
    {
        public static List<CtxmItemData> Clone(this List<CtxmItemData> src) { return CtxmItemData.Clone(src); }
    }

}
