#ifndef _AST_STMT_H_
#define _AST_STMT_H_

#include <string>
#include <vector>
#include <iostream>

#include "absl/strings/str_cat.h"
#include "backends/codeGen.h"
#include "DeclGroup.h"

class ASTContext;
class Expr;
class VarDecl;

class Stmt {
public:
    enum StmtClass {
        NoStmtClass = 0,
        DeclStmtClass,
        CompoundStmtClass,
        ValueStmtClass,
        IfStmtClass,
        ForStmtClass,
        ReturnStmtClass,
        IntegerLiteralClass,
        StringLiteralClass,
        CallExprClass,
        DeclRefExprClass,
        UnaryOperatorClass,
        UnaryExprOrTypeTraitExprClass,
        MemberExprClass,
        CompoundAssignOperatorClass,
        ArraySubscriptExprClass,
    };

    struct EmptyShell {};

    class StmtBitfields {
        friend class Stmt;
        unsigned sClass : 8;
    };

    enum { NumStmtBits = 8 };

    class CompoundStmtBitfields {
        friend class CompoundStmt;
        unsigned : NumStmtBits;
        unsigned NumStmts : 32 - NumStmtBits;
    };

    class ForStmtBitfields {
        friend class ForStmt;

        unsigned : NumStmtBits;
    };

    class ExprBitfields {
        friend class Expr;
        unsigned : NumStmtBits;
        unsigned ValueKind : 2;
        unsigned ObjectKind : 3;
    };

    enum { NumExprBits = NumStmtBits + 5 };

    class StringLiteralBitfields {
        friend class StringLiteral;

        unsigned : NumExprBits;
        unsigned Kind : 3;
        unsigned CharByteWidth : 3;
    };

    class UnaryOperatorBitfields {
        friend class UnaryOperator;
        unsigned : NumExprBits;
        unsigned Opc : 5;
    };

    class UnaryExprOrTypeTraitExprBitfields {
        friend class UnaryExprOrTypeTraitExpr;

        unsigned : NumExprBits;

        unsigned Kind : 3;
        unsigned IsType : 1;
    };

    class MemberExprBitfields {
        friend class MemberExpr;

        unsigned : NumExprBits;

        unsigned IsArrow : 1;
    };

    class BinaryOperatorBitfields {
        friend class BinaryOperator;

        unsigned : NumExprBits;

        unsigned Opc : 6;
    };

    class ArrayOrMatrixSubscriptExprBitfields {
        friend class ArraySubscriptExpr;

        unsigned : NumExprBits;
    };

    union {
        StmtBitfields StmtBits;
        CompoundStmtBitfields CompoundStmtBits;
        ForStmtBitfields ForStmtBits;

        ExprBitfields ExprBits;
        StringLiteralBitfields StringLiteralBits;
        UnaryOperatorBitfields UnaryOperatorBits;
        UnaryExprOrTypeTraitExprBitfields UnaryExprOrTypeTraitExprBits;
        MemberExprBitfields MemberExprBits;
        BinaryOperatorBitfields BinaryOperatorBits;
        ArrayOrMatrixSubscriptExprBitfields ArrayOrMatrixSubscriptExprBits;
    };

    virtual std::string emitStmt(int level) const = 0;
    virtual void emitC(CodeBuilder * builder) const = 0;

public:
    Stmt() = delete;
    Stmt(const Stmt &) = delete;
    Stmt(Stmt &&) = delete;
    Stmt &operator=(const Stmt &) = delete;
    Stmt &operator=(Stmt &&) = delete;

    Stmt(StmtClass SC) {
        StmtBits.sClass = SC;
    }
protected:
    explicit Stmt(StmtClass SC, EmptyShell) : Stmt(SC) {}
};

class DeclStmt : public Stmt {
    DeclGroupRef DG;
public:
    void* operator new(size_t size, const ASTContext &C) {
        // Custom allocation logic here
        return ::operator new(size);
    }

    DeclStmt(DeclGroupRef dg) : Stmt(DeclStmtClass), DG(dg) {}
    static DeclStmt *Create(const ASTContext &C, DeclGroupRef declGroupRef);

    std::string emitStmt(int level = 0) const override;
    void emitC(CodeBuilder * builder) const override;
};

class CompoundStmt : public Stmt {
private:
    std::vector<Stmt *> Stmts;
    CompoundStmt(std::vector<Stmt *> Stmts);
    void setStmts(std::vector<Stmt *> Stmts);

public:
    void* operator new(size_t size, const ASTContext &C) {
        // Custom allocation logic here
        return ::operator new(size);
    }

    static CompoundStmt *Create(const ASTContext &C, std::vector<Stmt *> Stmts);

    bool body_empty() const { return CompoundStmtBits.NumStmts == 0; }
    unsigned size() const { return CompoundStmtBits.NumStmts; }

    const std::vector<Stmt *>& body() const { return Stmts; }

    std::string emitStmt(int level = 0) const override;
    void emitC(CodeBuilder * builder) const override;
};

class ValueStmt : public Stmt {
protected:
    using Stmt::Stmt;
public:
    const Expr *getExprStmt() const;
    Expr *getExprStmt() {
        const ValueStmt *ConstThis = this;
        return const_cast<Expr*>(ConstThis->getExprStmt());
    }
};

class IfStmt : public Stmt {};

class ForStmt : public Stmt {
    enum { INIT, CONDVAR, COND, INC, BODY, END_EXPR };
    Stmt* SubExprs[END_EXPR];

    void *operator new(size_t bytes, const ASTContext &C) {
        return ::operator new(bytes);
    }

public:
    ForStmt(const ASTContext &C, Stmt *Init, Expr *Cond, VarDecl *condVar, Expr *Inc, Stmt *Body);

    VarDecl *getConditionVariable() const;
    void setConditionVariable(const ASTContext &C, VarDecl *V);

    DeclStmt *getConditionVariableDeclStmt() const;
    Stmt *getInit() const;
    Expr *getCond() const;
    Expr *getInc()  const ;
    Stmt *getBody() const;

    void setInit(Stmt *S);
    void setCond(Expr *E);
    void setInc(Expr *E);
    void setBody(Stmt *S);

    std::string emitStmt(int level = 0) const override;
    void emitC(CodeBuilder * builder) const override;
};

class ReturnStmt : public Stmt {
    Stmt *RetExpr;

    ReturnStmt(Expr *E);
public:
    void *operator new(size_t bytes, const ASTContext &C) {
        return ::operator new(bytes);
    }

    static ReturnStmt * Create(const ASTContext &Ctx, Expr *E);

    Expr *getRetValue() const { return reinterpret_cast<Expr *>(RetExpr); }

    std::string emitStmt(int level = 0) const override;
    void emitC(CodeBuilder * builder) const override;
};

#endif  /* _AST_STMT_H_ */