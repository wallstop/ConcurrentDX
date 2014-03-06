
#include "SpinBarrier.h"

namespace DX
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    SpinBarrier::SpinBarrier(size_t numThreads) : Barrier(numThreads)
    {
    }

    SpinBarrier::~SpinBarrier()
    {
    }

    void SpinBarrier::wait() const
    {
        assert(m_count > 0); // wait called too many times
        if(m_count > 0)
            --m_count;
        while(m_count > 0)
        {
            // Spin out
        }
    }

}