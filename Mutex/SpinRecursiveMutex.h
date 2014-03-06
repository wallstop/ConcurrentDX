
#pragma once

#include "../CacheLine.h"
#include "SpinMutex.h"

#include <atomic>

namespace DX
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    class SpinRecursiveMutex : public SpinMutex
    {
    public:
        SpinRecursiveMutex();
        ~SpinRecursiveMutex();

        void lock() const;

        bool tryLock() const;

        void unlock() const;

    private:
        mutable SpinMutex m_assignmentMutex;
        mutable std::atomic<size_t> m_count;
        mutable std::atomic<size_t> m_owner;
        volatile char pad_0[CACHE_LINE_SIZE - ((sizeof(std::atomic<size_t>) + sizeof(std::atomic<bool>)) % CACHE_LINE_SIZE)];

        SpinRecursiveMutex(const SpinRecursiveMutex&);
        SpinRecursiveMutex(SpinRecursiveMutex&&);
    };

}