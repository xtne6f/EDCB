using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public delegate void ViewSettingClickHandler(object sender, object param);

    public enum ErrCode : uint
    {
        CMD_SUCCESS = 1, //成功
        CMD_ERR = 0, //汎用エラー
        CMD_NEXT = 202, //Enumコマンド用、続きあり
        CMD_NON_SUPPORT = 203, //未サポートのコマンド
        CMD_ERR_INVALID_ARG = 204, //引数エラー
        CMD_ERR_CONNECT = 205, //サーバーにコネクトできなかった
        CMD_ERR_DISCONNECT = 206, //サーバーから切断された
        CMD_ERR_TIMEOUT = 207, //タイムアウト発生
        CMD_ERR_BUSY = 208, //ビジー状態で現在処理できない（EPGデータ読み込み中、録画中など）
        CMD_NO_RES = 250 //Post用でレスポンスの必要なし
    };

    public enum UpdateNotifyItem : uint
    {
        No = 0, //なし
        EpgData = 1, //EPGデータ更新
        ReserveInfo = 2, //予約情報更新
        RecInfo = 3, //録画結果更新
        AutoAddEpgInfo = 4, //EPG自動予約登録更新
        AutoAddManualInfo = 5, //プログラム自動予約登録更新
        PlugInFile = 6, //PlugIn系のファイル一覧更新
        IniFile = 51,//iniファイルの更新。
        SrvStatus = 100,
        PreRecStart = 101,
        RecStart = 102,
        RecEnd = 103,
        RecTuijyu = 104,
        ChgTuijyu = 105,
        PreEpgCapStart = 106,
        EpgCapStart = 107,
        EpgCapEnd = 108,
    };

    public enum EpgViewMode : uint
    {
        Unknown = 0, //無効
        BS = 1, //BS
        CS = 2, //CS
        Dttv = 3, //地デジ
        Other = 4, //その他
        Custom = 5, //カスタム
    };
    
    public enum EventInfoTextMode : uint
    {
        All = 0, //基本＋詳細
        BasicOnly = 1, //基本のみ
        TextOnly = 2, //基本＋ジャンルなどを除く番組情報
    };

    //CommonDef.hより
    public enum RecEndStatus : uint
    {
        NORMAL = 1,	        //正常終了
        OPEN_ERR = 2,	    //チューナーのオープンができなかった
        ERR_END = 3,	    //録画中にエラーが発生した
        NEXT_START_END = 4,	//次の予約開始のため終了
        START_ERR = 5,	    //開始時間が過ぎていた
        CHG_TIME = 6,	    //開始時間が変更された
        NO_TUNER = 7,	    //チューナーが足りなかった
        NO_RECMODE = 8,  	//無効扱いだった
        NOT_FIND_PF = 9, 	//p/fに番組情報確認できなかった
        NOT_FIND_6H = 10,	//6時間番組情報確認できなかった
        END_SUBREC = 11,	//サブフォルダへの録画が発生した
        ERR_RECSTART = 12,	//録画開始に失敗した
        NOT_START_HEAD = 13,//一部のみ録画された
        ERR_CH_CHG = 14,	//チャンネル切り替えに失敗した
        ERR_END2 = 15	    //録画中にエラーが発生した(Writeでexception)
    };
    //EpgTimer内部用、集約した簡易ステータス
    public enum RecEndStatusBasic : uint
    {
        DEFAULT = 32,//エラー無し
        WARN = 64,	//軽微なエラー
        ERR = 128   //重度なエラー
    };

    //EpgTimer内部用、予約モード
    public enum ReserveMode : uint
    {
        KeywordAuto = 0,        //キーワード予約
        ManualAuto = 1,	    //プログラム自動予約
        EPG = 2,            //個別予約(EPG)
        Program = 3         //個別予約(プログラム)
    };

    public class CMD_STREAM
    {
        public uint uiParam;
        public uint uiSize;
        public byte[] bData;

        public CMD_STREAM()
        {
            uiParam = 0;
            uiSize = 0;
            bData = null;
        }
    }

}
