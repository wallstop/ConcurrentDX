
#pragma once

#include "../Mutex/SpinLocks.h"

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
        T       pop();
        void    push(const T&);
        void    push(T&&);

    private: 

        template <typename T>
        struct Node
        {
            Node() : next(nullptr), data(nullptr){};
            Node(T* data) :data(data), next(nullptr){};

            T* data;
            std::atomic<Node*> next;
            char pad_[CACHE_LINE_SIZE - sizeof(T*) - sizeof(std::atomic<Node*>)];
        };

        Node<T>* m_start;
        char pad_0[CACHE_LINE_SIZE - sizeof(Node<T>*)];
        Node<T>* m_end;
        char pad_1[CACHE_LINE_SIZE - sizeof(Node<T>*)];    
        // SpinLocks are already padded on their own cache lines, so we don't need anymore padding
        SpinMutex pushMutex;
        SpinMutex popMutex;
        std::atomic<size_t> m_size;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    template <typename T>
    ConcurrentQueue<T>::ConcurrentQueue() : m_start(nullptr), m_end(nullptr), m_size(0)
    {
        m_start = new Node<T>();
        m_end = new Node<T>();
    }

    template <typename T>
    ConcurrentQueue<T>::ConcurrentQueue(const ConcurrentQueue& copy) 
        : m_start(nullptr), m_end(nullptr), m_size(0)
    {
        // TODO: Thread-safe copy constructor
    }

    template <typename T>
    ConcurrentQueue<T>::ConcurrentQueue(ConcurrentQueue&& move) 
        : m_start(nullptr), m_end(nullptr), m_size(0)
    {
        // TODO: Thread-safe move constructor
    }

    template <typename T>
    ConcurrentQueue<T>::~ConcurrentQueue()
    {
        // You never know how you get in these destructors, so *just* in case...
        SpinLock pushLock(pushMutex);
        SpinLock popLock(popMutex);
        while(m_start)
        {
            Node<T>* currentNode = m_start;
            m_start = currentNode->next;
            delete currentNode->data;
            delete currentNode;
        }
    }

    template <typename T>
    bool ConcurrentQueue<T>::isEmpty() const
    {
        return !m_start->next;
    }

    template <typename T>
    size_t ConcurrentQueue<T>::size() const
    {
        return m_size;
    }

    template <typename T>
    T ConcurrentQueue<T>::pop()
    {
        T ret;
        Node<T>* temp;
        SpinLock _lock(popMutex);
        temp = m_start->next;
        if(!temp) // No items left!
            return ret;

        if(temp)
            ret = std::move(m_start->*data);
        
        // Single pass deletion
        // TODO: Consolidate this? We can pass this off to another thread, too
        m_start = temp->next;
        delete temp->data;
        delete temp;
        --m_size;

        return ret;
    }

    template <typename T>
    void ConcurrentQueue<T>::push(const T& object)
    {
        Node<T>* temp = new Node(new T(object));
        SpinLock _lock(pushMutex);
        m_end->next = temp;
        m_end = temp;
        ++m_size;
    }

}
