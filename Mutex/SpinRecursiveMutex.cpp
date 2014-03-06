
#include "SpinRecursiveMutex.h"

#include <thread>

namespace DX
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////   
    // impl

    SpinRecursiveMutex::SpinRecursiveMutex() : SpinMutex(), m_owner(0), m_count(0)
    {
    }

    SpinRecursiveMutex::~SpinRecursiveMutex()
    {
    }

    void SpinRecursiveMutex::lock() const
    {
        const size_t queryingThread = std::this_thread::get_id().hash();
        #if defined _DEBUG || defined DEBUG 
            const bool wasLocked = m_lock.load();
            assert(wasLocked ? queryingThread == m_owner : m_count == 0);
        #endif
        while(m_lock.exchange(true) && queryingThread != m_owner)
        {
            // Spin out
        }

        // Here we have exclusive ownership
        SpinLock _lock(m_assignmentMutex);
        m_owner = queryingThread;
        ++m_count;
    }

    bool SpinRecursiveMutex::tryLock() const
    {
        const size_t queryingThread = std::this_thread::get_id().hash();

        const bool wasLocked = m_lock.exchange(true);
        const size_t owner = m_owner;
        if(!wasLocked || (wasLocked && queryingThread == owner))
        {
            SpinLock _lock(m_assignmentMutex);
            assert(wasLocked ? queryingThread == m_owner : m_count == 0);
            m_owner = queryingThread;
            ++m_count;
            return true;
        }
        return false;        
    }

    void SpinRecursiveMutex::unlock() const
    {
        #if defined(_DEBUG) || defined(DEBUG)
            const size_t queryingThread = std::this_thread::get_id().hash();
            assert(m_owner == queryingThread);
        #endif

        assert(m_count > 0); // Make sure unlock() isn't called more than lock()
        if(--m_count == 0)
        {
            m_lock = false;
            m_owner = 0;
        }
    }

}