#ifndef _ARGS_H_
#define _ARGS_H_

#include <iostream>
#include <cstdint>
#include <array>
#include <string>

// Define an enum for argument types
enum class ArgType {
    MAC,
    UINT32,
    UINT16,
    STRING,
    // Add other argument types as needed
};

class ArgBase {
public:
    virtual ~ArgBase() = default;
    virtual void print() const = 0; // Pure virtual function to print
    virtual ArgType getType() const = 0; // Pure virtual function to get type
};

class ArgUInt32 : public ArgBase {
public:
    ArgUInt32(uint32_t value) : value(value) {}
    void print() const override { std::cout << "uint32: " << value << std::endl; }
    ArgType getType() const override { return ArgType::UINT32; } // Implement getType
    uint32_t value;
};

class ArgUInt16 : public ArgBase {
public:
    ArgUInt16(uint16_t value) : value(value) {}
    void print() const override { std::cout << "uint16: " << value << std::endl; }
    ArgType getType() const override { return ArgType::UINT16; } // Implement getType
    uint16_t value;
};

class ArgMac : public ArgBase {
public:
    ArgMac(const std::array<uint8_t, 6>& mac) : mac(mac) {}
    void print() const override {
        std::cout << "MAC: ";
        for (const auto& byte : mac) {
            std::cout << std::hex << static_cast<int>(byte) << ":";
        }
        std::cout << std::dec << std::endl;
    }
    ArgType getType() const override { return ArgType::MAC; } // Implement getType
    std::array<uint8_t, 6> mac;
};

class ArgString : public ArgBase {
public:
    ArgString(const std::string& value) : value(value) {}
    void print() const override { std::cout << "string: " << value << std::endl; }
    ArgType getType() const override { return ArgType::STRING; } // Implement getType
    std::string value;
};

#endif  /* _ARGS_H_ */
