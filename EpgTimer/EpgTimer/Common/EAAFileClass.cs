using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    class EAAFileClass
    {
        public EpgAutoAddData AddKey = null;
        public bool LoadEAAFile(String filePath)
        {
            bool ret = false;
            System.IO.StreamReader reader = null;
            try
            {
                reader = (new System.IO.StreamReader(filePath, System.Text.Encoding.Default));
                while (reader.Peek() >= 0)
                {
                    string buff = reader.ReadLine();
                    if (buff.IndexOf(";") == 0)
                    {
                        //コメント行
                    }
                    else
                    {
                        AddKey = new EpgAutoAddData();
                        EpgSearchKeyInfo searchKey = new EpgSearchKeyInfo();
                        RecSettingData recSetKey = new RecSettingData();

                        string[] list = buff.Split('\t');
                        if (list.Length == 0)
                        {
                            continue;
                        }
                        try
                        {
                            searchKey.andKey = list[0];
                            searchKey.notKey = list[1];
                            searchKey.regExpFlag = Convert.ToByte(list[2]);
                            searchKey.titleOnlyFlag = Convert.ToByte(list[3]);
                            //対象ジャンル
                            searchKey.contentList = new List<EpgContentData>();
                            string[] valBuff;
                            if (list[4].Length > 0)
                            {
                                valBuff = list[4].Split(',');
                                foreach (string val in valBuff)
                                {
                                    UInt32 content = Convert.ToUInt32(val);
                                    EpgContentData addItem = new EpgContentData();
                                    addItem.content_nibble_level_1 = (byte)((content & 0xFF000000) >> 24);
                                    addItem.content_nibble_level_2 = (byte)((content & 0x00FF0000) >> 16);
                                    addItem.user_nibble_1 = (byte)((content & 0x0000FF00) >> 8);
                                    addItem.user_nibble_2 = (byte)((content & 0x000000FF));

                                    searchKey.contentList.Add(addItem);
                                }
                            }
                            //対象期間
                            searchKey.dateList = new List<EpgSearchDateInfo>();
                            if (list[5].Length > 0)
                            {
                                valBuff = list[5].Split(',');
                                foreach (string val in valBuff)
                                {
                                    string[] dateBuff = val.Split('-');
                                    EpgSearchDateInfo addItem = new EpgSearchDateInfo();
                                    addItem.startDayOfWeek = Convert.ToByte(dateBuff[0]);
                                    UInt32 time = Convert.ToUInt32(dateBuff[1]);
                                    addItem.startHour = (UInt16)((time & 0xFFFF0000) >> 16);
                                    addItem.startMin = (UInt16)((time & 0x0000FFFF));
                                    addItem.endDayOfWeek = Convert.ToByte(dateBuff[2]);
                                    time = Convert.ToUInt32(dateBuff[3]);
                                    addItem.endHour = (UInt16)((time & 0xFFFF0000) >> 16);
                                    addItem.endMin = (UInt16)((time & 0x0000FFFF));

                                    searchKey.dateList.Add(addItem);
                                }
                            }
                            //対象サービス
                            searchKey.serviceList = new List<long>();
                            if (list[6].Length > 0)
                            {
                                valBuff = list[6].Split(',');
                                foreach (string val in valBuff)
                                {
                                    Int64 addItem = 0;
                                    addItem = Convert.ToInt64(val, 16);

                                    searchKey.serviceList.Add(addItem);
                                }
                            }
                            searchKey.aimaiFlag = Convert.ToByte(list[7]);


                            Settings.GetDefRecSetting(0, ref recSetKey);

                            AddKey.searchInfo = searchKey;
                            AddKey.recSetting = recSetKey;

                            ret = true;
                        }
                        catch
                        {
                        }
                    }
                }
            }
            catch
            {
                AddKey = null;
            }
            if( reader != null ){
                reader.Close();
            }
            return ret;
        }
    }
}
