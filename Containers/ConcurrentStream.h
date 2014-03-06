////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// First-Pass at high performance single-reader, single-writer queue, "Stream"
// Author: Eli Pinkerton
// Date: 2/26/14
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "AbstractQueue.h"

#include <new>

namespace DX
{

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename T>
    class ConcurrentStream : public Queue<T>
    {
    public:
        ConcurrentStream();
        ConcurrentStream(const ConcurrentStream&);
        ConcurrentStream(ConcurrentStream&&);
        ~ConcurrentStream();

        bool isEmpty() const;
        size_t size() const;
        bool pop(T& in);
        void push(const T&);
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    template <typename T>
    ConcurrentStream<T>::ConcurrentStream() : Queue<T>()
    {
        m_start = new Node<T>();
        m_end = m_start;
        assert(m_start != nullptr);
    }

    template <typename T>
    ConcurrentStream<T>::ConcurrentStream(const ConcurrentStream& copy)
    {
        m_start = new Node<T>();
        assert(m_start != nullptr);
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
    ConcurrentStream<T>::ConcurrentStream(ConcurrentStream&& move)
    {
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
        move.m_size = 0;

        assert(m_start != nullptr);
        assert(m_end != nullptr);
    }

    template <typename T>
    ConcurrentStream<T>::~ConcurrentStream()
    {
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
    bool ConcurrentStream<T>::isEmpty() const
    {
        return m_size == 0;
    }

    template <typename T>
    size_t ConcurrentStream<T>::size() const
    {
        return m_size;
    }

    template <typename T>
    bool ConcurrentStream<T>::pop(T& out)
    {
        assert(m_start != nullptr);

        Node<T>* newStart = m_start->next.load();
        if(newStart == nullptr)
            return false;

        Node<T>* oldStart = m_start;
        m_start = newStart;

        assert(m_start->data != nullptr);
        out = std::move(*(m_start->data));
        assert(m_size > 0);
        --m_size;

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
    void ConcurrentStream<T>::push(const T& in)
    {
        assert(m_end != nullptr);

        Node<T>* temp = new (std::nothrow) Node<T>(new (std::nothrow) T(in));
        assert(temp != nullptr);
        assert(temp->data != nullptr);

        ++m_size;
        m_end->next = temp;
        m_end = temp;
    }

}