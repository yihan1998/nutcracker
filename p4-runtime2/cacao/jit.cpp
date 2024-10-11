#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <iostream>

#include "utils/jit.h"

using namespace llvm;
using namespace llvm::orc;

int jit_init() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    return 0;
}
