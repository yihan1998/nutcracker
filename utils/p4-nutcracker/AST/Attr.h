#ifndef _AST_ATTR_H_
#define _AST_ATTR_H_

#include <cstddef>

#include "backends/codeGen.h"

class ASTContext;

class Attr {
public:
    enum Kind {
        Packed,
    };

    Kind AttrKind;

protected:
    unsigned Inherited;

public:
    void *operator new(size_t size, ASTContext &C, size_t Alignment = 8) {
        return ::operator new(size);
    }
    virtual void emitC(CodeBuilder * builder) const = 0;

protected:
    Attr(ASTContext &Context, Kind AK) : AttrKind(AK), Inherited(false) {}

public:
    Kind getKind() const { return AttrKind; }
};

class InheritableAttr : public Attr {
protected:
    InheritableAttr(ASTContext &Context, Kind AK) : Attr(Context, AK) {}

public:
    void setInherited(bool I) { Inherited = I; }
};

class PackedAttr : public InheritableAttr {
public:
    static PackedAttr *Create(ASTContext &Ctx);
    PackedAttr(ASTContext &Ctx) : InheritableAttr(Ctx, Packed) {}
    PackedAttr *clone(ASTContext &C) const;
    void emitC(CodeBuilder * builder) const override;
};

#endif  /* _AST_ATTR_H_ */