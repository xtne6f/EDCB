using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace EpgTimer
{
    /// <summary>
    /// SettingWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SettingWindow : Window
    {
        public SettingWindow()
        {
            InitializeComponent();

            DataContext = Settings.Instance.DeepCloneStaticSettings();

            CheckServiceSettings((Settings)DataContext, false);
        }

        private void button_OK_Click(object sender, RoutedEventArgs e)
        {
            Settings.Instance.ShallowCopyDynamicSettingsTo((Settings)DataContext);
            Settings.Instance = (Settings)DataContext;
            setAppView.SaveSetting();
            setEpgView.SaveSetting();
            setOtherAppView.SaveSetting();

            Settings.SaveToXmlFile(true);
            CommonManager.Instance.ReloadCustContentColorList();

            this.DialogResult = true;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            this.DialogResult = false;
        }

        private static void CheckServiceSettings(Settings settings, bool apply)
        {
            //サービス一覧に見つからず、TSIDを無視すれば見つかるサービスがないか調べる
            string searchDefault = "";
            for (int i = 0; i < settings.SearchKeyServiceList.Count; i++)
            {
                ulong id = (ulong)settings.SearchKeyServiceList[i];
                if (ChSet5.Instance.ChList.ContainsKey(id) == false)
                {
                    ChSet5Item item = ChSet5.Instance.ChList.Values.FirstOrDefault(a => (a.Key & 0xFFFF0000FFFF) == (id & 0xFFFF0000FFFF));
                    if (item != null)
                    {
                        if (apply)
                        {
                            settings.SearchKeyServiceList[i] = (long)item.Key;
                        }
                        else if (searchDefault.Count(c => c == '\n') < 5)
                        {
                            searchDefault += "  ID=0x" + id.ToString("X12") + " -> 0x" + item.Key.ToString("X12") + " (" + item.ServiceName + ")\r\n";
                        }
                        else if (searchDefault.EndsWith(".\r\n") == false)
                        {
                            searchDefault += "  ...\r\n";
                        }
                    }
                }
            }

            string viewService = "";
            string searchKey = "";
            foreach (CustomEpgTabInfo info in settings.CustomEpgTabList)
            {
                for (int i = 0; i < info.ViewServiceList.Count; i++)
                {
                    ulong id = info.ViewServiceList[i];
                    if (ChSet5.Instance.ChList.ContainsKey(id) == false)
                    {
                        ChSet5Item item = ChSet5.Instance.ChList.Values.FirstOrDefault(a => (a.Key & 0xFFFF0000FFFF) == (id & 0xFFFF0000FFFF));
                        if (item != null)
                        {
                            if (apply)
                            {
                                info.ViewServiceList[i] = item.Key;
                            }
                            else if (viewService.Count(c => c == '\n') < 5)
                            {
                                viewService += "  ID=0x" + id.ToString("X12") + " -> 0x" + item.Key.ToString("X12") + " (" + item.ServiceName + ")\r\n";
                            }
                            else if (viewService.EndsWith(".\r\n") == false)
                            {
                                viewService += "  ...\r\n";
                            }
                        }
                    }
                }
                for (int i = 0; i < info.SearchKey.serviceList.Count; i++)
                {
                    ulong id = (ulong)info.SearchKey.serviceList[i];
                    if (ChSet5.Instance.ChList.ContainsKey(id) == false)
                    {
                        ChSet5Item item = ChSet5.Instance.ChList.Values.FirstOrDefault(a => (a.Key & 0xFFFF0000FFFF) == (id & 0xFFFF0000FFFF));
                        if (item != null)
                        {
                            if (apply)
                            {
                                info.SearchKey.serviceList[i] = (long)item.Key;
                            }
                            else if (searchKey.Count(c => c == '\n') < 5)
                            {
                                searchKey += "  ID=0x" + id.ToString("X12") + " -> 0x" + item.Key.ToString("X12") + " (" + item.ServiceName + ")\r\n";
                            }
                            else if (searchKey.EndsWith(".\r\n") == false)
                            {
                                searchKey += "  ...\r\n";
                            }
                        }
                    }
                }
            }

            string iepgStation = "";
            foreach (IEPGStationInfo info in settings.IEpgStationList)
            {
                if (ChSet5.Instance.ChList.ContainsKey(info.Key) == false)
                {
                    ChSet5Item item = ChSet5.Instance.ChList.Values.FirstOrDefault(a => (a.Key & 0xFFFF0000FFFF) == (info.Key & 0xFFFF0000FFFF));
                    if (item != null)
                    {
                        if (apply)
                        {
                            info.Key = item.Key;
                        }
                        else if (iepgStation.Count(c => c == '\n') < 5)
                        {
                            iepgStation += "  ID=0x" + info.Key.ToString("X12") + " -> 0x" + item.Key.ToString("X12") + " (" + item.ServiceName + ")\r\n";
                        }
                        else if (iepgStation.EndsWith(".\r\n") == false)
                        {
                            iepgStation += "  ...\r\n";
                        }
                    }
                }
            }

            if (searchDefault != "" || viewService != "" || searchKey != "" || iepgStation != "")
            {
                if (MessageBox.Show("TransportStreamIDの変更を検出しました。\r\n\r\n" +
                                    (searchDefault != "" ? "【検索条件のデフォルト値のサービス絞り込み】\r\n" : "") + searchDefault +
                                    (viewService != "" ? "【番組表の表示条件の表示サービス】\r\n" : "") + viewService +
                                    (searchKey != "" ? "【番組表の表示条件の検索条件のサービス絞り込み】\r\n" : "") + searchKey +
                                    (iepgStation != "" ? "【iEPG Ver.1の放送局リスト】\r\n" : "") + iepgStation +
                                    "\r\n変更を設定ウィンドウに適用しますか？\r\n（設定ウィンドウをOKで閉じるまで変更は保存されません）",
                                    "", MessageBoxButton.YesNo) == MessageBoxResult.Yes)
                {
                    CheckServiceSettings(settings, true);
                }
            }
        }
    }
}
