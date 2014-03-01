////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// First-Pass at high performance mutexes.
// Author: Eli Pinkerton
// Date: 2/26/14
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../CacheLine.h"

#include <atomic>

namespace DX
{

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
    class SpinMutex
    {
    public:
        SpinMutex();
        ~SpinMutex();

        /*! \brief Locks the mutex. Further calls to lock will block until there is a call to unlock().
            
            \note lock() is not recursive.
            \note lock() is blocking.
        */
        void lock();

        /*! \brief Attempts to lock the mutex. tryLock() is non-blocking, meaning that regardless of 
            the state of the mutex when tryLock() is called, execution will continue.

            \note tryLock() is not recursive.
            \note tryLock() is non-blocking.
        */
        bool tryLock();
        /*! \brief Unlocks the mutex. This call is non-blocking, because having it otherwise doesn't
            make any sense.
        */
        void unlock(); 

    private:
        // Initial padding so we aren't overlapping some other potentially contended cache
        volatile char pad_[CACHE_LINE_SIZE];
        std::atomic<bool> m_lock;
        // And then flesh out the rest of our pad
        volatile char pad_0[CACHE_LINE_SIZE - (sizeof(std::atomic<bool>) % CACHE_LINE_SIZE)];

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

    void SpinMutex::lock()
    {
        while(m_lock.exchange(true)) 
        {
            // Spin out
        }
    }

    bool SpinMutex::tryLock()
    {
        return m_lock.exchange(true);
    }

    void SpinMutex::unlock()
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
    class SpinLock
    {
    public:
        /*! \param[in] mutex The SpinMutex the lock will lock and guard
        */
        SpinLock(SpinMutex& mutex);
        ~SpinLock();
    private:
        SpinMutex* m_mutex;

        SpinLock(const SpinLock&);
        SpinLock(SpinLock&&);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    SpinLock::SpinLock(SpinMutex& _mutex) : m_mutex(&_mutex)
    {
        m_mutex->lock();
    }

    SpinLock::~SpinLock()
    {
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
        void lock(bool isWriter);    

        /*! \brief Unlocks the mutex as a writer or a reader
        */
        void unlock(bool isWriter);
    
    private:
        // Initial padding so we aren't overlapping some other potentially contended cache
        volatile char pad_[CACHE_LINE_SIZE];
        // Keeps track of the number of readers
        std::atomic<size_t> m_readerLock;
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

    // TODO: CHECK

    void SpinRWMutex::lock(bool isWriter)
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

    void SpinRWMutex::unlock(bool isWriter)
    {
        if(!isWriter)
            --m_readerLock;
        else
            m_writerMutex.unlock();
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
        SpinRWLock(SpinRWMutex& mutex, bool isWriter); 
        ~SpinRWLock();
    private:
        bool isWriter;
        SpinRWMutex* m_lock;

        SpinRWLock(const SpinRWLock&);
        SpinRWLock(SpinRWLock&&);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    SpinRWLock::SpinRWLock(SpinRWMutex& _mutex, bool _writer) 
        : m_lock(&_mutex), isWriter(_writer)
    {
        if(m_lock)
            m_lock->lock(isWriter);
    }

    SpinRWLock::~SpinRWLock()
    {
        if(m_lock)
            m_lock->unlock(isWriter);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    class Barrier
    {
    public:
        explicit Barrier(size_t numThreads);
        virtual ~Barrier();

        virtual void wait() = 0;

    protected:
        volatile char pad_[CACHE_LINE_SIZE];
        std::atomic<size_t> m_count;
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

        void wait();

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

    void SpinBarrier::wait()
    {
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

        void wait();
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

    // TODO: 

    //void CyclicSpinBarrier::wait()
    //{
    //    // Calls to wait while the counter is still resetting will block here
    //    {
    //        SpinRWLock _lock(m_reset, m_count == 0);
    //    }
    //    const size_t numInQueue = m_count > 0 ? m_count-- : 0;
    //    {
    //        SpinRWLock _lock(m_reset, m_count == 0);
    //        while(m_count > 0)
    //        {
    //            // Spin out
    //        }

    //        if(numInQueue == 0)
    //            m_count = m_initial;
    //    }
    //}
}
