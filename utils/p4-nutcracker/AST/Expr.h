#ifndef _AST_EXPR_H_
#define _AST_EXPR_H_

#include <cassert>
#include "ADT/APInt.h"
#include "AST/APNumericStorage.h"
#include "AST/OperationKinds.h"
#include "AST/TypeTraits.h"
#include "Basic/Specifiers.h"
#include "Stmt.h"
#include "Type.h"
#include "DeclarationName.h"

class ValueDecl;

class Expr : public ValueStmt {
    QualType TR;
public:
    Expr() = delete;
    Expr(const Expr&) = delete;
    Expr(Expr &&) = delete;
    Expr &operator=(const Expr&) = delete;
    Expr &operator=(Expr&&) = delete;
protected:
    Expr(StmtClass SC, QualType T, ExprValueKind VK, ExprObjectKind OK) : ValueStmt(SC) {
        ExprBits.ValueKind = VK;
        ExprBits.ObjectKind = OK;
        assert(ExprBits.ObjectKind == OK && "truncated kind");
        setType(T);
    }
public:
    void setType(QualType t) { TR = t; }
    QualType getType() const { return TR; }

    std::string emitStmt(int level) const override { return emitStmt(level); }
};

class IntegerLiteral : public Expr, public APIntStorage {
public:
    enum Radix {
        Hex,
        Dec
    };

    void* operator new(size_t size, const ASTContext &C) {
        // Custom allocation logic here
        return ::operator new(size);
    }

    // type should be IntTy, LongTy, LongLongTy, UnsignedIntTy, UnsignedLongTy,
    // or UnsignedLongLongTy
    IntegerLiteral(const ASTContext &C, const APInt &V, Radix base, QualType type);

    static IntegerLiteral *Create(const ASTContext &C, const APInt &V, Radix base, QualType type);

    std::string emitStmt(int level = 0) const override;
    void emitC(CodeBuilder * builder) const override;

private:
    Radix radix;
};

class StringLiteral : public Expr {
public:
    enum StringKind { Ascii, Wide, UTF8, UTF16, UTF32 };

protected:
    StringLiteral(const ASTContext &C, std::string &Str, StringKind Kind, QualType Ty);

    void* operator new(size_t size, const ASTContext &C) {
        // Custom allocation logic here
        return ::operator new(size);
    }

    std::string str;

public:
    static StringLiteral *
    Create(const ASTContext &C, std::string &Str, StringKind Kind, QualType Ty);

    std::string emitStmt(int level = 0) const override { return str; }
    void emitC(CodeBuilder * builder) const override;
};

class DeclRefExpr : public Expr {
    ValueDecl *D;

    DeclarationNameInfo NameInfo;

protected:
    DeclRefExpr(const ASTContext &C, ValueDecl *D, const DeclarationNameInfo &NameInfo, QualType T, ExprValueKind VK);

public:
    static DeclRefExpr *
    Create(const ASTContext &C, ValueDecl *D, DeclarationName N, QualType T, ExprValueKind VK);

    std::string getName() const { return NameInfo.getName(); };

    std::string emitStmt(int level = 0) const override;
    void emitC(CodeBuilder * builder) const override;
};

class UnaryOperator : public Expr {
    Stmt *Val;
public:
    typedef UnaryOperatorKind Opcode;

protected:
    void* operator new(size_t size, const ASTContext &C) {
        return ::operator new(size);
    }

    UnaryOperator(const ASTContext &C, Expr *input, Opcode opc, QualType type, ExprValueKind VK, ExprObjectKind OK);

public:
    static UnaryOperator *
    Create(const ASTContext &C, Expr *input, Opcode opc, QualType type, ExprValueKind VK, ExprObjectKind OK);

    std::string emitStmt(int level = 0) const override;
    void emitC(CodeBuilder * builder) const override;
};

class UnaryExprOrTypeTraitExpr : public Expr {
    Stmt *Ex;
protected:
    void* operator new(size_t size, const ASTContext &C) {
        return ::operator new(size);
    }

    UnaryExprOrTypeTraitExpr(UnaryExprOrTypeTrait ExprKind, Expr *E, QualType resultType);

public:
    static UnaryExprOrTypeTraitExpr *
    Create(const ASTContext &C, UnaryExprOrTypeTrait ExprKind, Expr *E, QualType resultType);

    UnaryExprOrTypeTrait getKind() const {
        return static_cast<UnaryExprOrTypeTrait>(UnaryExprOrTypeTraitExprBits.Kind);
    }

    bool isArgumentType() const { return UnaryExprOrTypeTraitExprBits.IsType; }

    Expr *getArgumentExpr() {
        assert(!isArgumentType() && "calling getArgumentExpr() when arg is type");
        return static_cast<Expr*>(Ex);
    }

    void setArgument(Expr *E) {
        Ex = E;
        UnaryExprOrTypeTraitExprBits.IsType = false;
    }

