
#pragma once

#include "Barrier.h"
#include "SpinRWMutex.h"

namespace DX
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    class CyclicSpinBarrier : public Barrier
    {
    public:
        CyclicSpinBarrier(size_t numThreads = 1);
        ~CyclicSpinBarrier();

        void wait() const;
    private:
        const size_t m_initial;
        mutable SpinRWMutex m_reset;

        CyclicSpinBarrier(const CyclicSpinBarrier&);
        CyclicSpinBarrier(CyclicSpinBarrier&&);
    };

}