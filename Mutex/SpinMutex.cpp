
#include "SpinMutex.h"

namespace DX
{
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