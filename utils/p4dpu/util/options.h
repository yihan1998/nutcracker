#ifndef _UTIL_OPTIONS_H_
#define _UTIL_OPTIONS_H_

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

class Options {
public:
    Options(int argc, char* argv[]) {
        parse(argc, argv);
    }

    bool hasOption(const std::string& option) const {
        return options.find(option) != options.end();
    }

    std::string getOptionValue(const std::string& option) const {
        auto it = options.find(option);
        if (it != options.end()) {
            return it->second;
        }
        throw std::runtime_error("Option not found: " + option);
    }

    const std::vector<std::string>& getPositionalArgs() const {
        return positionalArgs;
    }

    // New method to retrieve the from-json option value
    std::string getInputFile() const {
        if (hasOption("--from-json")) {
            return getOptionValue("--from-json");
        }
        return ""; // or throw an exception if preferred
    }

protected:
    std::unordered_map<std::string, std::string> options;
    std::vector<std::string> positionalArgs;

    void parse(int argc, char* argv[]) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg[0] == '-') {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    options[arg] = argv[++i];
                } else {
                    options[arg] = "";
                }
            } else {
                positionalArgs.push_back(arg);
            }
        }
    }
};

#endif  /* _UTIL_OPTIONS_H_ */
