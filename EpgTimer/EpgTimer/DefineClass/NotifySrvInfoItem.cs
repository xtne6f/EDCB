using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class NotifySrvInfoItem
    {
        public NotifySrvInfoItem(NotifySrvInfo info)
        {
            Time = info.time.ToString("yyyy/MM/dd HH:mm:ss.fff");
            UpdateNotifyItem notifyID = (UpdateNotifyItem)info.notifyID;
            Title = notifyID == UpdateNotifyItem.PreRecStart ? "予約録画開始準備" :
                    notifyID == UpdateNotifyItem.RecStart ? "録画開始" :
                    notifyID == UpdateNotifyItem.RecEnd ? "録画終了" :
                    notifyID == UpdateNotifyItem.RecTuijyu ? "追従発生" :
                    notifyID == UpdateNotifyItem.ChgTuijyu ? "番組変更" :
                    notifyID == UpdateNotifyItem.PreEpgCapStart ? "EPG取得" :
                    notifyID == UpdateNotifyItem.EpgCapStart ? "EPG取得" :
                    notifyID == UpdateNotifyItem.EpgCapEnd ? "EPG取得" : info.notifyID.ToString();
            LogText = notifyID == UpdateNotifyItem.EpgCapStart ? "開始" :
                      notifyID == UpdateNotifyItem.EpgCapEnd ? "終了" : info.param4.Replace("\r\n", "  ");
        }
        public NotifySrvInfoItem(string text)
        {
            string[] s = text.Split(new char[] { '[', ']' }, 3);
            Time = s.Length > 0 ? s[0].TrimEnd(' ') : "";
            Title = s.Length > 1 ? s[1] : "";
            LogText = s.Length > 2 ? s[2].TrimStart(' ') : "";
        }
        public String Time
        {
            get;
            private set;
        }
        public String Title
        {
            get;
            private set;
        }

        public String LogText
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return Time + " [" + Title + "] " + LogText;
        }
    }
}
