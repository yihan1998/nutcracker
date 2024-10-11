#ifndef _AST_APNUMERICSTORAGE_H_
#define _AST_APNUMERICSTORAGE_H_

class APNumericStorage {
    union {
        uint64_t VAL;   ///< Used to store the <= 64 bits integer value.
        uint64_t *pVal; ///< Used to store the >64 bits integer value.
    };
    unsigned BitWidth;

    APNumericStorage(const APNumericStorage &) = delete;

protected:
    APNumericStorage() : VAL(0), BitWidth(0) {}

    APInt getIntValue() const {
        unsigned NumWords = APInt::getNumWords(BitWidth);
        if (NumWords > 1)
            return APInt(BitWidth, NumWords, pVal);
        else
            return APInt(BitWidth, VAL);
    }

    void setIntValue(const ASTContext &C, const APInt &Val);
};

class APIntStorage : private APNumericStorage {
public:
    APInt getValue() const { return getIntValue(); }
    void setValue(const ASTContext &C, const APInt &Val) {
        setIntValue(C, Val);
    }
};

#endif  /* _AST_APNUMERICSTORAGE_H_ */