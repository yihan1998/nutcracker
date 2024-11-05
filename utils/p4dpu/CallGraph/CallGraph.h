#ifndef _CALLGRAPH_H_
#define _CALLGRAPH_H_

#include <nlohmann/json.hpp>
#include <set>

#include "Action.h"
#include "Table.h"
#include "Conditional.h"
#include "Expression.h"

using json = nlohmann::json;

class CallGraphNodeBase {
public:
    virtual ~CallGraphNodeBase() = default;

    CallGraphNodeBase* parent = nullptr;
    std::vector<CallGraphNodeBase*> children;

    virtual void print(int level = 0, int verbosity = 0) const = 0; // Pure virtual function for printing
    virtual std::string getName() const = 0;

    int getBaseId() const { return baseId; } // Getter for ID
    void setBaseId(int id) { baseId = id; } // Setter for ID

protected:
    CallGraphNodeBase() {};

private:
    int baseId; // Unique ID for the node
};

template <typename T>
class CallGraphNode : public CallGraphNodeBase {
public:
    CallGraphNode(T* data) : data(data) {}

    void print(int level = 0, int verbosity = 0) const override {
        std::cout << std::string(level * 2, ' ') << ", ";
        data->print(level, verbosity);

        // Recursively print child nodes
        for (auto* child : children) {
            child->print(level + 1, verbosity);
        }
    }

    bool operator==(const CallGraphNode& other) const {
        return *data == *(other.data);
    }

    T* data;
};

class ActionNode : public CallGraphNode<Action> {
public:
    ActionNode(Action* action) : CallGraphNode<Action>(action), nextNode_(nullptr) {}

    void setNextNode(CallGraphNodeBase* node) { nextNode_ = node; }
    CallGraphNodeBase* nextNode() const { return nextNode_; }

    int getId() const { return data->getId(); }
    std::string getName() const { return data->getName(); }

    void print(int level = 0, int verbosity = 0) const {
        std::string indent(level * 2, ' ');
        std::cout << indent << "ActionNode: " << getName() << " (id: " << getId() << ", base id: " << getBaseId() << ")" << std::endl;

        if (verbosity) {
            data->print(level+1, verbosity);
        }

        if (nextNode_) {
            std::cout << indent << "  Next Node: " << nextNode_->getName() << std::endl;
        } else {
            std::cout << indent << "  Next Node: [none]" << std::endl;
        }
    }

private:
    CallGraphNodeBase* nextNode_;
};

class TableNode : public CallGraphNode<Table> {
public:
    struct ActionNextTable {
        ActionNode* actionNode;
        CallGraphNodeBase* nextTableNode;

        ActionNextTable(ActionNode* action, CallGraphNodeBase* nextTable) 
            : actionNode(action), nextTableNode(nextTable) {}
    };

    std::vector<ActionNextTable> actionNextTable;

    TableNode(Table* table) : CallGraphNode<Table>(table), defaultActionNode_(nullptr) {}

    int getId() const { return data->getId(); }
    std::string getName() const { return data->getName(); }

    void setDefaultActionNode(ActionNode* defaultActionNode) {
        defaultActionNode_ = defaultActionNode;
    }

    ActionNode* getDefaultActionNode() const { 
        return defaultActionNode_;
    }

    void print(int level = 0, int verbosity = 0) const {
        std::string indent(level * 2, ' ');
        std::cout << indent << "TableNode: " << getName() << " (id: " << getId() << ", base id: " << getBaseId() << ")" << std::endl;

        if (defaultActionNode_) {
            std::cout << indent << "  Default Action Node: " << defaultActionNode_->getName() << std::endl;
        } else {
            std::cout << indent << "  Default Action Node: [none]" << std::endl;
        }

        std::cout << indent << "  Action-Next Table Pairs: " << std::endl;
        for (const auto& pair : actionNextTable) {
            std::cout << indent << "    Action: " << pair.actionNode->getName() 
                      << ", Next Table Node: " 
                      << (pair.nextTableNode ? pair.nextTableNode->getName() : "[null]") 
                      << std::endl;
        }
    }

private:
    ActionNode* defaultActionNode_;
};

class ConditionalNode : public CallGraphNode<Conditional> {
public:
    ConditionalNode(Conditional* conditional) : CallGraphNode<Conditional>(conditional), trueNextNode_(nullptr), falseNextNode_(nullptr) {}

