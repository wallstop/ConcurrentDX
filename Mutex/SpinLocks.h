////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// First-Pass at high performance mutexes.
// Author: Eli Pinkerton
// Date: 2/26/14
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../CacheLine.h"

#include <atomic>
#include <thread>

#ifndef DEFAULT_YIELD_TICKS
// Arbitrary for now, best results TBD
#define DEFAULT_YIELD_TICKS 50
#endif

namespace DX
{

    // Currently available classes:
    class Mutex;    // Abstract base
    class Lock;     // Abstract base
    class Barrier;  // Abstract base
    class SpinMutex;
    class SpinRecursiveMutex;
    class SpinYieldMutex;
    class SpinLock;
    class SpinRWMutex;
    class SpinRWLock;
    class SpinBarrier;
    class SpinCyclicBarrier;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    class Mutex
    {
    public:
        Mutex();
        virtual ~Mutex() = 0;

        virtual void lock() const = 0;
        virtual void unlock() const = 0;
    private:
        Mutex(const Mutex&);
        Mutex(Mutex&&);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    Mutex::Mutex()
    {
    }

    Mutex::~Mutex()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    class Lock
    {
    public:
        Lock();
        virtual ~Lock() = 0;
    private:
        Lock(const Lock&);
        Lock(Lock&&);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    Lock::Lock()
    {
    }

    Lock::~Lock()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    /*! \brief SpinMutex is a leightweight mutex class that makes use of C++11 atomics to spin out in
        active-CPU-land as opposed to the traditional method of yielding context. It's target 
        use-case is for code regions that aren't too highly contended, or for contended regions
        that execute fairly fast.

        \note SpinMutex is not a recursive mutex

        Sample of a SpinMutex protecting an int:
        \code
            mutable SpinMutex myMutex;
            int myProtectedValue; 

            void setMyValue(int newValue)
            {
                myMutex.lock();
                myProtectedValue = newValue;
                myMutex.unlock();
            }

            int getMyValue() const
            {
                int ret;
                myMutex.lock();
                ret = myProtectedValue;
                myMutex.unlock();

                return ret;
            }
            \endcode

            Please note that the above example could also be accomplished by making the type of
            myProtectedValue std::atomic<int>, but the usage still stands.
    */
    class SpinMutex : public Mutex
    {
    public:
        SpinMutex();
        virtual ~SpinMutex();

        /*! \brief Locks the mutex. Further calls to lock will block until there is a call to unlock().
            
            \note lock() is not recursive.
            \note lock() is blocking.
        */
        virtual void lock() const;

        /*! \brief Attempts to lock the mutex. tryLock() is non-blocking, meaning that regardless of 
            the state of the mutex when tryLock() is called, execution will continue.

            \note tryLock() is not recursive.
            \note tryLock() is non-blocking.
        */
        virtual bool tryLock() const;
        /*! \brief Unlocks the mutex. This call is non-blocking, because having it otherwise doesn't
            make any sense.
        */
        virtual void unlock() const; 

    protected:
        // Initial padding so we aren't overlapping some other potentially contended cache
        volatile char pad_[CACHE_LINE_SIZE];
        mutable std::atomic<bool> m_lock;
        // And then flesh out the rest of our pad
        volatile char pad_0[CACHE_LINE_SIZE - (sizeof(std::atomic<bool>) % CACHE_LINE_SIZE)];

    private:
        SpinMutex(const SpinMutex&);
        SpinMutex(SpinMutex&&);
    };

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

    /*! \brief SpinLock is a lock-guard style class that latches onto a mutex, locking it upon creation
        and unlocking it upon destruction.

        SpinLocks are the preferred way of interacting with SpinMutex.

        \note SpinLocks do not allow for recursive locking of a SpinMutex

        Here are two examples of using SpinLocks to protect critical sections of code:
        \code

        MyClass myClass;
        SpinMutex myMutex;

        void getMyClass() const
        {
            SpinLock _lock(myMutex);
            return myClass; // After the return, _lock destroys itself, automatically unlocking myMutex
        }
        \endcode

        \code
        MyClass myClass;
        SpinMutex myMutex;

        void doSomeThings(const MyClass& other)
        {
            // The below scope is thread-safe, the rest of the function isn't
            {
                SpinLock _lock(myMutex);
                myClass = other;
            }
            // Continue doing things here - the mutex is unlocked!
        }
        \endcode
    */
    class SpinLock : Lock
    {
    public:
        /*! \param[in] mutex The SpinMutex the lock will lock and guard
        */
        SpinLock(const SpinMutex& mutex);
        ~SpinLock();
    private:
        const SpinMutex* m_mutex;

        SpinLock(const SpinLock&);
        SpinLock(SpinLock&&);
    };

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

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    /*! \brief SpinRWMutex is a leightweight mutex that supports multiple readers (const-only access)
        and a single writer. Supports readers up to the maximum value of size_t for your system. 

        Calls to lock() from readers will block if a writer holds the lock. Calls to lock() from
        writers will block if there are any readers or writers holding onto the lock.

        \note SpinRWMutex is favored towards writers attempting to get the lock

        \note SpinRWMutex is easiest to use with SpinRWLock. SpinRWLock lets you "set-it-and-forget-it" 
        in regards to remembering if you're a reader/writer.

        \note SpinRWMutex does *NOT* check to ensure that calls to lock/unlock are called with the same
        boolean. It does handle these cases (crash-wise), but calling lock(true) and unlock(false) from
        the same thread does not unlock the writer lock.

        \code
        mutable SpinRWMutex myRWMutex;
        MyClass myClass;

        void setMyClass(const MyClass& other)
        {
            myRWMutex.lock(true); // true indicates a writer
            myClass = other;
            myRWMutex.unlock(true); // unlock my writer reference            
        }

        MyClass getMyClass() const
        {
            // static , thread-local object so it isn't constructed every function call, only copied
            static thread_local MyClass ret;
            myRWMutex.lock(false); // false indicates reader
            ret = myClass;
            myRwMutex.unlock(false); // unlock my reader reference
            return ret;
        }
        \endcode
    */
    class SpinRWMutex
    {
    public:
        SpinRWMutex();
        ~SpinRWMutex();
    
        /*! \brief  Locks the mutex as a writer or a reader
            \param[in] isWriter true indicates the caller is attempting to lock this as a writer,
                                false indicates the caller is attempting to lock as a reader.
            \note lock() is not recursive.
            \note lock() is blocking.
        */
        void lock(bool isWriter) const;    

        /*! \brief Unlocks the mutex as a writer or a reader
        */
        void unlock(bool isWriter) const;
    
    private:
        // Initial padding so we aren't overlapping some other potentially contended cache
        volatile char pad_[CACHE_LINE_SIZE];
        // Keeps track of the number of readers
        mutable std::atomic<size_t> m_readerLock;
        // SpinMutex is already padded
        SpinMutex m_lockMutex;
        SpinMutex m_writerMutex;

        SpinRWMutex(const SpinRWMutex&);
        SpinRWMutex(SpinRWMutex&&);
    };

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
    
    /*! \brief SpinRWLock is the preferred way of interacting with a SpinRWMutex. SpinRWLock is a 
        lock-guard style class, locking the SpinRWMutex upon construction and releasing the lock upon
        destruction. SpinRWMutex remembers whether it is a reader or writer lock, and releases the 
        same kind of lock that it was constructed with.

        The big advantage of using SpinRWLocks over SpinRWMutex is the automatic unlock on destruction:
        unlike the examples provided above, no copies have to be made on a get() call

        \note SpinRWLocks do not allow for recursive writer-locks of a single SpinRWMutex

        \code
        MyClass myClass;
        mutable SpinRWMutex myRWMutex;

        MyClass getMyClass() const
        {
            SpinRWLock _lock(myRWMutex, false);
            return myClass; // No copies or thread-local storage necessary
        }

        void setMyClass(const MyClass& other)
        {
            SpinRWLock _lock(myRWMutex, true);
            myClass = other;
        }
        \endcode
    */
    class SpinRWLock
    {
    public:
        /*! \param[in] mutex The SpinRWMutex that the lock should be locking/unlocking
            \param[in] isWriter True indicates a writer lock, False indicates a reader lock
        */
        SpinRWLock(const SpinRWMutex& mutex, bool isWriter); 
        ~SpinRWLock();
    private:
        bool isWriter;
        const SpinRWMutex* m_lock;

        SpinRWLock(const SpinRWLock&);
        SpinRWLock(SpinRWLock&&);
    };

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

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    class SpinRecursiveMutex : public SpinMutex
    {
    public:
        SpinRecursiveMutex();
        ~SpinRecursiveMutex();

        void lock() const;

        bool tryLock() const;

        void unlock() const;

    private:
        mutable SpinMutex m_assignmentMutex;
        mutable std::atomic<size_t> m_count;
        mutable std::atomic<size_t> m_owner;
        volatile char pad_2[CACHE_LINE_SIZE - ((sizeof(std::atomic<size_t>) + sizeof(std::atomic<size_t>) + sizeof(std::atomic<bool>)) % CACHE_LINE_SIZE)];

        SpinRecursiveMutex(const SpinRecursiveMutex&);
        SpinRecursiveMutex(SpinRecursiveMutex&&);
    };

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

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    class SpinYieldMutex : public SpinMutex
    {
    public:
        SpinYieldMutex(const size_t maxYieldTicks = DEFAULT_YIELD_TICKS);
        ~SpinYieldMutex();

        void lock() const;
        bool tryLock() const;
        void unlock() const;
    private:
        const   size_t m_maxYieldTicks;

        SpinYieldMutex(const SpinYieldMutex&);
        SpinYieldMutex(SpinYieldMutex&&);
    };

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

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    // TODO: make these yielding

    class Barrier
    {
    public:
        explicit Barrier(size_t numThreads);
        virtual ~Barrier();

        virtual void wait() const = 0;

    protected:
        volatile char pad_[CACHE_LINE_SIZE];
        mutable std::atomic<size_t> m_count;
        volatile char pad_0[CACHE_LINE_SIZE - (sizeof(std::atomic<size_t>) % CACHE_LINE_SIZE)];

    private:
        Barrier(const Barrier&);
        Barrier(Barrier&&);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    Barrier::Barrier(size_t numThreads) : m_count(numThreads)
    {
    }

    Barrier::~Barrier()
    {
    }
    
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
        SpinRWMutex m_reset;

        CyclicSpinBarrier(const CyclicSpinBarrier&);
        CyclicSpinBarrier(CyclicSpinBarrier&&);
    };

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
