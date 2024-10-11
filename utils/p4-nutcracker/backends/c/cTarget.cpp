#include "backends/target.h"
#include "backends/codeGen.h"

void CTarget::emitIncludes(CodeBuilder *builder) const {
    builder->append(
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <stdint.h>\n"
        "#include <stdbool.h>\n"
        "#include <unistd.h>\n");
}