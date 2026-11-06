#ifndef INCLUDE_ERR_DEF_H
#define INCLUDE_ERR_DEF_H

//共有ライブラリの戻り値のエラーコード
enum {
	ERR_FALSE = 0, //汎用エラー
	NO_ERR = 1, //成功
	ERR_NOT_INIT = 11, //未初期化
	ERR_INVALID_ARG = 14, //引数が無効
	ERR_NOT_FIND = 15, //情報が見つからなかった
};

#endif
