
#include "CyclicSpinBarrier.h"

namespace DX
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    CyclicSpinBarrier::CyclicSpinBarrier(size_t numThreads) 
        : Barrier(numThreads), m_initial(numThreads)
    {
    }

    CyclicSpinBarrier::~CyclicSpinBarrier()
    {
    }

    void CyclicSpinBarrier::wait() const
    {
        // Calls to wait while the counter is still resetting will block here
        m_reset.lock(false);
        assert(m_count > 0); // wait called too many times
        const size_t count = m_count > 0 ? --m_count : 0;
        {
            if(count == 0)
            {
                /*
                    Relinquish our hold on the reentrant lock, grab one as a writer.
                    This will block until every other thread waiting has finished their wait
                */
                m_reset.unlock(false);
                m_reset.lock(true);
            }
            while(m_count > 0)
            {
                // Spin out
            }
            if(count == 0)
            {
                m_count = m_initial;
                m_reset.unlock(true);
            }
        }
        // The "resetter" has already unlocked their hold on the reentrant lock
        if(count != 0)
            m_reset.unlock(false);
    }

}