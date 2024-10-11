#include <sstream>
#include <iomanip>

#include "APInt.h"

std::string APInt::toString(unsigned Radix, bool Signed) const {
    std::string Result;

    // Ensure radix is valid
    if (Radix < 2 || Radix > 16) {
        return Result; // or throw an exception if preferred
    }

    // Convert value to a string in the specified radix
    if (isSingleWord()) {
        uint64_t Value = getZExtValue();
        std::stringstream Stream;

        if (Radix == 16) {
            Stream << "0x" << std::hex << std::setw(getBitWidth()) << std::setfill('0') << Value;
        } else {
            Stream << Value;
        }

        Result = Stream.str();
    }

    return Result;
}