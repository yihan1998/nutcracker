#ifndef _ADT_APSINT_H_
#define _ADT_APSINT_H_

#include "APInt.h"

class APSInt : public APInt {
    bool IsUnsigned;
public:
    explicit APSInt() : IsUnsigned(false) {}
    explicit APSInt(uint32_t BitWidth, bool isUnsigned = true)
        : APInt(BitWidth, 0), IsUnsigned(isUnsigned) {}

    explicit APSInt(APInt I, bool isUnsigned = true)
        : APInt(std::move(I)), IsUnsigned(isUnsigned) {}
};

#endif  /* _ADT_APSINT_H_ */