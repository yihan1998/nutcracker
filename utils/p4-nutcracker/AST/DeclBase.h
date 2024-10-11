#ifndef _AST_DECLBASE_H_
#define _AST_DECLBASE_H_

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "AST/Attr.h"
#include "backends/codeGen.h"

class NamedDecl;
class DeclContext;

class Decl {
public:
    enum Kind {
        Var,
        ParmVar,
        Function,
        Tag,
        ObjCContainer,
        Block,
        Field,
        Record,
        Enum,
        EnumConstant,
        Captured,
        Export,
        ExternCContext,
        LinkageSpec,
        Namespace,
        OMPDeclareMapper,
        OMPDeclareReduction,
        ObjCMethod,
        RequiresExprBody,
        TranslationUnit,
    };

    Decl(Kind kind) : DeclKind(kind) {}

    // Virtual destructor for proper cleanup of derived classes
    virtual ~Decl() = default;

    virtual std::string emitDecl() const = 0;
    virtual void emitC(CodeBuilder * builder) const = 0;

    // Get the kind of the declaration
    Kind getKind() const { return DeclKind; }

    void setDeclContext(DeclContext *DC) { DeclCtx = DC; }
    DeclContext *getDeclContext() { return DeclCtx; }

    const FunctionType *getFunctionType() const;

    bool hasAttrs() const { return HasAttrs; }

    void setAttrs(const std::vector<Attr*>& A) {
        assert(!HasAttrs && "Decl already contains attrs.");
        assert(Attrs.empty() && "HasAttrs was wrong?");
        Attrs = A;
        HasAttrs = true;
    }

    const std::vector<Attr*> &getAttrs() const { return Attrs; }
    void dropAttrs();
    void addAttr(Attr *A);

protected:
    Decl(Kind DK, DeclContext *DC)
        : DeclKind(DK), DeclCtx(DC), HasAttrs(false) {}

private:
    Kind DeclKind;  // The kind of this declaration
    DeclContext *DeclCtx;
    unsigned HasAttrs : 1;
    std::vector<Attr*> Attrs;
};

class DeclContext {
protected:
    std::vector<Decl *> Decls;
  
    class DeclContextBitfields {
        friend class DeclContext;
        uint64_t DeclKind : 7;
    };

    enum { NumDeclContextBits = 7 };

    class TagDeclBitfields {
        friend class TagDecl;
        uint64_t : NumDeclContextBits;
        uint64_t TagDeclKind : 3;
    };

    enum { NumTagDeclBits = 9 };

    class RecordDeclBitfields {
        friend class RecordDecl;
        uint64_t : NumDeclContextBits;
        uint64_t : NumTagDeclBits;
    };

    enum { NumRecordDeclBits = 14 };

    class FunctionDeclBitfields {
        friend class FunctionDecl;
        uint64_t : NumDeclContextBits;

        uint64_t SClass : 3;
        uint64_t IsInline : 1;
        uint64_t IsInlineSpecified : 1;
    };

    enum { NumFunctionDeclBits = 12 };

    union {
        DeclContextBitfields DeclContextBits;
        TagDeclBitfields TagDeclBits;
        RecordDeclBitfields RecordDeclBits;
        FunctionDeclBitfields FunctionDeclBits;
    };

    DeclContext *ParentDC;

    std::map<std::string, Decl*> DeclsMap;

    DeclContext(Decl::Kind K);

public:
    void emitDeclContext() const;
    void emitDeclContext(CodeBuilder *builder) const;

    void setParentContext(DeclContext *DC) { ParentDC = DC; }
    DeclContext *getParentContext() { return ParentDC; }

    void addHiddenDecl(Decl *D);
    void addDecl(Decl *D);

    void makeDeclVisible(NamedDecl *D);

    Decl::Kind getDeclKind() const {
        return static_cast<Decl::Kind>(DeclContextBits.DeclKind);
    }

    DeclContext * getPrimaryContext();

    std::vector<Decl *> decls() const { return Decls; }

    virtual ~DeclContext() = default;
};

#endif  /* _AST_DECLBASE_H_ */