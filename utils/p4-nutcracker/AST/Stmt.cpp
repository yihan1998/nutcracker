#include "Stmt.h"
#include "Expr.h"
#include "Decl.h"

ReturnStmt::ReturnStmt(Expr *E) : Stmt(ReturnStmtClass), RetExpr(E) {}

ReturnStmt * ReturnStmt::Create(const ASTContext &Ctx, Expr *E) {
    return new (Ctx) ReturnStmt(E);
}

std::string ReturnStmt::emitStmt(int level) const {
    std::string indent(level * 2, ' ');
    std::string body = absl::StrCat(indent, "return");
    Expr * retValue = getRetValue();
    if (retValue) {
        body = absl::StrCat(body, " ");
        body = absl::StrCat(body, retValue->emitStmt(level));
    }
    body = absl::StrCat(body, ";\n");
    return body;
}

void ReturnStmt::emitC(CodeBuilder * builder) const {
    builder->append("return");
    Expr * retValue = getRetValue();
    if (retValue) {
        builder->append(" ");
        retValue->emitC(builder);
    }
    // builder->endOfStatement(true);
}

const Expr *ValueStmt::getExprStmt() const {
    const Stmt *S = this;
    if (auto *E = dynamic_cast<const Expr*>(S))
        return E;

    // if (const auto *LS = dynamic_cast<const LabelStmt*>(S))
    //     S = LS->getSubStmt();
    // else if (const auto *AS = dynamic_cast<const AttributedStmt*>(S))
    //     S = AS->getSubStmt();
    // else
    //     std::cerr << "unknown kind of ValueStmt" << std::endl;

    return nullptr;
}

CompoundStmt::CompoundStmt(std::vector<Stmt *> Stmts) : Stmt(CompoundStmtClass) {
    CompoundStmtBits.NumStmts = Stmts.size();
    setStmts(Stmts);
}

void CompoundStmt::setStmts(std::vector<Stmt *> InputStmts) {
    assert(CompoundStmtBits.NumStmts == InputStmts.size() &&
            "NumStmts doesn't fit in bits of CompoundStmtBits.NumStmts!");
    Stmts = InputStmts;
}

CompoundStmt *CompoundStmt::Create(const ASTContext &C, std::vector<Stmt *> Stmts) {
    return new (C) CompoundStmt(Stmts);
}

std::string CompoundStmt::emitStmt(int level) const {
    std::string body;
    for (auto stmt : Stmts) {
        body = absl::StrCat(body, stmt->emitStmt(level + 1));
    }
    return body;
}

void CompoundStmt::emitC(CodeBuilder * builder) const {
    builder->blockStart();
    for (auto stmt : Stmts) {
        builder->emitIndent();
        stmt->emitC(builder);
        if (!dynamic_cast<CompoundStmt*>(stmt)) {
            builder->endOfStatement(true);
        }
    }
    builder->blockEnd(true);
}

DeclStmt *DeclStmt::Create(const ASTContext &C, DeclGroupRef declGroupRef) {
    return new (C) DeclStmt(declGroupRef);
}

std::string DeclStmt::emitStmt(int level) const {
    return "Hello world";
}

void DeclStmt::emitC(CodeBuilder * builder) const {
    VarDecl * varDecl = dynamic_cast<VarDecl *>(DG.getDecl());
    varDecl->emitC(builder);
    // builder->endOfStatement(true);
}

ForStmt::ForStmt(const ASTContext &C, Stmt *Init, Expr *Cond, VarDecl *condVar, Expr *Inc, Stmt *Body)
    : Stmt(ForStmtClass) {
    SubExprs[INIT] = Init;
    setConditionVariable(C, condVar);
    SubExprs[COND] = Cond;
    SubExprs[INC] = Inc;
    SubExprs[BODY] = Body;
}

VarDecl *ForStmt::getConditionVariable() const {
    if (!SubExprs[CONDVAR])
        return nullptr;

    auto DS = dynamic_cast<DeclStmt*>(SubExprs[CONDVAR]);
    return dynamic_cast<VarDecl*>(DS);
}
void ForStmt::setConditionVariable(const ASTContext &C, VarDecl *V) {
    if (!V) {
        SubExprs[CONDVAR] = nullptr;
        return;
    }
    SubExprs[CONDVAR] = new (C) DeclStmt(DeclGroupRef(V));
}

DeclStmt *ForStmt::getConditionVariableDeclStmt() const { return dynamic_cast<DeclStmt*>(SubExprs[CONDVAR]); }
Stmt *ForStmt::getInit() const { return SubExprs[INIT]; }
Expr *ForStmt::getCond() const { return static_cast<Expr*>(SubExprs[COND]);}
Expr *ForStmt::getInc()  const { return static_cast<Expr*>(SubExprs[INC]); }
Stmt *ForStmt::getBody() const { return SubExprs[BODY]; }

void ForStmt::setInit(Stmt *S) { SubExprs[INIT] = S; }
void ForStmt::setCond(Expr *E) { SubExprs[COND] = static_cast<Stmt*>(E); }
void ForStmt::setInc(Expr *E) { SubExprs[INC] = static_cast<Stmt*>(E); }
void ForStmt::setBody(Stmt *S) { SubExprs[BODY] = S; }

std::string ForStmt::emitStmt(int level) const {
    return "Hello world";
}

void ForStmt::emitC(CodeBuilder * builder) const {
    // Stmt * init = getInit();
    // Expr * cond = getCond();
    // Expr * inc = getInc();
    // Stmt * body = getBody();

    // if (auto initStmt = dynamic_cast<DeclStmt*>(init)) {
    //     initStmt->emitC(builder);
    // }
}