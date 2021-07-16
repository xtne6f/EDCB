using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace EpgTimer
{
    /// <summary>
    /// RecInfoDescWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class RecInfoDescWindow : Window
    {
        private RecFileInfo recInfo = null;

        public RecInfoDescWindow()
        {
            InitializeComponent();
        }

        public void SetRecInfo(RecFileInfo info)
        {
            recInfo = info;
            EpgEventInfo eventInfo = null;
            if (info.ProgramInfo.Length == 0 && info.EventID != 0xFFFF)
            {
                // 過去番組情報を探してみる
                var arcList = new List<EpgServiceEventInfo>();
                if (CommonManager.CreateSrvCtrl().SendEnumPgArc(new List<long> {
                        0, (long)CommonManager.Create64Key(info.OriginalNetworkID, info.TransportStreamID, info.ServiceID),
                        info.StartTime.ToFileTimeUtc(), info.StartTime.ToFileTimeUtc() + 1 }, ref arcList) == ErrCode.CMD_SUCCESS &&
                    arcList.Count > 0 && arcList[0].eventList.Count > 0)
                {
                    eventInfo = arcList[0].eventList[0];
                }
                else
                {
                    // 番組情報を探してみる
                    eventInfo = CommonManager.Instance.DB.GetPgInfo(info.OriginalNetworkID, info.TransportStreamID,
                                                                    info.ServiceID, info.EventID, false);
                    if (eventInfo == null || eventInfo.StartTimeFlag == 0 || eventInfo.start_time != info.StartTime)
                    {
                        eventInfo = null;
                    }
                }
            }
            if (eventInfo != null)
            {
                richTextBox_pgInfo.Document = new FlowDocument(CommonManager.ConvertDisplayText(
                    CommonManager.ConvertProgramText(eventInfo, EventInfoTextMode.BasicInfo) +
                    CommonManager.ConvertProgramText(eventInfo, EventInfoTextMode.BasicText),
                    CommonManager.ConvertProgramText(eventInfo, EventInfoTextMode.ExtendedText),
                    CommonManager.ConvertProgramText(eventInfo, EventInfoTextMode.PropertyInfo)));
            }
            else
            {
                // 詳細情報を分離してみる
                string basicInfo = info.ProgramInfo;
                string extText = "";
                string propertyInfo = "";
                Match m = Regex.Match(basicInfo, @"^([\s\S]*?\r?\n\r?\n[\s\S]*?\r?\n\r?\n)(詳細情報\r?\n[\s\S]*?\r?\n\r?\n\r?\n)([\s\S]*)$");
                if (m.Success)
                {
                    basicInfo = m.Groups[1].Value;
                    extText = m.Groups[2].Value;
                    propertyInfo = m.Groups[3].Value;
                }
                richTextBox_pgInfo.Document = new FlowDocument(CommonManager.ConvertDisplayText(basicInfo, extText, propertyInfo));
            }
            textBox_errLog.Text = info.ErrInfo;
            textBox_recFilePath.Text = info.RecFilePath;
            button_rename.IsEnabled = false;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            tabItem_pgInfo.Focus();
        }

        private void textBox_recFilePath_TextChanged(object sender, TextChangedEventArgs e)
        {
            button_rename.IsEnabled = CommonManager.Instance.NWMode == false &&
                                      recInfo != null &&
                                      recInfo.RecFilePath.Length > 0 &&
                                      textBox_recFilePath.Text.Length > 0 &&
                                      recInfo.RecFilePath != textBox_recFilePath.Text;
        }

        private void button_rename_Click(object sender, RoutedEventArgs e)
        {
            if (CommonManager.Instance.NWMode == false && recInfo != null && recInfo.RecFilePath.Length > 0)
            {
                string destPath = null;
                try
                {
                    // 絶対パスであること
                    string path = textBox_recFilePath.Text;
                    if (Path.GetFullPath(path).Equals(path, StringComparison.OrdinalIgnoreCase))
                    {
                        // 拡張子は変更できない
                        if (Path.GetExtension(path).Equals(Path.GetExtension(recInfo.RecFilePath), StringComparison.OrdinalIgnoreCase))
                        {
                            // 移動先のディレクトリは存在しなければならない
                            if (Directory.Exists(Path.GetDirectoryName(path)))
                            {
                                destPath = path;
                            }
                        }
                    }
                }
                catch { }

                if (destPath == null)
                {
                    MessageBox.Show("拡張子または移動先が不正です。", "", MessageBoxButton.OK, MessageBoxImage.Error);
                    textBox_recFilePath.Text = recInfo.RecFilePath;
                }
                else
                {
                    // データベースを変更
                    ErrCode err = ErrCode.CMD_ERR;
                    string originalPath = recInfo.RecFilePath;
                    recInfo.RecFilePath = destPath;
                    try
                    {
                        err = CommonManager.CreateSrvCtrl().SendChgPathRecInfo(new List<RecFileInfo>() { recInfo });
                        if (err != ErrCode.CMD_SUCCESS)
                        {
                            MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "ファイル名の変更に失敗しました。");
                        }
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.ToString());
                    }
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        textBox_recFilePath.Text = recInfo.RecFilePath = originalPath;
                    }
                    else
                    {
                        // ファイルが存在すれば移動する
                        try
                        {
                            File.Move(originalPath, destPath);
                        }
                        catch (FileNotFoundException) { }
                        catch
                        {
                            MessageBox.Show("移動に失敗しました: " + originalPath, "", MessageBoxButton.OK, MessageBoxImage.Warning);
                        }
                        try
                        {
                            // 拡張子が付加されたファイルも移動する
                            foreach (string path in Directory.GetFiles(Path.GetDirectoryName(originalPath), Path.GetFileName(originalPath) + ".*"))
                            {
                                if (path.Length > originalPath.Length &&
                                    string.Compare(path, 0, originalPath, 0, originalPath.Length, StringComparison.OrdinalIgnoreCase) == 0)
                                {
                                    try
                                    {
                                        File.Move(path, destPath + path.Substring(originalPath.Length));
                                    }
                                    catch
                                    {
                                        MessageBox.Show("移動に失敗しました: " + path, "", MessageBoxButton.OK, MessageBoxImage.Warning);
                                    }
                                }
                            }
                        }
                        catch { }
                    }
                }
            }
            button_rename.IsEnabled = false;
        }

        private void button_play_Click(object sender, RoutedEventArgs e)
        {
            if (recInfo != null)
            {
                if (recInfo.RecFilePath.Length > 0)
                {
                    CommonManager.Instance.FilePlay(recInfo.RecFilePath);
                }
            }
        }

        private void button_del_Click(object sender, RoutedEventArgs e)
        {
            if (recInfo != null)
            {
                if (Settings.Instance.ConfirmDelRecInfo)
                {
                    if ((recInfo.RecFilePath.Length > 0 || Settings.Instance.ConfirmDelRecInfoAlways) &&
                        MessageBox.Show("削除してよろしいですか?" +
                                        (recInfo.RecFilePath.Length > 0 ? "\r\n\r\n「録画ファイルも削除する」設定が有効な場合、ファイルも削除されます。" : ""), "確認",
                                        MessageBoxButton.OKCancel, MessageBoxImage.Question, MessageBoxResult.OK) != MessageBoxResult.OK)
                    {
                        return;
                    }
                }
                try
                {
                    CommonManager.CreateSrvCtrl().SendDelRecInfo(new List<uint>() { recInfo.ID });
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.ToString());
                }
            }
            DialogResult = false;
        }
    }
}
