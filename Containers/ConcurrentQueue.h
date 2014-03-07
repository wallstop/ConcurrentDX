////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// First-Pass at high performance multi-reader, multi-writer queue
// Author: Eli Pinkerton
// Date: 2/26/14
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../CacheLine.h"
#include "AbstractQueue.h"
#include "../Mutex/SpinYieldMutex.h"

#include <new>

namespace DX
{

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename T>
    class ConcurrentQueue : public Queue<T>
    {
    public:
        ConcurrentQueue();
        // This is slow - you should really avoid this
        ConcurrentQueue(const ConcurrentQueue& copy);
        // This is fast
        ConcurrentQueue(ConcurrentQueue&& move);
        ~ConcurrentQueue();       

        bool    isEmpty() const;
        size_t  size() const;
        bool    front(T& out) const;
        bool    pop(T& out);
        void    push(const T& in);

    private:
        // SpinLocks are already padded on their own cache lines, so we don't need anymore padding
        SpinYieldMutex pushMutex;
        SpinYieldMutex popMutex;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    template <typename T>
    ConcurrentQueue<T>::ConcurrentQueue() : Queue<T>()
    {
        m_start = new Node<T>();
        m_end = m_start;
        assert(m_start != nullptr);
    }

    template <typename T>
    ConcurrentQueue<T>::ConcurrentQueue(const ConcurrentQueue& copy) : Queue<T>()
    {
        m_start = new Node<T>();
        assert(m_start != nullptr);

        SpinLock popLock(copy.popMutex);
        assert(copy.m_start != nullptr);

        Node<T>* currentNode = copy.m_start->next;
        while(currentNode != nullptr)
        {
            assert(currentNode->data != nullptr);
            push(*(currentNode->data));      
            currentNode = currentNode->next;
        }
    }

    template <typename T>
    ConcurrentQueue<T>::ConcurrentQueue(ConcurrentQueue&& move) : Queue()
    {
        SpinLock popLock(move.popMutex);
        SpinLock pushLock(move.pushMutex);

        m_start = new Node<T>();
        m_start->next = move.m_start->next;
        move.m_start->next = nullptr;
        if(move.m_end == move.m_start)
        {
            m_end = m_start;
        }
        else
        {
            move.m_end = move.m_start;
            m_end = move.m_end;
        }
        m_size = move.m_size;
        move.m_size = 0;

        assert(m_start != nullptr);
        assert(m_end != nullptr);
    }

    template <typename T>
    ConcurrentQueue<T>::~ConcurrentQueue()
    {
        // Thread-safe destructor, just in case
        SpinLock popLock(popMutex);
        SpinLock pushLock(pushMutex);

        while(m_start != nullptr)
        {
            Node<T>* currentNode = m_start;
            m_start = currentNode->next.load();
            if(currentNode->data != nullptr)
            {
                delete currentNode->data;
                currentNode->data = nullptr;
            }
            delete currentNode;
            currentNode = nullptr;
        }
    }

    template <typename T>
    bool ConcurrentQueue<T>::isEmpty() const
    {
        return m_size == 0;
    }

    template <typename T>
    size_t ConcurrentQueue<T>::size() const
    {
        return m_size;
    }

    template <typename T>
    bool ConcurrentQueue<T>::front(T& out) const
    {
        assert(m_start);
        if(m_start->next.load() == nullptr)
            return false;
        SpinLock popLock(popMutex);
        if(m_start->next.load()->data == nullptr)
            return false;

        out = *(m_start->next.load()->data);
        return true;
    }

    template <typename T>
    bool ConcurrentQueue<T>::pop(T& out)
    {
        assert(m_start != nullptr);
        // m_start should never be a nullptr on a valid queue

        Node<T>* newStart = nullptr;
        Node<T>* oldStart = nullptr;

        {
            SpinLock popLock(popMutex);

            newStart = m_start->next.load();
            if(newStart == nullptr) // No items left
                return false;

            oldStart = m_start;
            m_start = newStart;

            assert(m_start->data != nullptr);
            out = std::move(*(m_start->data));
            assert(m_size > 0);
            --m_size;
        }

        delete oldStart->data;
        // Only null out what used to be there in DEBUG mode
        #if defined _DEBUG || defined DEBUG
            oldStart->data = nullptr;
        #endif
        delete oldStart;
        #if defined _DEBUG || defined DEBUG
            oldStart = nullptr;
        #endif

        return true;
    }

    template <typename T>
    void ConcurrentQueue<T>::push(const T&  in)
    {
        assert(m_end != nullptr);
        // m_end should never be a nullptr on a valid queue

        Node<T>* temp = new (std::nothrow) Node<T>(new (std::nothrow) T(in));
        assert(temp != nullptr);
        assert(temp->data != nullptr);
        {
	        SpinLock pushLock(pushMutex);
            ++m_size;
            m_end->next = temp;
            m_end = temp;
        }
    }
 
}