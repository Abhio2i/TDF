#include "profiler.h"

Frame* Profiler::currentFrame = nullptr;
Profiler::Profiler() {
    currentFrame = new Frame();
}
Profiler::~Profiler() {
    delete currentFrame;
}
