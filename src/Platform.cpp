#include "platform.h"
#include <stdint.h>
#include <malloc.h>
#include <stdio.h>

void IBMutexInit(IBMutex* mutex, const char* name)
{
#ifdef _WIN32
	*mutex = CreateMutex(NULL, false, "Vice connect mutex");
#else
	if (pthread_mutex_init(mutex, NULL) != 0) {
		// error
	}
#endif
}

bool IBMutexDestroy(IBMutex* mutex)
{
#ifdef _WIN32
	HANDLE m = *mutex;
	if (m != IBMutex_Clear) {
		*mutex = IBMutex_Clear;
		return CloseHandle(m);
	}
	return false;
#else
	return pthread_mutex_destroy(mutex) == 0;
#endif
}


int IBMutexLock(IBMutex* mutex)
{
#ifdef _WIN32
	return WaitForSingleObject(
		*mutex,    // handle to mutex
		INFINITE);  // no time-out interval
#else
	return pthread_mutex_lock(mutex);
#endif
}

bool IBMutexRelease(IBMutex* mutex)
{
#ifdef _WIN32
	return ReleaseMutex(*mutex);
#else
	return pthread_mutex_unlock(mutex);
#endif
}

bool IBCreateThread(IBThread* thread, size_t stackSize, IBThreadFunc func, void *param)
{
#ifdef _WIN32
	*thread = CreateThread(nullptr, stackSize, func, param, 0, nullptr);
	return *thread != nullptr;
#else
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

//	int pthread_create(pthread_t * thread, const pthread_attr_t * attr,
//					   void* (*start_routine) (void*), void* arg);
	return pthread_create(thread, &attr, func, param);
#endif
}

bool IBDestroyThread(IBThread* thread)
{
#ifdef _WIN32
	HANDLE t = *thread;
	if (t != INVALID_HANDLE_VALUE) {
		*thread = INVALID_HANDLE_VALUE;
		return CloseHandle(t);
	}
	return false;
#else
	return pthread_cancel(*thread) == 0;
#endif
}

#ifdef _WIN32
HWND GetHWnd();
#endif

void CopyBitmapToClipboard(void* bitmap, int width, int height)
{
#ifdef _WIN32
	HANDLE hData = GlobalAlloc(GHND | GMEM_SHARE, sizeof(BITMAPINFO) + (width * height - 1) * sizeof(uint32_t));
	LPVOID pData = (LPVOID)GlobalLock(hData);
	BITMAPINFO* dib = (BITMAPINFO*)pData;
	dib->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	dib->bmiHeader.biWidth = width;
	dib->bmiHeader.biHeight = height;
	dib->bmiHeader.biPlanes = 1;
	dib->bmiHeader.biBitCount = 32;
	dib->bmiHeader.biCompression = BI_RGB;
	dib->bmiHeader.biSizeImage = width * height * sizeof(uint32_t);
	dib->bmiHeader.biXPelsPerMeter = 1080;
	dib->bmiHeader.biYPelsPerMeter = 1080;

	memcpy(dib->bmiColors, bitmap, (size_t)width * (size_t)height * sizeof(uint32_t));
	GlobalUnlock(hData);
	if (OpenClipboard(GetHWnd())) {
		EmptyClipboard();
		SetClipboardData(CF_DIB, hData);
		CloseClipboard();
	}
#endif
}
