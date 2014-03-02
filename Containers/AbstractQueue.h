////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// First-Pass at high performance Node-type structure for singly linked list-based data structures
// Author: Eli Pinkerton
// Date: 2/26/14
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../CacheLine.h"

#include <atomic>

namespace DX
{

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    template <typename T>
    struct Node
    {
        Node();
        Node(T* data);
        ~Node();

        T* data;
        std::atomic<Node*> next;
        volatile char pad_[CACHE_LINE_SIZE - ((sizeof(T*) + sizeof(std::atomic<Node*>)) % CACHE_LINE_SIZE)];
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    template <typename T>
    Node<T>::Node() : next(nullptr), data(nullptr)
    {
    }

    template <typename T>
    Node<T>::Node(T* _data) : data(_data), next(nullptr)
    {
    }

    template <typename T>
    Node<T>::~Node()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename T>
    class Queue
    {
    public:
        Queue();
        virtual ~Queue() = 0;

        virtual bool isEmpty() const = 0;
        virtual size_t size() const = 0;
        virtual bool pop(T& in) = 0;
        virtual void push(const T& in) = 0;

        bool operator>>(T&);
        Queue& operator<<(const T&);

    protected:
        Node<T>* m_start;
        volatile char pad_0[CACHE_LINE_SIZE - (sizeof(Node<T>*) % CACHE_LINE_SIZE)];
        Node<T>* m_end;
        volatile char pad_1[CACHE_LINE_SIZE - (sizeof(Node<T>*) % CACHE_LINE_SIZE)];
        std::atomic<size_t> m_size;
        volatile char pad_2[CACHE_LINE_SIZE - (sizeof(std::atomic<size_t>) % CACHE_LINE_SIZE)];
    private:
        Queue(const Queue&);
        Queue(Queue&&);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    template <typename T>
    Queue<T>::Queue() : m_start(nullptr), m_end(nullptr), m_size(0)
    {
    }

    template <typename T>
    Queue<T>::~Queue()
    {
    }

    template<typename T>
    Queue<T>& Queue<T>::operator<<(const T& object)
    {
        push(object);
        return *this;
    }

    template <typename T>
    bool Queue<T>::operator>>(T& object)
    {
        bool inserted = false;
        do
        {
            inserted = pop(object);
        }
        while(!inserted);

        return inserted;
    }

}