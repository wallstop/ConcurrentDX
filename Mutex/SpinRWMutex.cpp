
#include "SpinRWMutex.h"

namespace DX
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    SpinRWMutex::SpinRWMutex() : m_readerLock(0)
    {
    }

    SpinRWMutex::~SpinRWMutex()
    {
    }

    void SpinRWMutex::lock(bool isWriter) const
    {
        SpinLock _lock(m_lockMutex);

        // Once a writer attempts to access, no more readers will be able to read
        if(isWriter)
        {
            // TODO: SpinBarrierLock
            m_writerMutex.lock();
            while(m_readerLock > 0) 
            {
                // Spin out waiting for readers to finish
            }
        }
        else
        {
            SpinLock _readLock(m_writerMutex);
            ++m_readerLock;
        }
    }

    void SpinRWMutex::unlock(bool isWriter) const
    {
        if(!isWriter)
        {
            assert(m_readerLock > 0); // unlock called too many times
            if(m_readerLock > 0)
                --m_readerLock;
        }
        else
        {
            m_writerMutex.unlock();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    SpinRWLock::SpinRWLock(const SpinRWMutex& _mutex, bool _writer) 
        : m_lock(&_mutex), isWriter(_writer)
    {
        assert(m_lock); // We should have a handle on a valid mutex
        if(m_lock)
            m_lock->lock(isWriter);
    }

    SpinRWLock::~SpinRWLock()
    {
        assert(m_lock); // We should have a handle on a valid mutex
        if(m_lock)
            m_lock->unlock(isWriter);
    }

}