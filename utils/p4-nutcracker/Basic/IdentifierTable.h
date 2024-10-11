#ifndef _BASIC_IDENTIFIERTABLE_H_
#define _BASIC_IDENTIFIERTABLE_H_

#include <string>
#include <unordered_map>

#include <unordered_map>
#include <memory>

class IdentifierInfo {
public:
    IdentifierInfo(const std::string &name, int id) : name(name), id(id) {}

    const std::string& getName() const { return name; }

private:
    std::string name;
    int id;
};

class IdentifierTable {
public:
    IdentifierInfo& get(const std::string &name) {
        auto it = table.find(name);
        if (it != table.end()) {
            return *(it->second);
        }

        auto identifier = std::make_unique<IdentifierInfo>(name, nextId++);
        IdentifierInfo &ref = *identifier;
        table[name] = std::move(identifier);
        return ref;
    }

private:
    std::unordered_map<std::string, std::unique_ptr<IdentifierInfo>> table;
    int nextId = 0;
};

#endif  /* _BASIC_IDENTIFIERTABLE_H_ */