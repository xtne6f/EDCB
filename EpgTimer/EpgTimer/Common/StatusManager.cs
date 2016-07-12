using System;
using System.Windows;
using System.Windows.Input;

namespace EpgTimer
{
    using UserCtrlView;

    static class StatusManager
    {
        private static RoutedCommand CmdGetStatusbar = new RoutedCommand();
        private class StatusBarObj { public StatusView sbar;}

        private static StatusView GetStatusbar(IInputElement target = null)
        {
            var param = new StatusBarObj();
            CmdGetStatusbar.Execute(param, target);
            return param.sbar ?? ViewUtil.MainWindow.statusBar;
        }
        public static void RegisterStatusbar(StatusView status_bar, UIElement target)
        {
            target.CommandBindings.Add(new CommandBinding(CmdGetStatusbar, (sender, e) =>
            {
                (e.Parameter as StatusBarObj).sbar = status_bar;
            }));
        }

        public static void StatusNotifySet(bool success, string subject, IInputElement target = null)
        {
            if (string.IsNullOrEmpty(subject)) return;
            StatusNotifySet((success == true ? "" : "中断またはキャンセルされました < ") + subject, null, target);
        }
        public static void StatusNotifySet(string s3, TimeSpan? interval = null, IInputElement target = null)
        {
            if (Settings.Instance.DisplayStatus == false || Settings.Instance.DisplayStatusNotify == false) return;
            GetStatusbar(target).SetText(s3: s3, interval: interval);
        }
        public static void StatusNotifyAppend(string s3, TimeSpan? interval = null, IInputElement target = null)
        {
            if (Settings.Instance.DisplayStatus == false || Settings.Instance.DisplayStatusNotify == false) return;
            GetStatusbar(target).AppendText(s3: s3, interval: interval);
        }
        public static void StatusSet(string s1 = null, string s2 = null, string s3 = null, IInputElement target = null)
        {
            if (Settings.Instance.DisplayStatus == false) return;
            GetStatusbar(target).SetText(s1, s2, s3);
        }
        public static void StatusAppend(string s1 = "", string s2 = "", string s3 = "", IInputElement target = null)
        {
            if (Settings.Instance.DisplayStatus == false) return;
            GetStatusbar(target).AppendText(s1, s2, s3);
        }
    }
}
