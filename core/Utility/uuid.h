#ifndef UUID_H
#define UUID_H
#include <string>

class Uuid
{
public:
    Uuid();
    // Static function to generate a short unique ID
    static std::string generateShortUniqueID();
};

#endif // UUID_H
