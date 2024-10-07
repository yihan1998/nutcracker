#ifndef _FLOWPIPE_H_
#define _FLOWPIPE_H_

#include <iostream>
#include <vector>
#include <cstring>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "backend/flowPipeInternal.h"

using namespace llvm;
using namespace llvm::orc;

class Pipeline {
public:
    std::string Name;
    std::vector<struct flow_pipe*> Pipes;
    std::unique_ptr<LLJIT> JitEnv;
    std::unique_ptr<LLVMContext> Context;
    std::unique_ptr<ThreadSafeModule> TSM;

    Pipeline(const std::string& Name, const std::string& configName);

    struct flow_pipe* lookupFlowPipe(const char* pipe_name) const {
        for (const auto& pipe : Pipes) {
            if (std::strcmp(pipe->name, pipe_name) == 0) {
                return pipe;
            }
        }
        return nullptr;
    }
};

extern std::vector<Pipeline*> pipelines;

#endif  /* _FLOWPIPE_H_ */