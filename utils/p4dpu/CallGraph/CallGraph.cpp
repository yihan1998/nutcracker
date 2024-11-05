#include <iostream>
#include <fstream>
#include <string>

#include "CallGraph.h"
#include "Pipeline.h"

template<typename T>
T getJsonValueOrDefault(const json& j, const std::string& key, const T& defaultValue) {
    if (j.contains(key) && !j[key].is_null()) {
        try {
            return j[key].get<T>();
        } catch (const json::exception& e) {
            std::cerr << "Error retrieving value for key '" << key << "': " << e.what() << std::endl;
        }
    }
    return defaultValue;
}

std::vector<RuntimeData> extractRuntimeData(const json& runtimedata_json) {
    std::vector<RuntimeData> runtimeData;

    for (const auto& data_json : runtimedata_json) {
        std::string name = data_json["name"].get<std::string>();
        int bitwidth = data_json["bitwidth"].get<int>();
        runtimeData.emplace_back(name, bitwidth);
    }

    return runtimeData;
}

Expression::Operator extractOperator(const std::string& op) {
    if (op == "and" || op == "&") return Expression::AND;
    if (op == "==") return Expression::EQUAL;
    throw std::invalid_argument("Unknown operator");
}

std::shared_ptr<Expression> extractExpression(const json& expressions_json) {
    if (expressions_json["type"] == "expression") {
        return std::make_shared<BinaryExpression>(
            extractOperator(expressions_json["value"]["op"]),
            extractExpression(expressions_json["value"]["left"]),
            extractExpression(expressions_json["value"]["right"])
        );
    } else if (expressions_json["type"] == "field") {
        return std::make_shared<FieldExpression>(
            expressions_json["value"].get<std::vector<std::string>>()
        );
    } else if (expressions_json["type"] == "hexstr") {
        return std::make_shared<HexStrExpression>(
            expressions_json["value"]
        );
    }

    throw std::invalid_argument("Unknown type");
}

std::shared_ptr<ParameterBase> createParameter(const json& param_json) {
    std::string type = param_json["type"].get<std::string>();
    if (type == "field") {
        return std::make_shared<FieldParameter>(type, param_json["value"].get<std::vector<std::string>>());
    } else if (type == "hexstr") {
        return std::make_shared<HexStrParameter>(type, param_json["value"].get<std::string>());
    } else if (type == "header") {
        return std::make_shared<HeaderParameter>(type, param_json["value"].get<std::string>());
    } else if (type == "runtime_data") {
        return std::make_shared<RuntimeDataParameter>(type, param_json["value"].get<int>());
    } else if (type == "counter_array") {
        return std::make_shared<CounterArrayParameter>(type, param_json["value"].get<std::string>());
    } else if (type == "expression") {
        return std::make_shared<ExpressionParameter>(type, extractExpression(param_json["value"]));
    } else {
        std::cerr << "\nParsing type: " << type << std::endl;
        throw std::runtime_error("Unknown parameter type");
    }
}

