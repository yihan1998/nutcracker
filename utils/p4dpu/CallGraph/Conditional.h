#ifndef _CONDITIONAL_H_
#define _CONDITIONAL_H_

#include <iostream>

#include "Expression.h"

#define BOLD "\033[1m"
#define RESET "\033[0m"

class Conditional {
public:
    Conditional(int id, const std::string& name, const std::string& trueNext, const std::string& falseNext, std::shared_ptr<Expression> expr)
        : id(id), name(name), trueNext_(trueNext), falseNext_(falseNext), expr_(expr) {}

    void print(int level = 0, int verbosity = 0) const {
        std::string indent(level * 2, ' ');
        if (verbosity == 0) {
            std::cout << indent << BOLD << "Conditional" << RESET << " (ID: " << id << ", Name: " << name << ")" << std::endl;
            std::cout << indent << "  True Next: " << (trueNext_.empty() ? "[empty]" : trueNext_) << std::endl;
            std::cout << indent << "  False Next: " << (falseNext_.empty() ? "[empty]" : falseNext_) << std::endl;
        } else {
            std::cout << indent << BOLD << "Conditional" << RESET << " (ID: " << id << ", Name: " << name << ")" << std::endl;
            std::cout << indent << "  True Next: " << (trueNext_.empty() ? "[empty]" : trueNext_) << std::endl;
            std::cout << indent << "  False Next: " << (falseNext_.empty() ? "[empty]" : falseNext_) << std::endl;

            if (expr_) {
                std::cout << indent << "  Expression: " << std::endl;
                expr_->print(level + 1);
            }
        }
    }

    int getId() { return id; }
    const std::string& getName() { return name; }
    std::string trueNext() const { return trueNext_; }
    std::string falseNext() const { return falseNext_; }
    std::shared_ptr<Expression> expr() const { return expr_; }

private:
    int id;
    std::string name;
    std::string trueNext_;
    std::string falseNext_;
    std::shared_ptr<Expression> expr_;
};

#endif  /* _CONDITIONAL_H_ */