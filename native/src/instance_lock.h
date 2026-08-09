#pragma once

#include <windows.h>

class InstanceLock
{
public:
    InstanceLock() = default;
    InstanceLock(const InstanceLock&) = delete;
    InstanceLock& operator=(const InstanceLock&) = delete;
    ~InstanceLock();

    bool Acquire();

private:
    HANDLE _mutex = nullptr;
};

