#include "backend/flowPipe.h"
#include "backend/doca.h"

static std::vector<std::unique_ptr<LLJIT>> GlobalJITs;

extern "C" struct flow_pipe* flow_get_pipe(char* pipe_name) {
    struct flow_pipe * pipe = NULL;
    for (auto p : pipelines) {
        printf("Looking for pipe %s in pipeline %s...\n", pipe_name, p->Name.c_str());
        pipe = p->lookupFlowPipe(pipe_name);
        if (pipe) {
            printf("Pipe %s is at %p\n", pipe_name, pipe);
            return pipe;
        }
    }
    return NULL;
}

extern "C" int flow_get_pipe_id_by_name(char* pipe_name) {
    struct flow_pipe * pipe = NULL;
    for (auto p : pipelines) {
        printf("Looking for pipe %s in pipeline %s...\n", pipe_name, p->Name.c_str());
        pipe = p->lookupFlowPipe(pipe_name);
        if (pipe) {
            printf("Pipe %s has id %u\n", pipe_name, pipe->id);
            return pipe->id;
        }
    }
    return 0;
}

extern "C" int flow_create_hw_pipe(struct flow_pipe_cfg* pipe_cfg, struct flow_fwd* fwd, struct flow_fwd* fwd_miss, struct flow_pipe* pipe) {
    if (pipe_cfg->attr.type == FLOW_PIPE_CONTROL) {
        return doca_create_hw_control_pipe(pipe, pipe_cfg, fwd, fwd_miss);
    }
    return doca_create_hw_pipe(pipe, pipe_cfg, fwd, fwd_miss);
}

extern "C" int flow_hw_control_pipe_add_entry(struct flow_pipe* pipe, uint8_t priority, struct flow_match* match, struct flow_actions* action, struct flow_fwd* fwd) {
    return doca_hw_control_pipe_add_entry(pipe, priority, match, action, fwd);
}

extern "C" int flow_hw_pipe_add_entry(struct flow_pipe* pipe, struct flow_match* match, struct flow_actions* actions, struct flow_fwd* fwd) {
    return doca_hw_pipe_add_entry(pipe, match, actions, fwd);
}

Pipeline::Pipeline(const std::string& name, const std::string& configName) : Name(name) {
    auto J = cantFail(LLJITBuilder().create());
    JitEnv = std::move(J);

    if (!JitEnv) {
        std::cerr << "Error: JitEnv is null after creation" << std::endl;
        return;
    }

    std::cout << "Getting main JIT DyLib..." << std::endl;
    auto &JD = JitEnv->getMainJITDylib();

    std::cout << "Create Symbols..." << std::endl;

    auto PrintfSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&printf));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("printf"), PrintfSymbol}})));

    auto FlowGetPipeSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&flow_get_pipe));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("flow_get_pipe"), FlowGetPipeSymbol}})));

    auto FlowGetPipeIdByNameSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&flow_get_pipe_id_by_name));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("flow_get_pipe_id_by_name"), FlowGetPipeIdByNameSymbol}})));

    auto FlowCreateHwPipeSymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&flow_create_hw_pipe));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("flow_create_hw_pipe"), FlowCreateHwPipeSymbol}})));

    auto FlowControlPipeAddEntrySymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&flow_hw_control_pipe_add_entry));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("flow_hw_control_pipe_add_entry"), FlowControlPipeAddEntrySymbol}})));

    auto FlowPipeAddEntrySymbol = JITEvaluatedSymbol::fromPointer(reinterpret_cast<void*>(&flow_hw_pipe_add_entry));
    cantFail(JD.define(absoluteSymbols({{JitEnv->mangleAndIntern("flow_hw_pipe_add_entry"), FlowPipeAddEntrySymbol}})));

    auto ObjBuffer = MemoryBuffer::getFile(configName);
    if (!ObjBuffer) {
        errs() << "Error reading bitcode file\n";
    }

    // LLVMContext Context;
    Context = std::make_unique<LLVMContext>();
    auto ModuleOrErr = parseBitcodeFile(ObjBuffer->get()->getMemBufferRef(), *Context);
    if (!ModuleOrErr) {
        errs() << "Error parsing bitcode file\n";
    }

    TSM = std::make_unique<ThreadSafeModule>(std::move(*ModuleOrErr), std::make_unique<LLVMContext>());
    cantFail(JitEnv->addIRModule(std::move(*TSM)));

    std::cout << "JIT Initialization Complete!" << std::endl;
}