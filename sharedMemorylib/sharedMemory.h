#ifndef SharedMemory_H
#define SharedMemory_H
#include <string>
#include "EntityHandler.h"
///create lib file
///g++ -c sharedMemory.cpp -o sharedMemory.o
class SharedMemory {
public:
    // Constructor and Destructor
    SharedMemory();
    ~SharedMemory();

    // Member function
    void create();
    void cleanup();

    std::string getSharedMemoryName() const;
    int getSize() const; 
    EntityHandler* entityHandler;
private:
    SharedStructs::SharedData* shareddata;
    int memoryFD;
    const char* name = "/my_shm";
    const int SIZE = sizeof(SharedStructs::SharedData);
};

#endif