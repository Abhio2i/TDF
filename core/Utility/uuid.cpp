#include "uuid.h"
#include <string>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
Uuid::Uuid() {}

std::string Uuid::generateShortUniqueID() {
    // 1️⃣ Get current time in nanoseconds
    auto now = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

    // 2️⃣ Generate random 4-byte hex string
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

    uint32_t randPart = dist(gen);

    // 3️⃣ Combine time + random in hex
    std::stringstream ss;
    ss << std::hex << ns << std::setw(8) << std::setfill('0') << randPart;

    return ss.str();
}
