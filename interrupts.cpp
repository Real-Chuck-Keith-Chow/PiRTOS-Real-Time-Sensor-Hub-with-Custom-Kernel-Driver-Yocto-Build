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



