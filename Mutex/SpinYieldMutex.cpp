
#include "SpinYieldMutex.h"

#include <thread>

namespace DX
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    SpinYieldMutex::SpinYieldMutex(const size_t maxYieldTicks) : SpinMutex(), m_maxYieldTicks(maxYieldTicks)
    {
    }

    SpinYieldMutex::~SpinYieldMutex()
    {
    }

    void SpinYieldMutex::lock() const
    {
        size_t numTries = 0;
        while(m_lock.exchange(true))
        {
            if(++numTries >= m_maxYieldTicks)   // >= just for sanity
            {
                numTries = 0;
                std::this_thread::yield();
            }
        }
    }

    bool SpinYieldMutex::tryLock() const
    {
        return !m_lock.exchange(true);
    }

    void SpinYieldMutex::unlock() const
    {
        m_lock = false;
    }

}