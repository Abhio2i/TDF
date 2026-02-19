#include "sharedMemory.h"
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// Constructor: Variables ko default/safe values par set karein
SharedMemory::SharedMemory() : memoryFD(-1), shareddata(nullptr) {
    std::cout << "SharedMemory object created." << std::endl;
    entityHandler = new EntityHandler();
}

// Destructor
SharedMemory::~SharedMemory() {
    cleanup();
    std::cout << "SharedMemory object destroyed." << std::endl;
}

void SharedMemory::create() {
    // 1. Check agar memory pehle se map toh nahi hai
    if (shareddata != nullptr) {
        std::cout << "Warning: Memory already mapped!" << std::endl;
        return;
    }

    // 2. Shared memory create/open
    memoryFD = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (memoryFD == -1) {
        perror("shm_open failed");
        return;
    }

    // 3. Size set
    ftruncate(memoryFD, SIZE);

    // 4. Memory map
    shareddata = (SharedStructs::SharedData*) mmap(nullptr, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memoryFD, 0);
    
    if (shareddata == MAP_FAILED) {
        perror("mmap failed");
        shareddata = nullptr;
    } else {
        entityHandler->setSharedData(shareddata);
        std::cout << "Shared memory created and mapped successfully." << std::endl;
    }
}

void SharedMemory::cleanup() {
    // 1. Check agar memory mapped hai, tabhi unmap karein
    if (shareddata != nullptr && shareddata != MAP_FAILED) {
        munmap(shareddata, SIZE);
        shareddata = nullptr; // Pointer reset karna zaroori hai
        entityHandler->setSharedData(nullptr);
        std::cout << "Memory unmapped." << std::endl;
    }

    // 2. Check agar file descriptor open hai, tabhi close karein
    if (memoryFD != -1) {
        close(memoryFD);
        memoryFD = -1; // FD reset karein
        std::cout << "File descriptor closed." << std::endl;
    }

    shm_unlink(name);
}


std::string SharedMemory::getSharedMemoryName() const {
    return std::string(name);
}

int SharedMemory::getSize() const {
    return SIZE;
}

