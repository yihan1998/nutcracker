#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

#include "CallGraph/CallGraph.h"
#include "CallGraph/Pipeline.h"
#include "Backends/doca/docaOptions.h"
#include "Backends/doca/docaBackend.h"

using json = nlohmann::json;

// ./build/p4_nutcracker ~/Nutcracker/utils/simple_fwd/simple_fwd_v1switch/simple_fwd_v1switch.json/simple_fwd_v1switch.json
int main(int argc, char ** argv) {
    auto options = Options(argc, argv); // Use Options directly
    std::string inputFile = options.getInputFile(); // Retrieve the input file

    if (!inputFile.empty()) {
        std::cout << "Input file: " << inputFile << std::endl;
    }

    std::ifstream file(inputFile);
    json j;
    file >> j;

    std::vector<Pipeline> pipelines;

    std::cout << "\033[42m\033[37m" << "Building pipelines" << "\033[0m" << std::endl;
    buildPipelineFromJson(j, pipelines);

    std::cout << "\033[42m\033[37m" << "Printing pipelines" << "\033[0m" << std::endl;
    // Print the call graph
    for (auto pipeline : pipelines) {
        pipeline.print();
    }

    std::cout << "\033[42m\033[37m" << "Sorting callgraph in each pipeline" << "\033[0m" << std::endl;
    for (auto pipeline : pipelines) {
        auto sortedNodes = pipeline.callGraph.topologicalSort();
        for (const auto* node : sortedNodes) {
            std::cout << node->getName() << std::endl;
            node->print(1);
        }
    }

    // std::cout << "\033[42m\033[37m" << "Generating DOCA backend" << "\033[0m" << std::endl;
    // for (auto pipeline : pipelines) {
    //     auto options = DocaOptions(argc, argv);
    //     if (!options.inputFile.empty()) {
    //         std::cout << "Input file: " << options.inputFile << std::endl;
    //     }
    //     run_doca_backend(options, pipeline);
    // }
    // std::cout << "[DONE]" << std::endl;

    return 0;
}
