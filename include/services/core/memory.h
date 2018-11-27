#ifndef SERVICES_CORE_MEMORY_H
#define SERVICES_CORE_MEMORY_H

namespace services {

template <typename T>
class StackAllocator {
public:
    T& request () {
        
    }
    void release (const T& obj) {

    }

    void clear () {
        top = buffer;
    }

private:
    T* buffer;
    T* top;
};

class MemoryManager {
public:

};


}

#endif // SERVICES_CORE_MEMORY_H