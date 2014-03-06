
#pragma once

#include "../CacheLine.h"
#include "SpinMutex.h"

#include <atomic>

namespace DX
{
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
        volatile char pad_0[CACHE_LINE_SIZE];
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

}