#pragma once
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#else
#include "pthread.h"
#endif

#ifdef _DEBUG

#define IBL_LOGF( Format, ...) DebugLog( Format, __VA_ARGS__ )
#define IBL_ASSUME(cond) if(!(cond)) { IBL_LOGF("Condition \"" #cond "\" failed\n"); }
#define IBL_LOGHEX(val) if(!(val)) { IBL_LOGF("Condition failed \"" #val "\" = 0x%x\n", val); }
#define IBL_ASSUMEF(cond, ...) if(!(cond)) { IBL_LOGF(...); }

#else

#define IBL_LOGF( Format, ...)
#define IBL_ASSUME(cond)
#define IBL_LOGHEX(val)
#define IBL_ASSUMEF(cond, ...)

#endif

#ifdef _WIN32

#define IBMutex_Clear INVALID_HANDLE_VALUE
#define IBThread_Clear INVALID_HANDLE_VALUE
#define NULL_HANDLE INVALID_HANDLE_VALUE
typedef DWORD IBThreadRet;
typedef HANDLE IBMutex;
typedef HANDLE IBThread;
typedef IBThreadRet(WINAPI* IBThreadFunc)(void* data);

#elif defined(__linux__) || defined(__APPLE__)

#define NULL_HANDLE 0
#define IBMutex_Clear 0
#define IBThread_Clear 0
typedef void* IBThreadRet;
typedef pthread_mutex_t IBMutex;
typedef pthread_t IBThread;
typedef IBThreadRet(*IBThreadFunc)(void* data);

#else

#endif

void IBMutexInit(IBMutex* mutex, const char* name);
bool IBMutexDestroy(IBMutex* mutex);
int IBMutexLock(IBMutex* mutex);
bool IBMutexRelease(IBMutex* mutex);
bool IBCreateThread(IBThread* thread, size_t stackSize, IBThreadFunc func, void* param);
bool IBDestroyThread(IBThread* thread);
void CopyBitmapToClipboard(void* bitmap, int width, int height);

#ifdef _DEBUG
int DebugLog(char const* const _Format, ...);
#endif


