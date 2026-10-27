#ifndef PROPERTY_H
#define PROPERTY_H
#include <functional>
#include <iostream>
#include <ostream>



template<typename T>
class Property {
    std::function<T()> getter;
    std::function<void(T)> setter;

public:
    // Getter aur Setter dono ke sath constructor
    Property(std::function<T()> g, std::function<void(T)> s)
        : getter(g), setter(s) {}

    // READ: Jab 'd' ko read karenge (eg: if(d == 0))
    operator T() const {
        return getter();
    }

    // WRITE: Jab 'd' me value assign karenge (eg: d = 10.5f)
    Property& operator=(const T& value) {
        if (setter) {
            setter(value); // Custom setter logic run hoga
        }
        return *this;
    }
};

void testFunction() {
    float actual_data = 0.0f; // Actual variable jo background me value store karega

    // Property setup with Getter and Setter
    Property<float> d(
        [&]() { return actual_data; },          // GETTER: Value read karne par
        [&](float val) { actual_data = val; }   // SETTER: Value assign (d = val) karne par
        );

    std::cout << "Initial value: " << d << std::endl; // Output: 0

    // Value Change kar rahe hain (C# style assignment)
    d = 25.5f;

    if (d == 25.5f) {
        std::cout << "Value updated successfully to: " << d << std::endl;
    }
}
#endif // PROPERTY_H
