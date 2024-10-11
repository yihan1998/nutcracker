#ifndef _EXPRESSION_H_
#define _EXPRESSION_H_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#define BOLD "\033[1m"
#define RESET "\033[0m"

class Expression {
public:
    enum Operator {
        AND,
        EQUAL,
        D2B,
    };

    virtual void print(int level = 0) const = 0;
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(Operator op, std::shared_ptr<Expression> left, std::shared_ptr<Expression> right)
        : op_(op), left_(left), right_(right) {}

    void print(int level = 0) const override {
        std::string indent(level * 2, ' ');
        std::cout << indent << "Expression" << std::endl;
        std::cout << indent << "  Operator: " << (op_ == AND ? "AND" : "==") << std::endl;
        std::cout << indent << "  Left: " << std::endl;
        left_->print(level + 1);
        std::cout << indent << "  Right: " << std::endl;
        right_->print(level + 1);
    }

    Operator getOp() { return op_; }
    std::shared_ptr<Expression> getLeft() { return left_; }
    std::shared_ptr<Expression> getRight() { return right_; }

private:
    Operator op_;
    std::shared_ptr<Expression> left_;
    std::shared_ptr<Expression> right_;
};

class FieldExpression : public Expression {
public:
    FieldExpression(const std::vector<std::string>& fields) : fields_(fields) {}

    void print(int level = 0) const override {
        std::string indent(level * 2, ' ');
        std::cout << indent << "Field: ";
        for (const auto& val : fields_) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }

    std::vector<std::string> getFields() { return fields_; }

private:
    std::vector<std::string> fields_;
};

class HexStrExpression : public Expression {
public:
    HexStrExpression(const std::string& hex_value) : hex_value_(hex_value) {}

    void print(int level = 0) const override {
        std::string indent(level * 2, ' ');
        std::cout << indent << "HexStr: " << hex_value_ << std::endl;
    }

    std::string getHexValue() { return hex_value_; }

private:
    std::string hex_value_;
};

#endif  /*_EXPRESSION_H_ */