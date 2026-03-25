#pragma once
// Optional codec interfaces (not wired into the main transmit path).
// Current encryption/encoding happens in src/radiolib.cpp using OpenSSL.
// Keep these as extension points for future custom codecs.
#include <vector>
#include <cstddef>

namespace radio {

class Encoder {
public:
    virtual ~Encoder() = default;
    virtual std::vector<std::byte> encode(const std::vector<std::byte>& data) = 0;
};

class Decoder {
public:
    virtual ~Decoder() = default;
    virtual std::vector<std::byte> decode(const std::vector<std::byte>& data) = 0;
};

// Passthrough encoder (no transformation)
class NullEncoder : public Encoder {
public:
    std::vector<std::byte> encode(const std::vector<std::byte>& data) override {
        return data;
    }
};

// Passthrough decoder
class NullDecoder : public Decoder {
public:
    std::vector<std::byte> decode(const std::vector<std::byte>& data) override {
        return data;
    }
};

} // namespace radio
