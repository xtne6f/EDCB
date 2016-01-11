using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public partial class RecFileInfo : ICtrlCmdReadWrite
    {
        //ファイル数が多くなる場合もあるようなので、拡張メソッドにせず、中へ置く。
        private long dropsCritical = -1;
        public long DropsCritical
        {
            get
            {
                if (dropsCritical == -1) CheckCriticalDrops();
                return dropsCritical;
            }
        }
        private long scramblesCritical = -1;
        public long ScramblesCritical
        {
            get
            {
                if (scramblesCritical == -1) CheckCriticalDrops();
                return scramblesCritical;
            }
        }
        private void CheckCriticalDrops()
        {
            if (string.IsNullOrEmpty(this.ErrInfo) == false)
            {
                try
                {
                    dropsCritical = 0;
                    scramblesCritical = 0;
                    var newInfo = new StringBuilder("");

                    string[] lines = this.ErrInfo.Split(new char[] { '\n' });
                    foreach (string line1 in lines)
                    {
                        string line_new = line1;
                        if (line1.StartsWith("PID:") == true)
                        {
                            string[] words = line1.Split(new char[] { ' ', ':' }, StringSplitOptions.RemoveEmptyEntries);
                            //デフォルト { "EIT", "NIT", "CAT", "SDT", "SDTT", "TOT", "ECM", "EMM" }
                            if (Settings.Instance.RecInfoDropExclude.FirstOrDefault(s => words[8].Contains(s)) == null)
                            {
                                dropsCritical += (Int64)Convert.ToUInt64(words[5]);
                                scramblesCritical += (Int64)Convert.ToUInt64(words[7]);
                                line_new = line1.Replace(" " + words[8], "*" + words[8]);
                            }
                        }
                        newInfo.Append(line_new.TrimEnd('\r') + "\r\n");//単に\n付けるだけでも良いが、一応"\r\n"に確定させる
                    }

                    newInfo.AppendFormat("                              * = Critical Drop/Scramble Parameter.\r\n");
                    newInfo.AppendFormat("                              Drop:{0,9}  Scramble:{1,10}  Total\r\n", this.Drops, this.Scrambles);
                    newInfo.AppendFormat("                              Drop:{0,9}  Scramble:{1,10} *Critical\r\n", this.dropsCritical, this.scramblesCritical);
                    this.ErrInfo = newInfo.ToString();

                    return;
                }
                catch { }//エラーがあったときは、ラストへ
            }

            dropsCritical = this.Drops;
            scramblesCritical = this.Scrambles;
        }

        //簡易ステータス
        public RecEndStatusBasic RecStatusBasic
        {
            get
            {
                switch ((RecEndStatus)RecStatus)
                {
                    case RecEndStatus.NORMAL:           //正常終了
                        return RecEndStatusBasic.DEFAULT;
                    case RecEndStatus.OPEN_ERR:         //チューナーのオープンができなかった
                        return RecEndStatusBasic.ERR;
                    case RecEndStatus.ERR_END:          //録画中にエラーが発生した
                        return RecEndStatusBasic.ERR;
                    case RecEndStatus.NEXT_START_END:   //次の予約開始のため終了
                        return RecEndStatusBasic.ERR;
                    case RecEndStatus.START_ERR:        //開始時間が過ぎていた
                        return RecEndStatusBasic.ERR;
                    case RecEndStatus.CHG_TIME:         //開始時間が変更された
                        return RecEndStatusBasic.DEFAULT;
                    case RecEndStatus.NO_TUNER:         //チューナーが足りなかった
                        return RecEndStatusBasic.ERR;
                    case RecEndStatus.NO_RECMODE:       //無効扱いだった
                        return RecEndStatusBasic.DEFAULT;
                    case RecEndStatus.NOT_FIND_PF:      //p/fに番組情報確認できなかった
                        return RecEndStatusBasic.WARN;
                    case RecEndStatus.NOT_FIND_6H:      //6時間番組情報確認できなかった
                        return RecEndStatusBasic.WARN;
                    case RecEndStatus.END_SUBREC:       //サブフォルダへの録画が発生した
                        return RecEndStatusBasic.WARN;
                    case RecEndStatus.ERR_RECSTART:     //録画開始に失敗した
                        return RecEndStatusBasic.ERR;
                    case RecEndStatus.NOT_START_HEAD:   //一部のみ録画された
                        return RecEndStatusBasic.ERR;
                    case RecEndStatus.ERR_CH_CHG:       //チャンネル切り替えに失敗した
                        return RecEndStatusBasic.ERR;
                    case RecEndStatus.ERR_END2:         //録画中にエラーが発生した(Writeでexception)
                        return RecEndStatusBasic.ERR;
                    default:                            //状況不明
                        return RecEndStatusBasic.ERR;
                }
            }
        }

    }

    public static class RecFileInfoEx
    {
        public static List<RecFileInfo> GetNoProtectedList(this ICollection<RecFileInfo> itemlist)
        {
            return itemlist.Where(item => item == null ? false : item.ProtectFlag == 0).ToList();
        }
        //public static bool HasProtected(this List<RecInfoItem> list)
        //{
        //    return list.Any(info => info == null ? false : info.RecInfo.ProtectFlag == true);
        //}
        public static bool HasNoProtected(this List<RecFileInfo> list)
        {
            return list.Any(info => info == null ? false : info.ProtectFlag == 0);
        }

        public static UInt64 Create64Key(this RecFileInfo obj)
        {
            return CommonManager.Create64Key(obj.OriginalNetworkID, obj.TransportStreamID, obj.ServiceID);
        }
        public static UInt64 Create64PgKey(this RecFileInfo obj)
        {
            return CommonManager.Create64PgKey(obj.OriginalNetworkID, obj.TransportStreamID, obj.ServiceID, obj.EventID);
        }

        public static List<RecFileInfo> Clone(this List<RecFileInfo> src) { return CopyObj.Clone(src, CopyData); }
        public static RecFileInfo Clone(this RecFileInfo src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this RecFileInfo src, RecFileInfo dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(RecFileInfo src, RecFileInfo dest)
        {
            dest.Comment = src.Comment;
            dest.Drops = src.Drops;
            dest.DurationSecond = src.DurationSecond;
            dest.ErrInfo = src.ErrInfo;
            dest.EventID = src.EventID;
            dest.ID = src.ID;
            dest.OriginalNetworkID = src.OriginalNetworkID;
            dest.ProgramInfo = src.ProgramInfo;
            dest.ProtectFlag = src.ProtectFlag;
            dest.RecFilePath = src.RecFilePath;
            dest.RecStatus = src.RecStatus;
            dest.Scrambles = src.Scrambles;
            dest.ServiceID = src.ServiceID;
            dest.ServiceName = src.ServiceName;
            dest.StartTime = src.StartTime;
            dest.StartTimeEpg = src.StartTimeEpg;
            dest.Title = src.Title;
            dest.TransportStreamID = src.TransportStreamID;
        }

    }
}
