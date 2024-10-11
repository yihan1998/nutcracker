#ifndef _PARAMETER_H_
#define _PARAMETER_H_

#include <string>
#include <vector>

// Base class for parameter types
class ParameterBase {
public:
    virtual ~ParameterBase() = default;
    virtual void print(int level = 0) const = 0; // Pure virtual function for printing
};

template<typename T>
class Parameter : public ParameterBase {
public:
    Parameter(const std::string& type, const T& value) : type_(type), value_(value) {}

    const T& value() const { return value_; }

    void print(int level = 0) const override {
        std::string indent(level * 2, ' ');
        std::cout << indent << "Type: " << type_ << ", Value: ";
        if constexpr (std::is_same<T, std::string>::value) {
            std::cout << value_;
        } else if constexpr (std::is_same<T, int>::value) {
            std::cout << value_;
        } else if constexpr (std::is_same<T, std::vector<std::string>>::value) {
            std::cout << "[ ";
            for (const auto& v : value_) {
                std::cout << v << " ";
            }
            std::cout << "]";
        }
        std::cout << std::endl;
    }

private:
    std::string type_;
    T value_;
};

// Define the concrete parameter types
using FieldParameter = Parameter<std::vector<std::string>>;
using HexStrParameter = Parameter<std::string>;
using HeaderParameter = Parameter<std::string>;
using RuntimeDataParameter = Parameter<int>;

#endif  /* _PARAMETER_H_ */
