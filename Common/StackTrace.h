#ifndef INCLUDE_STACK_TRACE_H
#define INCLUDE_STACK_TRACE_H

#ifndef SUPPRESS_OUTPUT_STACK_TRACE
// 例外によってアプリケーションが終了する直前にスタックトレースを出力する
// デバッグ情報(.pdbファイル)が存在すれば出力はより詳細になる
void SetOutputStackTraceOnUnhandledException(LPCWSTR pathOrNull);
#endif

#endif
