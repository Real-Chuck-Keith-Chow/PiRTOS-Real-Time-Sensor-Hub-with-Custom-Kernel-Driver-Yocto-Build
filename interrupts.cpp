//still working on it 

// Interface to our cover class.
template<typename System>
class cover : System
{
public:
    // Called by low priority task to start/end a critical section.
    void protect() { System::protect(); }
    void unprotect() { System::unprotect(); }

    // Called by a high priority task to start/end a synchronization region.
    void sync() { System::sync(); }
    void unsync() { System::unsync(); }
};



template<typename Cover>
class protect_lock {
    Cover& m_c;
public:
    protect_lock(Cover& c) : m_c(c) { c.protect(); }
    ~protect_lock() { m_c.unprotect(); }
};

template<typename Cover>
class sync_lock {
    Cover& m_c;
public:
    sync_lock(Cover& c) : m_c(c) { c.sync(); }
    ~sync_lock() { m_c.unsync(); }
};


//the below is viable on an ARM Cortex-M4 microcontroller

using Cover = cover<armv7_m::cover>;
static Cover cov;
static unsigned count;

extern "C" void SysTickHandler(void)
{
    sync_lock<Cover> lk(cov);
    count++;
}

int main()
{
    setupSysTick();
    while(1) {
        bool odd;
        {
            protect_lock<Cover> lk;
            odd = (count & 1) != 0;
        }
        setLed(odd);
    }
}


// ARMV7-M implementation of cover
namespace armv7-m {
class cover
{
public:
    void protect() { __disable_irq(); sync(); }
    void unprotect() { unsync(); __enable_irq(); }

    void sync() { std::atomic_thread_fence(std::memory_order::acquire); }
    void unsync() { std::atomic_thread_fence(std::memory_order::release); }
};
}

