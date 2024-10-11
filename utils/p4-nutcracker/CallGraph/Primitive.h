#ifndef _PRIMITIVE_H_
#define _PRIMITIVE_H_

#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "Parameter.h"

#define BOLD "\033[1m"
#define RESET "\033[0m"

// Define the Primitive class
class Primitive {
public:
    Primitive(const std::string& op, const std::vector<std::shared_ptr<ParameterBase>>& parameters)
        : op_(op), parameters_(parameters) {}

    void print(int level = 0) const {
        std::string indent(level * 2, ' ');
        std::cout << indent << BOLD << "Operation" << RESET << ": " << op_ << std::endl;
        std::cout << indent << BOLD << "Parameters" << RESET << ": " << std::endl;
        for (const auto& param : parameters_) {
            param->print(level + 1);
        }
    }

    std::string getOp() { return op_; }
    std::vector<std::shared_ptr<ParameterBase>>& getParameter() { return parameters_; }

private:
    std::string op_;
    std::vector<std::shared_ptr<ParameterBase>> parameters_;
};

#endif  /* _PRIMITIVE_H_ */