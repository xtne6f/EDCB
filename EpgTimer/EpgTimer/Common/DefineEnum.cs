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
        Tere = 3, //地デジ
        Other = 4, //その他
        Custom = 5, //カスタム
    };
    
    public enum EventInfoTextMode : uint
    {
        All = 0, //基本＋詳細
        BasicOnly = 1, //基本のみ
        ExtOnly = 2, //詳細のみ
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
