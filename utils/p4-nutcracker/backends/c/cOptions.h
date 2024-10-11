#ifndef _BACKENDS_C_COPTIONS_H_
#define _BACKENDS_C_COPTIONS_H_

#include <filesystem>

#include "util/options.h"

class COptions : public Options {
public:
    std::filesystem::path outputFolder;
    std::filesystem::path inputFile;

    COptions(int argc, char* argv[]) : Options(argc, argv) {
        if (hasOption("-o")) {
            outputFolder = getOptionValue("-o");
        }
        if (hasOption("--from-json")) {
            inputFile = getOptionValue("--from-json");
        }
    }
};

#endif  /* _BACKENDS_DOCA_DOCAOPTIONS_H_ */