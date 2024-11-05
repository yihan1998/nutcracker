#ifndef _ACTION_H_
#define _ACTION_H_

#include <iostream>

#include "RuntimeData.h"
#include "Primitive.h"

#define BOLD "\033[1m"
#define RESET "\033[0m"

class Action {
public:
    Action(int id, const std::string& name, const std::vector<RuntimeData>& runtimeData, const std::vector<Primitive>& primitives)
        : id(id), name(name), runtimeData(runtimeData), primitives(primitives) {}

    void print(int level = 0, int verbosity = 0) const {
        std::string indent(level * 2, ' ');
        if (verbosity == 0) {
            std::cout << indent << BOLD << "Action" << RESET << " (ID: " << id << ", Name: " << name << ")" << std::endl;
        } else {
            std::cout << indent << BOLD << "Action" << RESET << " (ID: " << id << ", Name: " << name << ")" << std::endl;
            for (const auto& runtimeData : runtimeData) {
                runtimeData.print(level + 1);
            }
            for (const auto& primitive : primitives) {
                primitive.print(level + 1);
            }
        }
    }

    // bool operator==(const Action& other) const {
    //     return id == other.id;
    // }

    int getId() { return id; }
    const std::string& getName() { return name; }
    const std::vector<RuntimeData> getRuntimeData() { return runtimeData; };
    const std::vector<Primitive>& getPrimitives() { return primitives; }

private:
    int id;
    std::string name;
    std::vector<RuntimeData> runtimeData;
    std::vector<Primitive> primitives;
};

#endif  /* _ACTION_H_ */