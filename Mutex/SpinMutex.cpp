
#include "SpinMutex.h"

namespace DX
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    SpinMutex::SpinMutex() : m_lock(false)
    {
    }

    SpinMutex::~SpinMutex()
    {
    }

    void SpinMutex::lock() const
    {
        while(m_lock.exchange(true))
        {
            // Spin out
        }
    }

    bool SpinMutex::tryLock() const
    {
        return !m_lock.exchange(true);
    }

    void SpinMutex::unlock() const
    {
        m_lock = false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    SpinLock::SpinLock(const SpinMutex& _mutex) : m_mutex(&_mutex)
    {
        assert(m_mutex); // We should have a handle on a valid mutex
        if(m_mutex)
            m_mutex->lock();
    }

    SpinLock::~SpinLock()
    {
        assert(m_mutex); // We should have a handle on a valid mutex
        if(m_mutex)
            m_mutex->unlock();
    }

}