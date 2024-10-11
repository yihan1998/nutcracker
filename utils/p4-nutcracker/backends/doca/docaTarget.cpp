#include "backends/target.h"
#include "backends/codeGen.h"

void DocaTarget::emitIncludes(CodeBuilder *builder) const {
    builder->append(
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <stdint.h>\n"
        "#include <stdbool.h>\n"
        "#include <unistd.h>\n");
    builder->append(
        "#include <rte_common.h>\n"
        "#include <rte_eal.h>\n"
        "#include <rte_flow.h>\n"
        "#include <rte_ethdev.h>\n"
        "#include <rte_mempool.h>\n"
        "#include <rte_version.h>\n");
    builder->append("#include <doca_flow.h>\n");
}