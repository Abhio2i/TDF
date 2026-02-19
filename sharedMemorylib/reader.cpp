#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "sharedMemory.h"
using namespace SharedStructs;

int main() {
    SharedMemory shm;
    shm.create();
    EntityHandler* handler = shm.entityHandler;
    while(true){
        // std::cout << "Entity count = " << handler->getEntityCount() << std::endl;
        // Application logic yahan dalein
        if(handler->getEntityByIndex(0) != nullptr) {
            Entity* entities = handler->getEntityByIndex(0);
            float x, y, z;
            entities->transformData.position.get(x, y, z);
            std::cout << "Position = (" << x << ", " << y << ", " << z << ")" << std::endl;
            // Entities ke saath kuch karein
        }

        usleep(100000); // Simulate work with sleep (100ms)
    }

    return 0;
}
