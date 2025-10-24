#ifndef COLOR_H
#define COLOR_H
#include<qobject.h>

class Color: public QObject
{
    Q_OBJECT
public:
    Color();
    int r;
    int g;
    int b;

    void toJson();
    void fromJson();
};

#endif // COLOR_H
