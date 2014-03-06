
#pragma once

#include "Barrier.h"

namespace DX
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    class SpinBarrier : public Barrier
    {
    public:
        SpinBarrier(size_t numThreads = 1);
        ~SpinBarrier();

        void wait() const;

    private:
        SpinBarrier(const SpinBarrier&);
        SpinBarrier(SpinBarrier&&);
    };

}