#ifndef _AST_ASTCONTEXT_H_
#define _AST_ASTCONTEXT_H_

#include <cstddef>

#include "Decl.h"
#include "Type.h"
#include "Expr.h"
#include "backends/codeGen.h"

class ASTContext {
public:
    ASTContext() : Idents() {
        VoidTy = new BuiltinType(BuiltinType::Void);
        BoolTy = new BuiltinType(BuiltinType::Bool);
        CharTy = new BuiltinType(BuiltinType::Char);
        IntTy = new BuiltinType(BuiltinType::Int);
        UnsignedCharTy = new BuiltinType(BuiltinType::UChar);
        UnsignedShortTy = new BuiltinType(BuiltinType::UShort);
        UnsignedIntTy = new BuiltinType(BuiltinType::UInt);
        VoidPtrTy = new PointerType(QualType(VoidTy));

        addTranslationUnitDecl();
    }

    void* Allocate(size_t Bytes, size_t Alignment = 8) const {
        return ::operator new(Bytes);
    }

    void emit() {
        if (TUDecl) {
            TUDecl->emitDeclContext();
        } else {
            std::cerr << "No declaration found in AST Context!" << std::endl;
        }
    };

    void emitC(CodeBuilder *builder) {
        if (TUDecl) {
            TUDecl->emitDeclContext(builder);
        } else {
            std::cerr << "No declaration found in AST Context!" << std::endl;
        }
    }

    BuiltinType *VoidTy;
    BuiltinType *BoolTy;
    BuiltinType *CharTy;
    BuiltinType *IntTy;
    BuiltinType *UnsignedCharTy;
    BuiltinType *UnsignedShortTy;
    BuiltinType *UnsignedIntTy;
    PointerType *VoidPtrTy;

    uint64_t getTypeSize(QualType T) const { return T.getTypeWidth(); }

    IdentifierTable *getIdentifierTable() { return &Idents; }
    TranslationUnitDecl *getTranslationUnitDecl() const { return TUDecl; }

    QualType getFunctionType(QualType ResultTy, std::vector<QualType> &Args) const {
        return getFunctionTypeInternal(ResultTy, Args);
    }

    QualType getPointerType(QualType T) const;

private:
    IdentifierTable Idents;
    TranslationUnitDecl *TUDecl = nullptr;

    void addTranslationUnitDecl() {
        /* Create the Root translation Unit Decl Context */
        void* Mem = Allocate(sizeof(TranslationUnitDecl));
        TUDecl = new (Mem) TranslationUnitDecl(*this);
        TUDecl->setParentContext(TUDecl);
    }

    QualType getFunctionTypeInternal(QualType ResultTy, std::vector<QualType> &Args) const;
};

#endif  /* _AST_ASTCONTEXT_H_ */