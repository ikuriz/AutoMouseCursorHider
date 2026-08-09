#include "instance_lock.h"

namespace
{
constexpr wchar_t kMutexName[] = L"Local\\AutoMouseCursorHider.Native.Instance.v2";
}

InstanceLock::~InstanceLock()
{
    if (_mutex != nullptr)
    {
        ReleaseMutex(_mutex);
        CloseHandle(_mutex);
    }
}

bool InstanceLock::Acquire()
{
    if (_mutex != nullptr)
    {
        return true;
    }

    _mutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (_mutex == nullptr)
    {
        return false;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(_mutex);
        _mutex = nullptr;
        return false;
    }

    return true;
}

