#include <fstream>

#include "CallGraph/CallGraph.h"
#include "CallGraph/Pipeline.h"
#include "backends/target.h"
#include "backends/codeGen.h"
#include "backends/c/cOptions.h"
#include "backends/c/cProgram.h"

void emitCProgram(const COptions &options, Pipeline& pipeline, Target *target) {
    CallGraph callGraph = pipeline.callGraph;
    CodeBuilder c(target);
    CodeBuilder h(target);

    auto docaprog = new CProgram(callGraph);
    docaprog->buildHeaderStructure();

    docaprog->j["pipes"] = json::array();

    auto sortedNodes = callGraph.topologicalSort();
    for (const auto* node : sortedNodes) {
        bool isRoot = false;
        if (const auto* conditionalNode = dynamic_cast<const ConditionalNode*>(node)) {
            isRoot = conditionalNode == pipeline.callGraph.getRoot();
        } else if (const auto* tableNode = dynamic_cast<const TableNode*>(node)) {
            isRoot = tableNode == pipeline.callGraph.getRoot();
        }
        docaprog->buildPipe(pipeline.kind, node, isRoot);
    }

    for (const auto* node : sortedNodes) {
        docaprog->initPipe(pipeline.kind, node);
    }

    // callGraph.pipelineMap_.iterate([&](int key, Pipeline* pipeline) {
    //     std::cout << "Pipeline " << key << " :" << std::endl;
    //     if (pipeline) {
    //         docaprog->buildSwFSAPipe();
    //         for (auto action : pipeline->getMatchActionGraph().getActions()) {
    //             action->print(1,1);
    //             if (docaprog->buildActionPipe(action)) continue;
    //         }
    //         for (auto conditional : pipeline->getMatchActionGraph().getConditionals()) {
    //             conditional->print(1,1);
    //             bool isRoot = conditional == pipeline->getMatchActionGraph().getRoot();
    //             if (docaprog->buildConditionalPipe(conditional, isRoot)) continue;
    //         }
    //         for (auto table : pipeline->getMatchActionGraph().getTables()) {
    //             table->print(1,1);
    //             bool isRoot = table == pipeline->getMatchActionGraph().getRoot();
    //             if (docaprog->buildTablePipe(table, isRoot)) continue;
    //         }
    //         for (auto conditional : pipeline->getMatchActionGraph().getConditionals()) {
    //             bool isRoot = conditional == pipeline->getMatchActionGraph().getRoot();
    //             if (docaprog->initConditionalPipe(conditional)) continue;
    //         }
    //         for (auto table : pipeline->getMatchActionGraph().getTables()) {
    //             bool isRoot = table == pipeline->getMatchActionGraph().getRoot();
    //             if (docaprog->initTablePipe(table)) continue;
    //         }
    //     }
    // });

    if (!docaprog->build()) return;

    if (options.outputFolder.empty()) {
        std::cerr << "Please specify output file name!" << std::endl;
        return;
    }

    std::string cFilename(absl::StrCat(pipeline.name,".c"));
    std::filesystem::path outputFile = options.outputFolder / cFilename;

    auto *cstream = new std::ofstream(outputFile);
    if (cstream == nullptr) return;

    std::filesystem::path bcfile = outputFile;
    bcfile.replace_extension(".bc");
    std::filesystem::path absolutePath = std::filesystem::absolute(bcfile);
    docaprog->j["source_code"] = absolutePath;
    std::filesystem::path pipelineName(outputFile);
    docaprog->j["name"] = pipelineName.stem().string();

    std::filesystem::path hfile = outputFile;
    hfile.replace_extension(".h");
    auto *hstream = new std::ofstream(hfile);
    if (hstream == nullptr) return;

    std::filesystem::path jsonfile = outputFile;
    jsonfile.replace_extension(".json");
    auto *jsonstream = new std::ofstream(jsonfile);
    if (jsonstream == nullptr) return;

    std::cout << "Emitting DOCA program => " << "\033[0m";

    docaprog->emitH(&h, hfile);
    docaprog->emitC(&c, hfile);
    *cstream << c.toString();
    *hstream << h.toString();
    *jsonstream << docaprog->j.dump(4);
    cstream->flush();
    hstream->flush();
    jsonstream->flush();

    std::cout <<  "\033[32m" << "[DONE]" << "\033[0m" << std::endl;
}

void run_c_backend(const COptions &options, Pipeline& pipeline) {
    std::cout << "\033[42m\033[37m" << "Running DOCA backend for pipeline " << pipeline.name << "\033[0m" << std::endl;
    auto target = new CTarget();
    emitCProgram(options, pipeline, target);
    return;
}
