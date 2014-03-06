
#include "SpinMutex.h"

namespace DX
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    // Arbitrary for now, best results TBD
    #ifndef DEFAULT_YIELD_TICKS
        #define DEFAULT_YIELD_TICKS 10
    #endif

    class SpinYieldMutex : public SpinMutex
    {
    public:
        SpinYieldMutex(const size_t maxYieldTicks = DEFAULT_YIELD_TICKS);
        ~SpinYieldMutex();

        void lock() const;
        bool tryLock() const;
        void unlock() const;
    private:
        const   size_t m_maxYieldTicks;

        SpinYieldMutex(const SpinYieldMutex&);  // Do not use
        SpinYieldMutex(SpinYieldMutex&&);       // Do not use
    };

}