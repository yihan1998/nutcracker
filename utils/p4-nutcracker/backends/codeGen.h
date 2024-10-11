#ifndef _BACKENDS_DOCA_CODEGEN_H_
#define _BACKENDS_DOCA_CODEGEN_H_

#include "util/sourceCodeBuilder.h"

class Target;

class CodeBuilder : public SourceCodeBuilder {
public:
    const Target *target;
    explicit CodeBuilder(const Target *target) : target(target) {}
};

#endif  /* _BACKENDS_DOCA_CODEGEN_H_ */