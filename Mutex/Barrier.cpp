
#include "Barrier.h"

namespace DX
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // impl

    Barrier::Barrier(size_t numThreads) : m_count(numThreads)
    {
    }

    Barrier::~Barrier()
    {
    }

}