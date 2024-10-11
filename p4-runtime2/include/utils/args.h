#ifndef _ARGS_H_
#define _ARGS_H_

#include <iostream>
#include <cstdint>
#include <array>
#include <string>

class ArgBase {
public:
    virtual ~ArgBase() = default;
    virtual void print() const = 0; // Pure virtual function to print
};

class ArgUInt32 : public ArgBase {
public:
    ArgUInt32(uint32_t value) : value(value) {}
    void print() const override { std::cout << "uint32: " << value << std::endl; }
    uint32_t value;
};

class ArgUInt16 : public ArgBase {
public:
    ArgUInt16(uint16_t value) : value(value) {}
    void print() const override { std::cout << "uint16: " << value << std::endl; }
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
    std::array<uint8_t, 6> mac;
};

class ArgString : public ArgBase {
public:
    ArgString(const std::string& value) : value(value) {}
    void print() const override { std::cout << "string: " << value << std::endl; }
    std::string value;
};

#endif  /* _ARGS_H_ */