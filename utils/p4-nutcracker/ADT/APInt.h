#ifndef _ADT_APINT_H_
#define _ADT_APINT_H_

#include <cassert>
#include <climits>
#include <ostream>
#include <iostream>

#include "absl/strings/str_cat.h"

class APInt {
public:
    enum : unsigned {
        /// Byte size of a word.
        APINT_WORD_SIZE = sizeof(uint64_t),
        /// Bits in a word.
        APINT_BITS_PER_WORD = APINT_WORD_SIZE * CHAR_BIT
    };

    bool isSingleWord() const { return BitWidth <= APINT_BITS_PER_WORD; }

    const uint64_t *getRawData() const {
        if (isSingleWord()) {
            return &U.VAL;
        }
        return &U.pVal[0];
    }

    std::string toString(unsigned Radix, bool Signed) const;

    unsigned getBitWidth() const { return BitWidth; }

    unsigned getNumWords() const { return getNumWords(BitWidth); }
    static unsigned getNumWords(unsigned BitWidth) {
        return ((uint64_t)BitWidth + APINT_BITS_PER_WORD - 1) / APINT_BITS_PER_WORD;
    }

    explicit APInt() : BitWidth(1) { U.VAL = 0; }

    APInt(unsigned numBits, uint64_t val, bool isSigned = false)
        : BitWidth(numBits) {
        if (isSingleWord()) {
            U.VAL = val;
        } else {
            std::cerr << "Not a single word!" << std::endl;
        }
    }

    uint64_t getZExtValue() const {
        if (isSingleWord())
            return U.VAL;
        return U.pVal[0];
    }

private:
    union {
        uint64_t VAL;   ///< Used to store the <= 64 bits integer value.
        uint64_t *pVal; ///< Used to store the >64 bits integer value.
    } U;
    unsigned BitWidth; ///< The number of bits in this APInt.
};

#endif  /* _ADT_APINT_H_ */