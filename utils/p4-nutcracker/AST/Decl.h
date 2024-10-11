#ifndef _AST_DECL_H_
#define _AST_DECL_H_

#include <cassert>

#include "Basic/IdentifierTable.h"
#include "Basic/Specifiers.h"
#include "ADT/APInt.h"
#include "ADT/APSInt.h"
#include "AST/DeclarationName.h"
#include "DeclBase.h"
#include "Stmt.h"
#include "Type.h"

class ASTContext;

class TranslationUnitDecl : public Decl,
                            public DeclContext {
public:
    TranslationUnitDecl(ASTContext &C) : Decl(TranslationUnit), DeclContext(TranslationUnit) {}

    std::string emitDecl() const override;
    void emitC(CodeBuilder * builder) const override;
};

class NamedDecl : public Decl {
    DeclarationName Name;

public:
    DeclarationName getDeclName() const { return Name; }
    std::string emitDecl() const override;
    void emitC(CodeBuilder * builder) const override;
    std::string getName() const { return Name.getName(); }
    const IdentifierInfo *getIdentifier() const { return Name.getAsIdentifierInfo(); }

protected:
    NamedDecl(Kind DK, DeclContext *DC, DeclarationName N)
        : Decl(DK, DC), Name(N) {}
};

class ValueDecl : public NamedDecl {
    QualType DeclType;

public:
    QualType getType() const { return DeclType; }
    void setType(QualType newType) { DeclType = newType; }
    std::string emitDecl() const override;
    void emitC(CodeBuilder * builder) const override;
    std::string getName() const { return NamedDecl::getName(); }

protected:
    ValueDecl(Kind DK, DeclContext *DC, DeclarationName N, QualType T)
        : NamedDecl(DK, DC, N), DeclType(T) {}
};

class DeclaratorDecl : public ValueDecl {
protected:
    DeclaratorDecl(Kind DK, DeclContext *DC, DeclarationName N, QualType T)
      : ValueDecl(DK, DC, N, T) {}

    std::string getName() const { 
        std::cout << "Getting DeclaratorDecl name..." << std::endl;
        return ValueDecl::getName(); 
    }
    QualType getType() const { return ValueDecl::getType(); };

public:
    std::string emitDecl() const override;
    void emitC(CodeBuilder * builder) const override;
};

class VarDecl : public DeclaratorDecl {
public:
    /// Initialization styles.
    enum InitializationStyle {
        /// C-style initialization with assignment
        CInit,

        /// Call-style initialization (C++98)
        CallInit,

        /// Direct list-initialization (C++11)
        ListInit
    };

    void* operator new(size_t size, ASTContext &C, DeclContext *DC) {
        // Custom allocation logic here
        return ::operator new(size);
    }

    VarDecl(Kind DK, ASTContext &C, DeclContext *DC, IdentifierInfo *Id, QualType T, StorageClass SC);

    static VarDecl *Create(ASTContext &C, DeclContext *DC, IdentifierInfo *Id, QualType T, StorageClass S);

    std::string emitDecl() const override;
    void emitC(CodeBuilder * builder) const override;

    std::string getName() const { return DeclaratorDecl::getName(); }
    QualType getType() const { return DeclaratorDecl::getType(); }
};

class ParmVarDecl : public VarDecl {
public:
    void* operator new(size_t size, ASTContext &C, DeclContext *DC) {
        // Custom allocation logic here
        return ::operator new(size);
    }
    static ParmVarDecl * Create(ASTContext &C, DeclContext *DC, IdentifierInfo *Id, QualType T, StorageClass S);
    std::string emitDecl() const override;
    void emitC(CodeBuilder * builder) const override;
    std::string getName() const { return VarDecl::getName(); }
protected:
    ParmVarDecl(Kind DK, ASTContext &C, DeclContext *DC, IdentifierInfo *Id, 
            QualType T, StorageClass S) 
        : VarDecl(DK, C, DC, Id, T, S) {}
};

class FunctionDecl : public DeclaratorDecl, 
                     public DeclContext {
private:
    std::vector<ParmVarDecl *> ParamInfo;
    Stmt *Body;

public:
    void* operator new(size_t size, ASTContext &C, DeclContext *DC) {
        // Custom allocation logic here
        return ::operator new(size);
    }

    std::string getName() const { return NamedDecl::getName(); }

    std::string emitDecl() const override;
    void emitC(CodeBuilder * builder) const override;

    void emitBody(int level) const;

    bool isMain() const;

    QualType getReturnType() const {
        Type* type = getType().getTypePtr();
        FunctionType* funcType = dynamic_cast<FunctionType*>(type);
        assert(funcType && "castAs<FunctionType>() argument of incompatible type!");
        return funcType->getReturnType();
    }

    void Visit();

    bool isExternC() const { return FunctionDeclBits.SClass == SC_Extern; }

    static FunctionDecl *
    Create(ASTContext &C, DeclContext *DC, DeclarationName N, QualType T, StorageClass SC) {
        DeclarationNameInfo NameInfo(N);
        return FunctionDecl::Create(C, DC, NameInfo, T, SC);
    }

    static FunctionDecl *
    Create(ASTContext &C, DeclContext *DC, const DeclarationNameInfo &NameInfo, QualType T, StorageClass SC);

    const FunctionType *getFunctionType() const;
    std::string getFunctionName() const;

    unsigned getNumParams() const;
    void setParams(ASTContext &C, std::vector<ParmVarDecl *> NewParamInfo);
    const std::vector<ParmVarDecl*>& getParmArray() const { return ParamInfo; }

    void setBody(Stmt *B) { Body = B; }
    Stmt * getBody() { return Body; }

protected:
    FunctionDecl(Kind DK, ASTContext &C, DeclContext *DC, 
                const DeclarationNameInfo &NameInfo, QualType T, StorageClass S);
};