    std::string emitStmt(int level = 0) const override;
    void emitC(CodeBuilder * builder) const override;
};

class CastExpr : public Expr {};

class MemberExpr : public Expr {
    Stmt *Base;
    ValueDecl *MemberDecl;
    DeclarationNameInfo NameInfo;

protected:
    MemberExpr(Expr *Base, bool IsArrow, ValueDecl *MemberDecl, const DeclarationNameInfo &NameInfo, QualType T, ExprValueKind VK, ExprObjectKind OK);

    void* operator new(size_t size, const ASTContext &C) {
        return ::operator new(size);
    }

public:
    static MemberExpr *Create(const ASTContext &C, Expr *Base, bool IsArrow, ValueDecl *MemberDecl, DeclarationName N, QualType T, ExprValueKind VK, ExprObjectKind OK);

    bool isArrow() const;

    std::string emitStmt(int level = 0) const override;
    void emitC(CodeBuilder * builder) const override;
};

class BinaryOperator : public Expr {
    enum { LHS, RHS, END_EXPR };
    Stmt *SubExprs[END_EXPR];

public:
    typedef BinaryOperatorKind Opcode;

protected:
    BinaryOperator(const ASTContext &C, Expr *lhs, Expr *rhs, Opcode opc, QualType ResTy, ExprValueKind VK, ExprObjectKind OK);

    void* operator new(size_t size, const ASTContext &C) {
        return ::operator new(size);
    }

public:
    static BinaryOperator *Create(const ASTContext &C, Expr *lhs, Expr *rhs, Opcode opc, QualType ResTy, ExprValueKind VK, ExprObjectKind OK);

    Opcode getOpcode() const {
        return static_cast<Opcode>(BinaryOperatorBits.Opc);
    }

    void setOpcode(Opcode Opc) { BinaryOperatorBits.Opc = Opc; }
    Expr *getLHS() const { return static_cast<Expr*>(SubExprs[LHS]); }
    void setLHS(Expr *E) { SubExprs[LHS] = E; }
    Expr *getRHS() const { return static_cast<Expr*>(SubExprs[RHS]); }
    void setRHS(Expr *E) { SubExprs[RHS] = E; }

    std::string emitStmt(int level = 0) const override;
    void emitC(CodeBuilder * builder) const override;
};

class StmtExpr : public Expr {};

class InitListExpr : public Expr {};

class DesignatedInitExpr : public Expr {};

class CallExpr : public Expr {
    unsigned NumArgs;
    Expr *Fn;
    std::vector<Expr *> Args;

protected:
    void* operator new(size_t size, const ASTContext &C) {
        return ::operator new(size);
    }

    CallExpr(StmtClass SC, Expr *Fn, std::vector<Expr *> &A, QualType Ty, ExprValueKind VK, unsigned NumArgs);

public:
    static CallExpr *
    Create(const ASTContext &Ctx, Expr *Fn, std::vector<Expr *> &Args, QualType Ty, ExprValueKind VK, unsigned MinNumArgs = 0);

    void setCallee(Expr *F) { Fn = F; }
    Expr * getCallee() const { return Fn; }
    void setArgs(std::vector<Expr *> &A) { Args = A; }
    std::vector<Expr *> getArgs() const { return Args; }

    std::string emitStmt(int level = 0) const override;
    void emitC(CodeBuilder * builder) const override;
};

class ArraySubscriptExpr : public Expr {
    enum { LHS, RHS, END_EXPR };
    Stmt *SubExprs[END_EXPR];

protected:
    void* operator new(size_t size, const ASTContext &C) {
        return ::operator new(size);
    }
    
public:
    ArraySubscriptExpr(Expr *lhs, Expr *rhs, QualType t, ExprValueKind VK, ExprObjectKind OK)
        : Expr(ArraySubscriptExprClass, t, VK, OK) {
        SubExprs[LHS] = lhs;
        SubExprs[RHS] = rhs;
    }

    static ArraySubscriptExpr *
    Create(const ASTContext &C, Expr *lhs, Expr *rhs, QualType t, ExprValueKind VK, ExprObjectKind OK);

    Expr *getLHS() { return dynamic_cast<Expr*>(SubExprs[LHS]); }
    Expr *getLHS() const { return dynamic_cast<Expr*>(SubExprs[LHS]); }
    void setLHS(Expr *E) { SubExprs[LHS] = E; }

    Expr *getRHS() { return dynamic_cast<Expr*>(SubExprs[RHS]); }
    Expr *getRHS() const { return dynamic_cast<Expr*>(SubExprs[RHS]); }
    void setRHS(Expr *E) { SubExprs[RHS] = E; }

    std::string emitStmt(int level = 0) const override;
    void emitC(CodeBuilder * builder) const override;
};

#endif  /* _AST_EXPR_H_ */