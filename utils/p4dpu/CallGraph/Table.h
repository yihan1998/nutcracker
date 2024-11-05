#ifndef _TABLE_H_
#define _TABLE_H_

#include <iostream>

#define BOLD "\033[1m"
#define RESET "\033[0m"

class Table {
public:
    struct Key {
        std::string match_type;
        std::string name;
        std::vector<std::string> target;
        std::string mask;
    };

    struct ActionEntry {
        int action_id;
        std::string action_name;
        std::string next_table;

        ActionEntry(int id, const std::string& name, const std::string& next)
            : action_id(id), action_name(name), next_table(next) {}

        int actionId() const { return action_id; }
        std::string actionName() const { return action_name; }
        std::string nextTable() const { return next_table; }
    };

    struct DefaultEntry {
        int action_id;
        bool action_const;
        std::vector<std::string> action_data;
        bool action_entry_const;

        int actionId() const { return action_id; }
    };

    Table(int id, const std::string& name, const std::vector<Key>& keys, 
            const std::vector<ActionEntry>& actions, const DefaultEntry& defaultEntry,  const std::string& baseDefaultNext)
        : id(id), name(name), keys(keys), actions(actions), defaultEntry(defaultEntry), baseDefaultNext(baseDefaultNext) {}

    void print(int level = 0, int verbosity = 0) const {
        std::string indent(level * 2, ' ');
        if (verbosity == 0) {
            std::cout << indent << BOLD << "Table" << RESET << " (ID: " << id << ", Name: " << name << ")" << std::endl;
        }
    }

    // bool operator==(const Table& other) const {
    //     return id == other.id;
    // }

    int getId() { return id; }
    const std::string& getName() { return name; }
    std::vector<Key> getKeys() const { return keys; }
    std::vector<ActionEntry> getActions() const { return actions; }
    DefaultEntry getDefaultEntry() const { return defaultEntry; }
    std::string getBaseDefaultNext() const { return baseDefaultNext; }

    const ActionEntry* getDefaultActionEntry() const {
        int defaultActionId = defaultEntry.actionId();
        
        // Iterate over the actions to find the matching action_id
        for (const auto& action : actions) {
            if (action.action_id == defaultActionId) {
                return &action;
            }
        }
        
        return nullptr;
    }

private:
    int id;
    std::string name;
    std::vector<Key> keys;
    std::vector<ActionEntry> actions;
    DefaultEntry defaultEntry;
    std::string baseDefaultNext;
};

#endif  /* _TABLE_H_ */