class FieldDecl : public DeclaratorDecl {
    unsigned BitField : 1;

    Expr *BitWidth;

protected:
    void* operator new(size_t size, ASTContext &C, DeclContext *DC) {
        // Custom allocation logic here
        return ::operator new(size);
    }

    FieldDecl(Kind DK, DeclContext *DC, IdentifierInfo *Id, QualType T, Expr *BW)
        : DeclaratorDecl(DK, DC, Id, T), BitWidth(BW) {}
public:
    static FieldDecl *Create(ASTContext &C, DeclContext *DC, IdentifierInfo *Id, QualType T, Expr *BW);

    Expr *getBitWidth() const { return BitWidth; }
    void setBitWidth(Expr *Width) {
        assert(!BitField && "bit width already set");
        assert(Width && "no bit width specified");
        BitWidth = Width;
        BitField = true;
    }

    QualType getType() const { return ValueDecl::getType(); } 
    std::string getName() const { return NamedDecl::getName(); } 
    std::string emitDecl() const override;
    void emitC(CodeBuilder * builder);
};

class TypeDecl : public NamedDecl {
    Type *TypeForDecl = nullptr;
protected:
    TypeDecl(Kind DK, DeclContext *DC, IdentifierInfo *Id) : NamedDecl(DK, DC, Id) {}
public:
    const Type *getTypeForDecl() const { return TypeForDecl; }
    void setTypeForDecl(Type *TD) { TypeForDecl = TD; }
    std::string getName() const { return NamedDecl::getName(); }
    std::string emitDecl() const override;
};

class TagDecl : public TypeDecl, public DeclContext {
public:
    using TagKind = TagTypeKind;

    TagKind getTagKind() const {
        return static_cast<TagKind>(TagDeclBits.TagDeclKind);
    }

    void setTagKind(TagKind TK) { TagDeclBits.TagDeclKind = TK; }

    bool isStruct() const { return getTagKind() == TTK_Struct; }
    bool isUnion()  const { return getTagKind() == TTK_Union; }
    bool isEnum()   const { return getTagKind() == TTK_Enum; }

    std::string getName() const {
        std::string prefix = isStruct()? "struct" : isUnion()? "union" : isEnum()? "enum" : "unknown";
        return absl::StrCat(prefix, " ", TypeDecl::getName());
    }

protected:
    TagDecl(Kind DK, TagKind TK, const ASTContext &C, DeclContext *DC, IdentifierInfo *Id, TagDecl *PrevDecl);
    std::string emitDecl() const override;
};

class RecordDecl : public TagDecl {
protected:
    void* operator new(size_t size, ASTContext &C, DeclContext *DC) {
        // Custom allocation logic here
        return ::operator new(size);
    }

    RecordDecl(Kind DK, TagKind TK, ASTContext &C, DeclContext *DC, IdentifierInfo *Id, RecordDecl *PrevDecl) 
        : TagDecl(DK, TK, C, DC, Id, PrevDecl) {}

public:
    const std::vector<Decl*>& getFields() const { return fields; }

    static RecordDecl *Create(ASTContext &C, TagKind TK, DeclContext *DC, IdentifierInfo *Id, RecordDecl* PrevDecl = nullptr);
    std::string emitDecl() const override;
    void emitC(CodeBuilder * builder);
    void addedMember(Decl *D) { fields.push_back(D); }
private:
    std::vector<Decl*> fields;
};

class EnumConstantDecl : public ValueDecl {
    Stmt *Init;
    APSInt Val;
protected:
    void* operator new(size_t size, ASTContext &C, DeclContext *DC) {
        // Custom allocation logic here
        return ::operator new(size);
    }

    EnumConstantDecl(DeclContext *DC, IdentifierInfo *Id, QualType T, Expr *E, const APSInt &V)
        : ValueDecl(EnumConstant, DC, Id, T), Init((Stmt*)E), Val(V) {}

public:
    static EnumConstantDecl *Create(ASTContext &C, EnumDecl *DC, IdentifierInfo *Id, QualType T, Expr *E, const APSInt &V);

    std::string emitDecl() const override {
        return ValueDecl::emitDecl();
    }
    void emitC(CodeBuilder * builder);
};

class EnumDecl : public TagDecl {
protected:
    void* operator new(size_t size, ASTContext &C, DeclContext *DC) {
        // Custom allocation logic here
        return ::operator new(size);
    }

    EnumDecl(ASTContext &C, DeclContext *DC, IdentifierInfo *Id, EnumDecl *PrevDecl)
        : TagDecl(Enum, TTK_Enum, C, DC, Id, PrevDecl) {}

public:
    const std::vector<Decl*>& getEnumerators() const { return enumerators; }

    static EnumDecl *Create(ASTContext &C, DeclContext *DC, IdentifierInfo *Id, EnumDecl *PrevDecl);

    std::string emitDecl() const override;
    void emitC(CodeBuilder * builder);
    void addedEnumerators(Decl *D) { enumerators.push_back(D); }
private:
    std::vector<Decl*> enumerators;
};

#endif  /* _AST_DECL_H_ */