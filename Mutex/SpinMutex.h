
#pragma once

#include "../CacheLine.h"
#include "Mutex.h"

#include <atomic>

namespace DX
{
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
        volatile char pad_0[CACHE_LINE_SIZE];
        mutable std::atomic<bool> m_lock;
        // And then flesh out the rest of our pad
        volatile char pad_1[CACHE_LINE_SIZE - (sizeof(std::atomic<bool>) % CACHE_LINE_SIZE)];

    private:
        SpinMutex(const SpinMutex&);
        SpinMutex(SpinMutex&&);
    };

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
    class SpinLock //: Lock
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

}