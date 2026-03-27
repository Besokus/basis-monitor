#pragma once

#ifndef _WIN32

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <sys/syscall.h>
#include <sys/types.h>
#include <termios.h>
#include <ctime>
#include <unistd.h>

#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif

#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0 0
#endif

#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT 258
#endif

using DWORD = unsigned long;

struct SYSTEMTIME
{
	unsigned short wYear;
	unsigned short wMonth;
	unsigned short wDayOfWeek;
	unsigned short wDay;
	unsigned short wHour;
	unsigned short wMinute;
	unsigned short wSecond;
	unsigned short wMilliseconds;
};

inline void GetLocalTime(SYSTEMTIME* st)
{
	if (st == nullptr)
	{
		return;
	}
	auto now = std::chrono::system_clock::now();
	std::time_t tt = std::chrono::system_clock::to_time_t(now);
	std::tm tmLocal = {};
	localtime_r(&tt, &tmLocal);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
	st->wYear = static_cast<unsigned short>(tmLocal.tm_year + 1900);
	st->wMonth = static_cast<unsigned short>(tmLocal.tm_mon + 1);
	st->wDayOfWeek = static_cast<unsigned short>(tmLocal.tm_wday);
	st->wDay = static_cast<unsigned short>(tmLocal.tm_mday);
	st->wHour = static_cast<unsigned short>(tmLocal.tm_hour);
	st->wMinute = static_cast<unsigned short>(tmLocal.tm_min);
	st->wSecond = static_cast<unsigned short>(tmLocal.tm_sec);
	st->wMilliseconds = static_cast<unsigned short>(ms.count());
}

inline DWORD GetCurrentThreadId()
{
#ifdef SYS_gettid
	return static_cast<DWORD>(::syscall(SYS_gettid));
#else
	return static_cast<DWORD>(::getpid());
#endif
}

struct LinuxEventHandle
{
	std::mutex mutex;
	std::condition_variable cond;
	bool manualReset;
	bool signaled;
};

using HANDLE = LinuxEventHandle*;

inline HANDLE CreateEvent(void*, bool manualReset, bool initialState, const char*)
{
	LinuxEventHandle* handle = new LinuxEventHandle();
	handle->manualReset = manualReset;
	handle->signaled = initialState;
	return handle;
}

inline int SetEvent(HANDLE hEvent)
{
	if (hEvent == nullptr)
	{
		return 0;
	}
	{
		std::lock_guard<std::mutex> lock(hEvent->mutex);
		hEvent->signaled = true;
	}
	if (hEvent->manualReset)
	{
		hEvent->cond.notify_all();
	}
	else
	{
		hEvent->cond.notify_one();
	}
	return 1;
}

inline int ResetEvent(HANDLE hEvent)
{
	if (hEvent == nullptr)
	{
		return 0;
	}
	{
		std::lock_guard<std::mutex> lock(hEvent->mutex);
		hEvent->signaled = false;
	}
	return 1;
}

inline unsigned long WaitForSingleObject(HANDLE hEvent, unsigned long milliseconds)
{
	if (hEvent == nullptr)
	{
		return WAIT_TIMEOUT;
	}
	std::unique_lock<std::mutex> lock(hEvent->mutex);
	if (milliseconds == INFINITE)
	{
		hEvent->cond.wait(lock, [&]() { return hEvent->signaled; });
	}
	else
	{
		bool ready = hEvent->cond.wait_for(lock, std::chrono::milliseconds(milliseconds), [&]() { return hEvent->signaled; });
		if (!ready)
		{
			return WAIT_TIMEOUT;
		}
	}
	if (!hEvent->manualReset)
	{
		hEvent->signaled = false;
	}
	return WAIT_OBJECT_0;
}

inline int _getch()
{
	termios oldTerm = {};
	tcgetattr(STDIN_FILENO, &oldTerm);
	termios newTerm = oldTerm;
	newTerm.c_lflag &= static_cast<unsigned int>(~(ICANON | ECHO));
	tcsetattr(STDIN_FILENO, TCSANOW, &newTerm);
	int ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm);
	return ch;
}

template <size_t N>
inline int strcpy_s(char (&dest)[N], const char* src)
{
	if (src == nullptr || N == 0)
	{
		return 1;
	}
	std::strncpy(dest, src, N - 1);
	dest[N - 1] = '\0';
	return 0;
}

inline int strcpy_s(char* dest, size_t destSize, const char* src)
{
	if (dest == nullptr || src == nullptr || destSize == 0)
	{
		return 1;
	}
	std::strncpy(dest, src, destSize - 1);
	dest[destSize - 1] = '\0';
	return 0;
}

inline char* itoa(int value, char* str, int base)
{
	if (str == nullptr || base < 2 || base > 36)
	{
		return nullptr;
	}
	if (base == 10)
	{
		std::snprintf(str, 64, "%d", value);
		return str;
	}
	const char* digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	unsigned int u = static_cast<unsigned int>(value);
	bool negative = (value < 0 && base == 10);
	if (value < 0 && base != 10)
	{
		u = static_cast<unsigned int>(-value);
	}
	char buf[70] = { 0 };
	int i = 0;
	do
	{
		buf[i++] = digits[u % static_cast<unsigned int>(base)];
		u /= static_cast<unsigned int>(base);
	} while (u > 0);
	if (negative)
	{
		buf[i++] = '-';
	}
	int j = 0;
	while (i > 0)
	{
		str[j++] = buf[--i];
	}
	str[j] = '\0';
	return str;
}

#endif
