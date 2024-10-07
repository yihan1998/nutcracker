#ifndef _COMMANDPARSER_H_
#define _COMMANDPARSER_H_

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <sstream>
#include <fstream> 
#include <atomic>
#include <nlohmann/json.hpp>

#include "utils/netStat.h"
#include "backend/flowPipe.h"

using json = nlohmann::json;

using CommandHandler = std::function<void(const std::vector<std::string>&)>;

class CommandParser {
public:
    CommandParser(NetStats& netStats) : netStats(netStats) {
        registerCommand("help", [this](const std::vector<std::string>& args) { displayHelp(args); });
        registerCommand("exit", [&netStats](const std::vector<std::string>&) { netStats.running.store(false); });
        registerCommand("quit", [&netStats](const std::vector<std::string>&) { netStats.running.store(false); });
        registerCommand("load", [this](const std::vector<std::string>& args) { loadJson(args); });
        registerCommand("run_test", [this](const std::vector<std::string>& args) { runTest(args); });
        registerCommand("table_info", [this](const std::vector<std::string>& args) { displayTableInfo(args); });
        registerCommand("table_add", [this](const std::vector<std::string>& args) { addTableEntry(args); });
    }

    void registerCommand(const std::string& name, CommandHandler handler) {
        commands[name] = handler;
    }

    void parseCommand(const std::string& command) {
        std::vector<std::string> tokens = tokenize(command);
        if (!tokens.empty()) {
            executeCommand(tokens);
        }
    }

private:
    [[maybe_unused]] NetStats& netStats;
    std::map<std::string, CommandHandler> commands;

    void executeCommand(const std::vector<std::string>& tokens) {
        const std::string& command = tokens[0];
        auto it = commands.find(command);
        if (it != commands.end()) {
            it->second(tokens);
        } else {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }

    void displayHelp(const std::vector<std::string>&) {
        std::cout << "Available commands:" << std::endl;
        for (const auto& cmd : commands) {
            std::cout << "  " << cmd.first << std::endl;
        }
    }

    void instantializeHwPipe(Pipeline& pipeline, struct flow_pipe * pipe, const json& hwOps);
    void loadJson(const std::vector<std::string>& args);
    void runTest(const std::vector<std::string>& args);
    void displayTableInfo(const std::vector<std::string>& args);
    void addTableEntry(const std::vector<std::string>& args);

    std::vector<std::string> tokenize(const std::string& input) {
        std::istringstream stream(input);
        std::string token;
        std::vector<std::string> tokens;
        while (stream >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }
};

#endif  /* _COMMANDPARSER_H_ */