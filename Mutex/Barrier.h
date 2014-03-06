
#pragma once

#include "../CacheLine.h"

#include <atomic>

namespace DX
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    class Barrier
    {
    public:
        explicit Barrier(size_t numThreads);
        virtual ~Barrier();

        virtual void wait() const = 0;

    protected:
        volatile char pad_0[CACHE_LINE_SIZE];
        mutable std::atomic<size_t> m_count;
        volatile char pad_1[CACHE_LINE_SIZE - (sizeof(std::atomic<size_t>) % CACHE_LINE_SIZE)];

    private:
        Barrier(const Barrier&);
        Barrier(Barrier&&);
    };

}