    void setTrueNextNode(CallGraphNodeBase* node) { trueNextNode_ = node; }
    void setFalseNextNode(CallGraphNodeBase* node) { falseNextNode_ = node; }

    CallGraphNodeBase* trueNextNode() const { return trueNextNode_; }
    CallGraphNodeBase* falseNextNode() const { return falseNextNode_; }

    int getId() const { return data->getId(); }
    std::string getName() const { return data->getName(); }

    void print(int level = 0, int verbosity = 0) const {
        std::string indent(level * 2, ' ');
        std::cout << indent << "ConditionalNode: " << getName() << " (id: " << getId() << ", base id: " << getBaseId() << ")" << std::endl;

        if (trueNextNode_) {
            std::cout << indent << "True Next Node: " << std::endl;
            trueNextNode_->print(level + 1, verbosity);
        }

        if (falseNextNode_) {
            std::cout << indent << "False Next Node: " << std::endl;
            falseNextNode_->print(level + 1, verbosity);
        }
    }

private:
    CallGraphNodeBase* trueNextNode_;
    CallGraphNodeBase* falseNextNode_;
};

class CallGraph {
public:
    CallGraph() : root(nullptr), nextId(0) {}

    CallGraphNodeBase* getRoot() const { return root; }

    std::vector<ActionNode*>& getActions() { return actions; }
    void addAction(ActionNode* actionNode) {
        actions.push_back(actionNode);
        actionsMap[actionNode->getId()] = actionNode;  // Assuming ActionNode has a getId() method
    }

    std::vector<TableNode*>& getTables() { return tables; }
    void addTable(TableNode* tableNode) {
        tables.push_back(tableNode);
        tablesMap[tableNode->getId()] = tableNode;  // Assuming TableNode has a getId() method
    }

    std::vector<ConditionalNode*>& getConditionals() { return conditionals; }
    void addConditional(ConditionalNode* conditionalNode) {
        conditionals.push_back(conditionalNode);
        condMap[conditionalNode->getId()] = conditionalNode;  // Assuming ConditionalNode has a getId() method
    }

    void setRoot(CallGraphNodeBase* node) {
        root = node;
    }

    // void insert(CallGraphNodeBase* node) {
    //     std::cout << "Inserting node..." << std::endl;
    //     node->setBaseId(nextId++);
    //     node->print();
    //     if (root == nullptr) {
    //         root = node;
    //     } else {
    //         insertNode(root, node);
    //     }
    // }

    void print(int level = 0, int verbosity = 0) const {
        if (root != nullptr) {
            root->print(level + 1, verbosity);
        }
    }

    // Maps to access nodes by ID
    std::map<int, ActionNode*> actionsMap;     // For storing action nodes by their IDs
    std::map<int, TableNode*> tablesMap;       // For storing table nodes by their IDs
    std::map<int, ConditionalNode*> condMap;    // For storing conditional nodes by their IDs

    std::vector<CallGraphNodeBase*> topologicalSort() {
        std::vector<CallGraphNodeBase*> sortedNodes;
        std::set<CallGraphNodeBase*> visited; // To keep track of visited nodes
        if (root != nullptr) {
            topologicalSortUtil(root, visited, sortedNodes);
        }
        return sortedNodes;
    }

    void insertNode(CallGraphNodeBase* current, CallGraphNodeBase* newNode) {
        current->children.push_back(newNode);
        newNode->parent = current;
    }

private:
    CallGraphNodeBase* root;
    int nextId;

    std::vector<ActionNode*> actions;
    std::vector<TableNode*> tables;
    std::vector<ConditionalNode*> conditionals;

    void topologicalSortUtil(CallGraphNodeBase* node, std::set<CallGraphNodeBase*>& visited, std::vector<CallGraphNodeBase*>& sortedNodes) {
        // Mark the current node as visited
        if (visited.find(node) == visited.end()) {
            node->setBaseId(nextId++);
            visited.insert(node);
            
            // Recur for all children
            for (auto* child : node->children) {
                topologicalSortUtil(child, visited, sortedNodes);
            }
            // Add the current node to the sorted list after its children
            sortedNodes.push_back(node);
        }
    }
};

#endif  /* _CALLGRAPH_H_ */