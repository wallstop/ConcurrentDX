
#pragma once

namespace DX
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    class Mutex
    {
    public:
        Mutex();
        virtual ~Mutex() = 0;

        virtual void lock() const = 0;
        virtual void unlock() const = 0;
    private:
        Mutex(const Mutex&);
        Mutex(Mutex&&);
    };

}