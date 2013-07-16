#include "stdafx.h"

#include "MPInclude.h"

#include "ThreadUtil.h"

typedef struct _MP_LOCK_HANDLE_ITEM{
#ifdef _WIN32
	HANDLE lockEvent;
#else
//Linux系
	pthread_mutex_t	mutex;
#endif
}MP_LOCK_HANDLE_ITEM;

typedef struct _MP_THREAD_HANDLE_ITEM{
#ifdef _WIN32
	HANDLE thread;
#else
//Linux系
	pthread_t thread;
#endif
}MP_THREAD_HANDLE_ITEM;


//排他制御用のハンドルを作成する
//WindowsではCreateEvent
//Linux系はpthread_mutex_init
//戻り値：ハンドル、エラー時NULL
MP_LOCK_HANDLE MP_LOCK_CreateHandle(
	void
)
{
	MP_LOCK_HANDLE_ITEM* handle = NULL;
	handle = (MP_LOCK_HANDLE_ITEM*)malloc(sizeof(MP_LOCK_HANDLE_ITEM));
	if(handle != NULL ){
#ifdef _WIN32
		handle->lockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
#else
		pthread_mutex_init(&handle->mutex,NULL);
#endif
	}

	return (MP_LOCK_HANDLE)handle;
}

//ハンドルの解放
//解放後handleはNULLに
//WindowsではCloseHandle
//Linux系はpthread_mutex_destroy
void MP_LOCK_CloseHandle(
	MP_LOCK_HANDLE* handle				//[IN]MP_LOCK_CreateHandleの戻り値
)
{
	MP_LOCK_HANDLE_ITEM* info = NULL;
	if( handle == NULL ){
		return;
	}
	info = (MP_LOCK_HANDLE_ITEM*)*handle;
	if( info != NULL ){
#ifdef _WIN32
		CloseHandle(info->lockEvent);
#else
		pthread_mutex_destroy(&info->mutex);
#endif
		free(info);
	}
	*handle = NULL;
}

//排他制御用にLockする
//WindowsではWaitForSingleObject
//Linux系はpthread_mutex_lock
//戻り値：0以上で成功、0未満でエラー
int MP_LOCK_Lock(
	MP_LOCK_HANDLE handle				//[IN]MP_LOCK_CreateHandleの戻り値
)
{
	MP_LOCK_HANDLE_ITEM* info = NULL;
	if( handle == NULL ){
		return -1;
	}
	info = (MP_LOCK_HANDLE_ITEM*)handle;
#ifdef _WIN32
	WaitForSingleObject(info->lockEvent, INFINITE);
#else
	pthread_mutex_lock(&info->mutex);
#endif
	return 0;
}

//排他制御用にUnLockする
//WindowsではSetEvent
//Linux系はpthread_mutex_unlock
//戻り値：0以上で成功、0未満でエラー
int MP_LOCK_UnLock(
	MP_LOCK_HANDLE handle				//[IN]MP_LOCK_CreateHandleの戻り値
)
{
	MP_LOCK_HANDLE_ITEM* info = NULL;
	if( handle == NULL ){
		return -1;
	}
	info = (MP_LOCK_HANDLE_ITEM*)handle;
#ifdef _WIN32
	SetEvent(info->lockEvent);
#else
	pthread_mutex_unlock(&info->mutex);
#endif
	return 0;
}

//別スレッドで関数を実行する
//Windowsでは_beginthreadex
//Linux系はpthread_create
//戻り値：ハンドル、エラー時NULL
MP_THREAD_HANDLE MP_THREAD_BegineThread(
	MP_THREAD_FUNC func,				//[IN]新規スレッドの実行を起動するルーチン
	void* param							//[IN]新規スレッドに渡される引数
)
{
	MP_THREAD_HANDLE_ITEM* handle = NULL;
	if( func == NULL ){
		return NULL;
	}

	handle = (MP_THREAD_HANDLE_ITEM*)malloc(sizeof(MP_THREAD_HANDLE_ITEM));
	if( handle != NULL ){
#ifdef _WIN32
		handle->thread = (HANDLE)_beginthreadex(NULL, 0, func, param, CREATE_SUSPENDED, NULL);
		if( handle->thread != 0 ){
			ResumeThread(handle->thread);
		}else{
			free(handle);
			handle = NULL;
		}
#else
		//Linux系
		if( pthread_create(&handle->thread, 0, func, param) != 0 ){
			free(handle);
			handle = NULL;
		}
#endif
	}

	return (MP_THREAD_HANDLE)handle;
}

//スレッドの終了を待ち開放処理をする（タイムアウトはなし）
//キャンセル処理はあらかじめ行っておくこと
//WindowsではWaitForSingleObject
//Linux系はpthread_join
//終了後handleはNULLに
void MP_THREAD_WaitEndThread(
	MP_THREAD_HANDLE* handle			//[IN]MP_THREAD_BegineThreadの戻り値
)
{
	MP_THREAD_HANDLE_ITEM* info = NULL;
	if( handle == NULL ){
		return ;
	}
	info = (MP_THREAD_HANDLE_ITEM*)*handle;
	if( info != NULL ){
#ifdef _WIN32
		WaitForSingleObject(info->thread, INFINITE);
		CloseHandle(info->thread);
#else
		pthread_join(info->thread);
#endif
		free(info);
	}
	*handle = NULL;
}

//Sleepを行う
//WindowsではSleep
//Linux系はnanosleep
void MP_Sleep(
	int msec			//[IN]msec単位
)
{
#ifdef _WIN32
	Sleep(msec);
#else
	struct timespec req;
	req.tv_sec = msec/1000;
	req.tvnsec = (msec%1000)*1000*1000;
	nanosleep(&req, NULL);
#endif
}

