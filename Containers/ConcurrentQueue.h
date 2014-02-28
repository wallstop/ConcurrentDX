////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// First-Pass at high performance multi-reader, multi-writer queue
// Author: Eli Pinkerton
// Date: 2/26/14
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../CacheLine.h"
#include "../Mutex/SpinLocks.h"

#include <iostream>

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

        bool isEmpty() const;
        size_t size() const;
        bool pop(T& in);
        void push(const T&);

    private:

        struct Node
        {
            Node() : next(nullptr), data(nullptr){};
            Node(T* _data) :data(_data), next(nullptr){};

            T* data;
            std::atomic<Node*> next;
            volatile char pad_[CACHE_LINE_SIZE - ((sizeof(T*) + sizeof(std::atomic<Node*>)) % CACHE_LINE_SIZE)];
        };

        Node* m_start;
        volatile char pad_0[CACHE_LINE_SIZE - (sizeof(Node*) % CACHE_LINE_SIZE)];
        Node* m_end;
        volatile char pad_1[CACHE_LINE_SIZE - (sizeof(Node*) % CACHE_LINE_SIZE)];
        // SpinLocks are already padded on their own cache lines, so we don't need anymore padding
        SpinMutex pushMutex;
        SpinMutex popMutex;
        std::atomic<size_t> m_size;
        volatile char pad_2[CACHE_LINE_SIZE - (sizeof(Node*) % CACHE_LINE_SIZE)];
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    template <typename T>
    ConcurrentQueue<T>::ConcurrentQueue() : m_start(nullptr), m_end(nullptr), m_size(0)
    {
        m_start = new Node();
        m_end = m_start;
        assert(m_start != nullptr);
    }

    template <typename T>
    ConcurrentQueue<T>::ConcurrentQueue(const ConcurrentQueue& copy)
        : m_start(nullptr), m_end(nullptr), m_size(0)
    {
        m_start = new Node();
        m_end = m_start;
        assert(m_start != nullptr);

        SpinLock popLock(copy.popMutex);
        if(copy.m_start == nullptr)
            return;

        Node* currentNode = copy.m_start;
        while(currentNode->next != nullptr)
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
            Node* currentNode = m_start;
            m_start = currentNode->next.load();
            if(currentNode->data != nullptr)
            {
                delete currentNode->data;
                currentNode->data = nullptr;
            }

            assert(currentNode != nullptr);
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
    bool ConcurrentQueue<T>::pop(T& in)
    {
        SpinLock popLock(popMutex);
        // Can we scope this tighter?
        assert(m_start != nullptr);
        if(m_start == nullptr)
            return false;

        Node* oldFront = m_start->next;
        if(oldFront->next.load() == nullptr) // No items left
            return false;

        Node* newFront = oldFront->next.load();

        m_start->next = newFront;

        assert(m_start->data == nullptr);
        const bool ok = newFront->data != nullptr;
        if(ok)
            in = std::move(*(newFront->data));
        if(oldFront->data != nullptr)
        {
            delete oldFront->data;
            oldFront->data = nullptr;
        }
        assert(oldFront != nullptr);
        delete oldFront;
        oldFront = nullptr;

        --m_size;

        assert(ok);
        return ok;
    }

    template <typename T>
    void ConcurrentQueue<T>::push(const T& object)
    {
        // push should never assign nullptr to m_end, so this check is thread-"ok"

        SpinLock pushLock(pushMutex);

        Node* temp = nullptr;
        short mallocCount = 0;
        do
        {
            temp = new (std::nothrow) Node(new T(object));
        } 
        while(temp == nullptr && mallocCount++ < 25);

        assert(m_end != nullptr);
        assert(m_end->next.load() == nullptr);
        assert(mallocCount == 0);
        assert(temp != nullptr);
        if(m_end == nullptr)
            return;
        if(temp == nullptr)
        {
            // new failed a bunch
            return;
        }
        m_end->next = temp;
        m_end = temp;

        assert(m_end != nullptr);
        assert(m_end->next.load() == nullptr);

        ++m_size;
    }

}