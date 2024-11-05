#ifndef _BACKENDS_DOCA_DOCAOPTIONS_H_
#define _BACKENDS_DOCA_DOCAOPTIONS_H_

#include <filesystem>

#include "util/options.h"

class DocaOptions : public Options {
public:
    std::filesystem::path outputFolder;

    DocaOptions(int argc, char* argv[]) : Options(argc, argv) {
        if (hasOption("-o")) {
            outputFolder = getOptionValue("-o");
        }
    }
};

#endif  /* _BACKENDS_DOCA_DOCAOPTIONS_H_ */
