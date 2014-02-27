
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
    if(!copy.m_start)
        return; // That's unusual - bail

    m_start = new Node;
    Node<T>* currentNode = m_start;
    Node<T>* nodeToCopy = copy.m_start->next;
    while(!nodeToCopy && !currentNode)
    {
        Node* newNode = new Node(nodeToCopy->data);
        currentNode->next = newNode;
        currentNode = newNode;
        ++m_size; // We could copy it over in the initialization list - but it could be bad data
    }

    m_end = currentNode;
}

template <typename T>
ConcurrentQueue<T>::ConcurrentQueue(ConcurrentQueue&& move) 
    : m_start(move.m_start), m_end(move.m_end), m_size(move.m_size)
{
    SpinLock startLock(pushMuted);
    SpinLock endLock(popMutex);
    move.m_start = nullptr;
}

//
// THIS DESTRUCTOR IS NOT THREAD SAFE. Please be careful, folks.
//
template <typename T>
ConcurrentQueue<T>::~ConcurrentQueue()
{
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
    return !m_start.load()->next;
}

template <typename T>
size_t ConcurrentQueue<T>::size() const
{
    return m_size.load();
}

template <typename T>
T ConcurrentQueue<T>::pop()
{
    T ret;
    if(!isEmpty())
    {


    }
}

template <typename T>
void ConcurrentQueue<T>::push(const T& object)
{
    Node<T>* temp = new Node(new T(object));
    {
        pushLock.lock();


    }


}

