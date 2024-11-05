#ifndef _RUNTIMEDATA_H_
#define _RUNTIMEDATA_H_

#include <iostream>

#define BOLD "\033[1m"
#define RESET "\033[0m"

class RuntimeData {
public:
    RuntimeData(const std::string& name, int bitwidth)
        : name_(name), bitwidth_(bitwidth) {}

    std::string getName() const { return name_; }
    int getBitwidth() const { return bitwidth_; }

    void print(int level = 0, int verbosity = 0) const {
        std::string indent(level * 2, ' ');
        if (verbosity == 0) {
            std::cout << indent << BOLD << "RuntimeData" << RESET << ": " << name_ << ", Bitwidth: " << bitwidth_ << std::endl;
        } else {
            std::cout << indent << BOLD << "RuntimeData" << RESET << ": " << name_ << ", Bitwidth: " << bitwidth_ << std::endl;
        }
    }

private:
    std::string name_;
    int bitwidth_;
};

#endif  /* _RUNTIMEDATA_H_ */