std::vector<Primitive> extractPrimitives(const json& primitives_json) {
    std::vector<Primitive> primitives;

    for (const auto& prim_json : primitives_json) {
        std::string op = prim_json["op"].get<std::string>();
        std::vector<std::shared_ptr<ParameterBase>> parameters;

        for (const auto& param_json : prim_json["parameters"]) {
            parameters.push_back(createParameter(param_json));
        }

        primitives.emplace_back(op, parameters);
    }

    return primitives;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

CallGraphNodeBase* createActionNode(const json& actionJson, CallGraph& callGraph) {
    int id = actionJson["id"];
    std::string name = actionJson["name"];
    
    // Initialize an Action object
    Action* action = nullptr;
    std::vector<Primitive> primitives;

    // Check for source_info in the action JSON
    if (actionJson.contains("source_info")) {
        /* See if it is a non-P4 action */\
        const auto& sourceInfo = actionJson["source_info"];
        std::string filename = sourceInfo["filename"];
        if (endsWith(filename, ".so")) {
            std::vector<Primitive> primitives = extractPrimitives(actionJson["primitives"]);
            action = new Action(id, name, {}, primitives);
        } else {
            std::cout << "Unknown source file!" << std::endl;
        }
    } else {
        /* By default its a P4 action */
        auto runtimeData = extractRuntimeData(actionJson["runtime_data"]);
        std::vector<Primitive> primitives = extractPrimitives(actionJson["primitives"]);
        action = new Action(id, name, runtimeData, primitives);
    }
    ActionNode* actionNode = new ActionNode(action);
    callGraph.actionsMap[id] = actionNode; // Store action node in map
    return actionNode;
}

CallGraphNodeBase* createTableNode(const json& tableJson, CallGraph& callGraph) {
    int id = tableJson["id"];
    std::string name = tableJson["name"];
    std::string baseDefaultNext = getJsonValueOrDefault(tableJson, "base_default_next", std::string{});
    std::vector<Table::Key> keys;
    if (tableJson.contains("key")) {
        for (const auto& key_json : tableJson["key"]) {
            Table::Key key;
            key.match_type = key_json["match_type"].get<std::string>();
            key.name = key_json["name"].get<std::string>();
            key.target = key_json["target"].get<std::vector<std::string>>();
            key.mask = getJsonValueOrDefault(key_json, "mask", std::string{});
            keys.push_back(key);
        }
    }
    std::vector<Table::ActionEntry> actions;
    auto actionIds = tableJson["action_ids"];
    auto actionNames = tableJson["actions"];
    auto nextTables = tableJson["next_tables"];
    for (size_t i = 0; i < actionIds.size(); ++i) {
        int action_id = actionIds[i].get<int>();
        std::string action_name = actionNames[i].get<std::string>();
        std::string next_table = getJsonValueOrDefault(nextTables, "action_name", std::string{});
        actions.emplace_back(action_id, action_name, next_table);
    }
    Table::DefaultEntry defaultEntry;
    if (tableJson.contains("default_entry")) {
        auto default_entry_json = tableJson["default_entry"];
        defaultEntry.action_id = default_entry_json["action_id"].get<int>();
        defaultEntry.action_const = default_entry_json["action_const"].get<bool>();
        defaultEntry.action_data = default_entry_json.contains("action_data") ? 
            default_entry_json["action_data"].get<std::vector<std::string>>() : std::vector<std::string>{};
        defaultEntry.action_entry_const = default_entry_json["action_entry_const"].get<bool>();
    }
    Table* table = new Table(id, name, keys, actions, defaultEntry, baseDefaultNext);
    TableNode* tableNode = new TableNode(table);
    callGraph.tablesMap[id] = tableNode; // Store table node in map
    return tableNode;
}

CallGraphNodeBase* createConditionalNode(const json& condJson, CallGraph& callGraph) {
    int id = condJson["id"].get<int>();
    std::string name = condJson["name"].get<std::string>();
    std::string trueNext = condJson["true_next"].get<std::string>();
    std::string falseNext = condJson["false_next"].get<std::string>();
    auto expr = extractExpression(condJson["expression"]);
    Conditional* cond = new Conditional(id, name, trueNext, falseNext, expr);
    ConditionalNode* condNode = new ConditionalNode(cond);
    callGraph.condMap[id] = condNode; // Store conditional node in map
    return condNode;
}

void createNodesFromJson(const json& actionsJson, const json& pipelineJson, CallGraph& callGraph) {
    // Create action nodes
    for (const auto& actionJson : actionsJson) {
        std::cout  << "  Creating action node " << actionJson["name"];
        ActionNode* actionNode = static_cast<ActionNode*>(createActionNode(actionJson, callGraph));
        // actionNode->print();
        callGraph.addAction(actionNode);
        std::cout <<  "\033[32m" << "\t[DONE]" << "\033[0m" << std::endl;
    }

    // Create table nodes
    for (const auto& tableJson : pipelineJson["tables"]) {
        std::cout  << "  Creating table node " << tableJson["name"];
        TableNode* tableNode = static_cast<TableNode*>(createTableNode(tableJson, callGraph));
        // tableNode->print();
        callGraph.addTable(tableNode);
        std::cout <<  "\033[32m" << "\t[DONE]" << "\033[0m" << std::endl;
    }

    // Create conditional nodes
    for (const auto& condJson : pipelineJson["conditionals"]) {
        std::cout  << "  Creating conditional node " << condJson["name"];
        ConditionalNode* condNode = static_cast<ConditionalNode*>(createConditionalNode(condJson, callGraph));
        // condNode->print();
        callGraph.addConditional(condNode);
        std::cout <<  "\033[32m" << "\t[DONE]" << "\033[0m" << std::endl;
    }
}

void connectNodes(const json& pipelinesJson, CallGraph& callGraph) {
    // Connect conditional nodes
    for (const auto& condJson : pipelinesJson["conditionals"]) {
        int condId = condJson["id"];
        ConditionalNode* condNode = static_cast<ConditionalNode*>(callGraph.condMap[condId]);

        if (condNode) {
            if (condJson.contains("true_next") && !condJson["true_next"].is_null()) {
                std::string trueNextName = condJson["true_next"];
                CallGraphNodeBase* trueNextNode = nullptr;

                // First check in conditionals
                for (const auto& entry : callGraph.condMap) {
                    if (entry.second->getName() == trueNextName) {
                        trueNextNode = entry.second;
                        break;
                    }
                }
                
                // If not found in conditionals, check in tables
                if (!trueNextNode) {
                    for (const auto& entry : callGraph.tablesMap) {
                        if (entry.second->getName() == trueNextName) {
                            trueNextNode = entry.second;
                            break;
                        }
                    }
                }

                condNode->setTrueNextNode(trueNextNode);
                callGraph.insertNode(condNode, trueNextNode);
            }

            if (condJson.contains("false_next") && !condJson["false_next"].is_null()) {
                std::string falseNextName = condJson["false_next"];
                CallGraphNodeBase* falseNextNode = nullptr;

                // First check in conditionals
                for (const auto& entry : callGraph.condMap) {
                    if (entry.second->getName() == falseNextName) {
                        falseNextNode = entry.second;
                        break;
                    }
                }

                // If not found in conditionals, check in tables
                if (!falseNextNode) {
                    for (const auto& entry : callGraph.tablesMap) {
                        if (entry.second->getName() == falseNextName) {
                            falseNextNode = entry.second;
                            break;
                        }
                    }
                }

                condNode->setFalseNextNode(falseNextNode);
                callGraph.insertNode(condNode, falseNextNode);
            }
        }
    }

    // Connect table nodes
    for (const auto& tableJson : pipelinesJson["tables"]) {
        int tableId = tableJson["id"];
        TableNode* tableNode = static_cast<TableNode*>(callGraph.tablesMap[tableId]);

        if (tableNode) {
            // Set default action node
            int defaultActionId = tableJson["default_entry"]["action_id"];
            tableNode->setDefaultActionNode(static_cast<ActionNode*>(callGraph.actionsMap[defaultActionId]));

            // Set action-next table pairs
            for (const auto& nextTableEntry : tableJson["next_tables"].items()) {
                std::string actionName = nextTableEntry.key();
                auto nextTableName = nextTableEntry.value();  // This can be a string or null

                // Find the index of actionName in actions
                auto actionsArray = tableJson["actions"];
                auto actionIdsArray = tableJson["action_ids"];
                auto it = std::find(actionsArray.begin(), actionsArray.end(), actionName);

                if (it != actionsArray.end()) {
                    // Get the index of the action in the actions array
                    size_t index = std::distance(actionsArray.begin(), it);
                    
                    // Get the corresponding action_id
                    int actionId = actionIdsArray[index].get<int>();

                    // Get actionNode using actionId
                    ActionNode* actionNode = static_cast<ActionNode*>(callGraph.actionsMap[actionId]);

                    // Create a nextTableNode that is initially null
                    CallGraphNodeBase* nextTableNode = nullptr;

                    // Check if nextTableName is not null and is not empty
                    if (!nextTableName.is_null() && !nextTableName.empty()) {
                        // Check in conditionals first
                        for (const auto& entry : callGraph.condMap) {
                            if (entry.second->getName() == nextTableName) {
                                nextTableNode = entry.second;
                                break;
                            }
                        }

                        // If not found in conditionals, check in tables
                        if (!nextTableNode) {
                            for (const auto& entry : callGraph.tablesMap) {
                                if (entry.second->getName() == nextTableName) {
                                    nextTableNode = entry.second;
                                    break;
                                }
                            }
                        }
                    }

                    // Create the action-next table pair, nextTableNode could be null
                    tableNode->actionNextTable.emplace_back(actionNode, nextTableNode);

                    // Insert actionNode as a child of tableNode
                    if (actionNode) {
                        std::cout << "Adding action " << actionNode->getName() << " as the child of table " << tableNode->getName() << std::endl;
                        callGraph.insertNode(tableNode, actionNode);
                    }

                    // Insert nextTableNode as a child of the actionNode (if it's not null)
                    if (nextTableNode) {
                        callGraph.insertNode(actionNode, nextTableNode);
                    }
                }
            }

            // Also handle base_default_next if it exists and is not null
            if (tableJson.contains("base_default_next") && !tableJson["base_default_next"].is_null()) {
                std::string baseDefaultNextName = tableJson["base_default_next"];

                // Find the corresponding nextTableNode for base_default_next
                CallGraphNodeBase* baseDefaultNextNode = nullptr;

                // Check in conditionals first
                for (const auto& entry : callGraph.condMap) {
                    if (entry.second->getName() == baseDefaultNextName) {
                        baseDefaultNextNode = entry.second;
                        break;
                    }
                }

                // If not found in conditionals, check in tables
                if (!baseDefaultNextNode) {
                    for (const auto& entry : callGraph.tablesMap) {
                        if (entry.second->getName() == baseDefaultNextName) {
                            baseDefaultNextNode = entry.second;
                            break;
                        }
                    }
                }

                // Insert baseDefaultNextNode as a child of the tableNode (if it's not null)
                // if (baseDefaultNextNode) {
                //     callGraph.insertNode(tableNode, baseDefaultNextNode);
                // }
            }
        }
    }
}

CallGraphNodeBase* findRootNode(const json& pipelineJson, CallGraph& callGraph) {
    std::string rootName = pipelineJson["init_table"]; // Get the name of the init table

    // Try to find the root node in tablesMap first
    for (const auto& entry : callGraph.tablesMap) {
        if (entry.second->getName() == rootName) {
            return entry.second; // Return the found TableNode
        }
    }

    // If not found in tablesMap, check in condMap
    for (const auto& entry : callGraph.condMap) {
        if (entry.second->getName() == rootName) {
            return entry.second; // Return the found ConditionalNode
        }
    }

    // Return nullptr if the root node is not found in either map
    return nullptr;
}

void buildPipelineFromJson(const json& jsonData, std::vector<Pipeline>& pipelines) {
    // Iterate through each pipeline stage
    auto actionsJson = jsonData["actions"];
    for (const auto& pipelineJson : jsonData["pipelines"]) {
        if (pipelineJson["init_table"].empty()) return;
        std::cout  << "Extracting the CallGraph for pipeline " << pipelineJson["name"] << std::endl;
        // Create a new CallGraph instance for this pipeline stage
        CallGraph stageCallGraph;

        // Create nodes for the current stage
        createNodesFromJson(actionsJson, pipelineJson, stageCallGraph);

        // Establish relationships for the current stage
        connectNodes(pipelineJson, stageCallGraph);

        // Set the root node for the current stage
        stageCallGraph.setRoot(findRootNode(pipelineJson, stageCallGraph));

        // If you want to store the call graph for each stage, you can push it to a vector or similar structure
        // callGraphs.push_back(stageCallGraph); // Assuming you have a vector of CallGraph instances
        Pipeline::Kind k = (pipelineJson["name"] == "ingress")? Pipeline::Kind::INGRESS : Pipeline::Kind::EGRESS;
        Pipeline stagePipe(pipelineJson["name"], pipelineJson["id"], k, stageCallGraph);

        pipelines.push_back(stagePipe);
        std::cout <<  "\033[32m" << "[DONE]" << "\033[0m" << std::endl;
    }
}