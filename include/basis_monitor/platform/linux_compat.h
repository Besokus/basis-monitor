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

inline int CloseHandle(HANDLE hEvent)
{
    delete hEvent;
    return 1;
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
        const bool ready = hEvent->cond.wait_for(lock, std::chrono::milliseconds(milliseconds), [&]() { return hEvent->signaled; });
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

#endif
