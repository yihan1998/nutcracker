#include "Attr.h"

PackedAttr *PackedAttr::Create(ASTContext &Ctx) {
    return new (Ctx) PackedAttr(Ctx);
}

PackedAttr *PackedAttr::clone(ASTContext &C) const {
    auto *A = new (C) PackedAttr(C);
    A->Inherited = Inherited;
    return A;
}

void PackedAttr::emitC(CodeBuilder * builder) const {
    builder->append("packed");
}