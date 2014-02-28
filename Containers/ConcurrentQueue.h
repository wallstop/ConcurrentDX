////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// First-Pass at high performance multi-reader, multi-writer queue
// Author: Eli Pinkerton
// Date: 2/26/14
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../CacheLine.h"
#include "../Mutex/SpinLocks.h"
#include "../Mutex/Locks.h"

namespace DX
{

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename T>
    class ConcurrentQueue
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
        bool    pop(T& in);
        void    push(const T&);

    private: 

        struct Node
        {
            Node() : next(nullptr), data(nullptr){};
            Node(T* _data) :data(_data), next(nullptr){};

            T* data;
            std::atomic<Node*> next;
            char pad_[CACHE_LINE_SIZE - (CACHE_LINE_SIZE % (sizeof(T*) + sizeof(std::atomic<Node*>)))];
        };

        Node* m_start;
        char pad_0[CACHE_LINE_SIZE - (CACHE_LINE_SIZE % sizeof(Node*))];
        Node* m_end;
        char pad_1[CACHE_LINE_SIZE - (CACHE_LINE_SIZE % sizeof(Node*))];    
        // SpinLocks are already padded on their own cache lines, so we don't need anymore padding
        SpinMutex pushMutex;
        SpinMutex popMutex;
        std::atomic<size_t> m_size;
        char pad_2[CACHE_LINE_SIZE - (CACHE_LINE_SIZE % sizeof(Node*))];
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    template <typename T>
    ConcurrentQueue<T>::ConcurrentQueue() : m_start(nullptr), m_end(nullptr), m_size(0)
    {
        m_start = new Node();
        m_end = m_start;
    }

    template <typename T>
    ConcurrentQueue<T>::ConcurrentQueue(const ConcurrentQueue& copy) 
        : m_start(nullptr), m_end(nullptr), m_size(0)
    {    
        m_start = new Node();
        m_end = m_start;

        SpinLock popLock(copy.popMutex);
        if(!copy.m_start)
            return;

        Node* currentNode = copy.m_start; 
        while(currentNode->next)
        {
            currentNode = currentNode->next;
            push(*(currentNode->data));
            ++m_size;
        }

        m_end = currentNode;
    }

    template <typename T>
    ConcurrentQueue<T>::ConcurrentQueue(ConcurrentQueue&& move) 
        : m_start(nullptr), m_end(nullptr), m_size(0)
    {
        SpinLock popLock(move.popMutex);
        SpinLock pushLock(move.pushMutex);       

        m_start = move.m_start;
        move.m_start = nullptr;
        m_end = move.m_end;
        move.m_end = nullptr;
        m_size = move.m_size;
        move.m_size = 0;
    }

    template <typename T>
    ConcurrentQueue<T>::~ConcurrentQueue()
    {
        // Thread-safe destructor, just in case
        SpinLock popLock(popMutex);
        SpinLock pushLock(pushMutex);

        while(m_start)
        {
            Node* currentNode = m_start;
            m_start = currentNode->next;
            delete currentNode->data;
            delete currentNode;
        }
    }

    template <typename T>
    bool ConcurrentQueue<T>::isEmpty() const
    {
        return m_start && !m_start->next.load();
    }

    template <typename T>
    size_t ConcurrentQueue<T>::size() const
    {
        return m_size;
    }

    template <typename T>
    bool ConcurrentQueue<T>::pop(T& in)
    {
        if(!m_start)
            return false;

        // pop() can't be using a spin mutex...
        SpinLock _lock(popMutex);

        Node* newStart = m_start->next;
        if(!newStart) // No items left!
            return false;

        Node* oldStart = m_start;

        m_start = newStart;

        const bool failed = !m_start->data;
        if(!failed)
            in = std::move(*(m_start->data));

        if(oldStart->data)
            delete oldStart->data;
        delete oldStart;

        --m_size;

        return !failed;
    }

    template <typename T>
    void ConcurrentQueue<T>::push(const T& object)
    {
        // push should never assign nullptr to m_end, so this check is thread-"ok"
        if(!m_end)
            return;

        Node* temp = new Node(new T(object));
        SpinLock _lock(pushMutex);
        m_end->next = temp;
        m_end = temp;

        ++m_size;
    }

}
