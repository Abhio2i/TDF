#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <csignal>   // Signal handling ke liye
#include "sharedMemory.h"

// Global pointer taaki signal handler ise access kar sake
SharedMemory* globalShmPtr = nullptr;

// Signal handler function
void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received.\n";
    
    if (globalShmPtr != nullptr) {
        globalShmPtr->cleanup(); // Manual cleanup call
    }

    exit(signum); // Program ko safely exit karein
}

int main() {
    SharedMemory shm;
    globalShmPtr = &shm; // Pointer set karein

    // Signals ko register karein
    signal(SIGINT, signalHandler);  // Ctrl+C ke liye
    signal(SIGTERM, signalHandler); // kill command ke liye

    shm.create();
    EntityHandler* handler = shm.entityHandler;
    handler->addEntity(Entity()); // Ek dummy entity add karein
    std::cout << "Program running... Press Ctrl+C to stop." << std::endl;

    while(true) {
        if(handler->getEntityByIndex(0) != nullptr) {
            Entity* entities = handler->getEntityByIndex(0);
            entities->dynamicsData.speed += 1.0f;
            std::cout << "Current Speed: " << entities->dynamicsData.speed << "\r" << std::flush;
        }
        usleep(100000); // 100ms sleep taaki CPU 100% na ho jaye
    }

    
    return 0;
}