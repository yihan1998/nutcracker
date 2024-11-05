#ifndef _PIPELINE_H_
#define _PIPELINE_H_

#include <map>
#include <vector>
#include <string>

// Assuming Action, Table, Conditional are defined
class Pipeline {
public:
    enum Kind {
        INGRESS = 0,
        EGRESS,
    };

    std::string name;
    int id;
    Kind kind;
    CallGraph callGraph;

    Pipeline(std::string name, int id, Kind K, CallGraph &C)
        : name(name), id(id), kind(K), callGraph(C) {}

    void print() const {
        std::cout << "Pipeline Name: " << name << ", ID: " << id << std::endl;
        // Print other details like actions, tables, conditionals
        callGraph.print(); // Print the call graph
    }
};

extern void buildPipelineFromJson(const json& jsonData, std::vector<Pipeline>& pipelines);

#endif  /* _PIPELINE_H_ */