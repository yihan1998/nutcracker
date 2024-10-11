#include "ASTContext.h"
#include "Expr.h"
#include "ADT/APInt.h"

#include <cassert>

IntegerLiteral::IntegerLiteral(const ASTContext &C, const APInt &V, Radix base, QualType type)
    : Expr(IntegerLiteralClass, type, VK_PRValue, OK_Ordinary) {
    // assert(type->isIntegerType() && "Illegal type in IntegerLiteral");
    radix = base;
    setValue(C, V);
}

std::string IntegerLiteral::emitStmt(int level) const {
    Type* type = getType().getTypePtr();
    if (auto *builtinType = dynamic_cast<BuiltinType *>(type)) {
        BuiltinType::Kind kind = builtinType->getKind();
        if (kind == BuiltinType::Bool) {
            if (getValue().getZExtValue() == 1) {
                return "true";
            } else {
                return "false";
            }
        } else {
            std::string valueStr;
            // Determine how to format the integer value based on its kind
            switch (kind) {
                case BuiltinType::Int:
                case BuiltinType::UInt:
                    valueStr = getValue().toString(10, /*isSigned=*/true); // Decimal
                    break;
                case BuiltinType::Char:
                case BuiltinType::UChar:
                    valueStr = getValue().toString(10, /*isSigned=*/false); // Decimal
                    break;
                case BuiltinType::Short:
                case BuiltinType::UShort:
                    valueStr = getValue().toString(10, /*isSigned=*/true); // Decimal
                    break;
                // case BuiltinType::Long:
                // case BuiltinType::UnsignedLong:
                //     valueStr = getValue().toString(10, /*isSigned=*/true); // Decimal
                //     break;
                // case BuiltinType::LongLong:
                // case BuiltinType::UnsignedLongLong:
                //     valueStr = getValue().toString(10, /*isSigned=*/true); // Decimal
                //     break;
                default:
                    valueStr = getValue().toString(10, /*isSigned=*/true); // Default to Decimal
                    break;
            }
            return valueStr;
        }
    } else {
        return getValue().toString(10, true);
    }
}

void IntegerLiteral::emitC(CodeBuilder *builder) const {
    Type* type = getType().getTypePtr();
    int base = (radix == Hex)? 16 : 10;
    if (auto *builtinType = dynamic_cast<BuiltinType *>(type)) {
        BuiltinType::Kind kind = builtinType->getKind();
        if (kind == BuiltinType::Bool) {
            if (getValue().getZExtValue() == 1) {
                builder->append("true");
            } else {
                builder->append("false");
            }
        } else {
            std::string valueStr;
            // Determine how to format the integer value based on its kind
            switch (kind) {
                case BuiltinType::Int:
                case BuiltinType::UInt:
                    valueStr = getValue().toString(base, /*isSigned=*/true); // Decimal
                    break;
                case BuiltinType::Char:
                case BuiltinType::UChar:
                    valueStr = getValue().toString(base, /*isSigned=*/false); // Decimal
                    break;
                case BuiltinType::Short:
                case BuiltinType::UShort:
                    valueStr = getValue().toString(base, /*isSigned=*/true); // Decimal
                    break;
                // case BuiltinType::Long:
                // case BuiltinType::UnsignedLong:
                //     valueStr = getValue().toString(10, /*isSigned=*/true); // Decimal
                //     break;
                // case BuiltinType::LongLong:
                // case BuiltinType::UnsignedLongLong:
                //     valueStr = getValue().toString(10, /*isSigned=*/true); // Decimal
                    break;
                default:
                    valueStr = getValue().toString(base, /*isSigned=*/true); // Default to Decimal
                    break;
            }
            builder->append(valueStr);
        }
    } else {
        builder->append(getValue().toString(base, /*isSigned=*/true)); // Default to Decimal
    }
}

IntegerLiteral * IntegerLiteral::Create(const ASTContext &C, const APInt &V, Radix base, QualType type) {
    return new (C) IntegerLiteral(C, V, base, type);
}

void APNumericStorage::setIntValue(const ASTContext &C, const APInt &Val) {
    BitWidth = Val.getBitWidth();
    unsigned NumWords = Val.getNumWords();
    const uint64_t* Words = Val.getRawData();
    if (NumWords > 1) {
        // pVal = new (C) uint64_t[NumWords];
        // std::copy(Words, Words + NumWords, pVal);
    } else if (NumWords == 1) {
        VAL = Words[0];
    } else {
        VAL = 0;
    }
}

StringLiteral::StringLiteral(const ASTContext &C, std::string &Str, StringKind Kind, QualType Ty) 
    : Expr(StringLiteralClass, Ty, VK_LValue, OK_Ordinary) {
    StringLiteralBits.Kind = Kind;
    StringLiteralBits.CharByteWidth = sizeof(char);
    str = Str;
}

StringLiteral *StringLiteral::Create(const ASTContext &C, std::string &Str, StringKind Kind, QualType Ty) {
    return new (C) StringLiteral(C, Str, Kind, Ty);
}

void StringLiteral::emitC(CodeBuilder * builder) const {
    builder->append("\"");
    builder->append(str.c_str());
    builder->append("\"");
}

DeclRefExpr::DeclRefExpr(const ASTContext &C, ValueDecl *D, const DeclarationNameInfo &NameInfo, QualType T, ExprValueKind VK)
    : Expr(DeclRefExprClass, T, VK, OK_Ordinary), D(D), NameInfo(NameInfo) {}

DeclRefExpr *DeclRefExpr::Create(const ASTContext &C, ValueDecl *D, DeclarationName N, QualType T, ExprValueKind VK) {
    DeclarationNameInfo NameInfo(N);
    return new DeclRefExpr(C, D, NameInfo, T, VK);
}
                                    
std::string DeclRefExpr::emitStmt(int level) const {
    return NameInfo.getName();
}

void DeclRefExpr::emitC(CodeBuilder * builder) const {
    auto name = NameInfo.getName();
    builder->append(name.c_str());
}

CallExpr::CallExpr(StmtClass SC, Expr *Fn, std::vector<Expr *> &Args, QualType Ty, ExprValueKind VK, unsigned NumArgs)
    : Expr(SC, Ty, VK, OK_Ordinary), NumArgs(NumArgs) {
    setCallee(Fn);
    setArgs(Args);
}

CallExpr *CallExpr::Create(const ASTContext &Ctx, Expr *Fn, std::vector<Expr *> &Args, QualType Ty, ExprValueKind VK, unsigned MinNumArgs) {
    unsigned NumArgs = std::max<unsigned>(Args.size(), MinNumArgs);
    return new (Ctx) CallExpr(CallExprClass, Fn, Args, Ty, VK, NumArgs);
}

std::string CallExpr::emitStmt(int level) const {
    return "CALL EXPR STMT";
}

void CallExpr::emitC(CodeBuilder * builder) const {
    Expr * callee = getCallee();
    std::vector<Expr *> args = getArgs();

    if (auto f = dynamic_cast<DeclRefExpr*>(callee)) {
        f->emitC(builder);
    }

    builder->append("(");
    int NumArgs = args.size();
    if (NumArgs > 0) {
        for (int i = 0; i < NumArgs; ++i) {
            auto arg = args[i];
            if (auto declRef = dynamic_cast<DeclRefExpr*>(arg)) {
                declRef->emitC(builder);
            } else if (auto member = dynamic_cast<MemberExpr*>(arg)) {
                member->emitC(builder);
            } else if (auto integer = dynamic_cast<IntegerLiteral*>(arg)) {
                integer->emitC(builder);
            } else if (auto string = dynamic_cast<StringLiteral*>(arg)) {
                string->emitC(builder);
            } else if (auto unary = dynamic_cast<UnaryOperator*>(arg)) {
                unary->emitC(builder);
            } else if (auto unaryOrType = dynamic_cast<UnaryExprOrTypeTraitExpr*>(arg)) {
                unaryOrType->emitC(builder);
            } else if (auto arraySubscript = dynamic_cast<ArraySubscriptExpr*>(arg)) {
                arraySubscript->emitC(builder);
            }
            if (i < NumArgs - 1) {
                builder->append(",");
            }
        }
    }
    builder->append(")");
}

UnaryOperator::UnaryOperator(const ASTContext &C, Expr *input, Opcode opc, QualType type, ExprValueKind VK, ExprObjectKind OK)
    : Expr(UnaryOperatorClass, type, VK, OK), Val(input) {
    UnaryOperatorBits.Opc = opc;
}

UnaryOperator *UnaryOperator::Create(const ASTContext &C, Expr *input, Opcode opc, QualType type, ExprValueKind VK, ExprObjectKind OK) {
    return new (C) UnaryOperator(C, input, opc, type, VK, OK);
}

std::string UnaryOperator::emitStmt(int level) const {
    return "UNARY OPERATION STMT";
}

void UnaryOperator::emitC(CodeBuilder * builder) const {
    switch (UnaryOperatorBits.Opc)
    {
    case UO_AddrOf:
        builder->append("&");
        dynamic_cast<DeclRefExpr*>(Val)->emitC(builder);
        break;
    
    default:
        break;
    }
}

UnaryExprOrTypeTraitExpr::UnaryExprOrTypeTraitExpr(UnaryExprOrTypeTrait ExprKind, Expr *E, QualType resultType)
    : Expr(UnaryExprOrTypeTraitExprClass, resultType, VK_PRValue, OK_Ordinary){
    UnaryExprOrTypeTraitExprBits.Kind = ExprKind;
    assert(static_cast<unsigned>(ExprKind) == UnaryExprOrTypeTraitExprBits.Kind &&
            "UnaryExprOrTypeTraitExprBits.Kind overflow!");
    UnaryExprOrTypeTraitExprBits.IsType = false;
    Ex = E;
}

UnaryExprOrTypeTraitExpr *UnaryExprOrTypeTraitExpr::Create(const ASTContext &C, UnaryExprOrTypeTrait ExprKind, Expr *E, QualType resultType) {
    return new (C) UnaryExprOrTypeTraitExpr(ExprKind, E, resultType);
}

std::string UnaryExprOrTypeTraitExpr::emitStmt(int level) const {
    return "UNARY EXPR OR TYPE TRAIT STMT";
}

void UnaryExprOrTypeTraitExpr::emitC(CodeBuilder * builder) const {
    switch (UnaryExprOrTypeTraitExprBits.Kind)
    {
    case UETT_SizeOf:
        builder->append("sizeof(");
        if (auto decl = dynamic_cast<DeclRefExpr*>(Ex)) {
            decl->emitC(builder);
        } else if (auto member = dynamic_cast<MemberExpr*>(Ex)) {
            member->emitC(builder);
        }
        builder->append(")");
        break;
    
    default:
        break;
    }
}

MemberExpr::MemberExpr(Expr *Base, bool IsArrow, ValueDecl *MemberDecl, const DeclarationNameInfo &NameInfo, QualType T, ExprValueKind VK, ExprObjectKind OK)
    : Expr(MemberExprClass, T, VK, OK), Base(Base), MemberDecl(MemberDecl), NameInfo(NameInfo) {
    MemberExprBits.IsArrow = IsArrow;
}

MemberExpr *MemberExpr::Create(const ASTContext &C, Expr *Base, bool IsArrow, ValueDecl *MemberDecl, DeclarationName N, QualType T, ExprValueKind VK, ExprObjectKind OK) {
    DeclarationNameInfo NameInfo(N);
    return new (C) MemberExpr(Base, IsArrow, MemberDecl, NameInfo, T, VK, OK);
}


std::string MemberExpr::emitStmt(int level) const {
    std::string out;
    if (auto declRef = dynamic_cast<DeclRefExpr*>(Base)) {
        out = absl::StrCat(out, declRef->emitStmt());
    } else if (auto member = dynamic_cast<MemberExpr*>(Base)) {
        out = absl::StrCat(out, member->emitStmt());
    }

    if (isArrow()) {
        out = absl::StrCat(out, "->");
    } else {
        out = absl::StrCat(out, ".");
    }

    out = absl::StrCat(out, NameInfo.getName().c_str());

    if (auto declRef = dynamic_cast<DeclRefExpr*>(MemberDecl)) {
        out = absl::StrCat(out, declRef->emitStmt());
    } else if (auto member = dynamic_cast<MemberExpr*>(MemberDecl)) {
        out = absl::StrCat(out, member->emitStmt());
    }

    return out;
}

bool MemberExpr::isArrow() const { return MemberExprBits.IsArrow; }

void MemberExpr::emitC(CodeBuilder * builder) const {
    // builder->append(NameInfo.getName().c_str());
    if (auto declRef = dynamic_cast<DeclRefExpr*>(Base)) {
        declRef->emitC(builder);
    } else if (auto member = dynamic_cast<MemberExpr*>(Base)) {
        member->emitC(builder);
    }

    if (isArrow()) {
        builder->append("->");
    } else {
        builder->append(".");
    }

    builder->append(NameInfo.getName().c_str());

    if (auto declRef = dynamic_cast<DeclRefExpr*>(MemberDecl)) {
        declRef->emitC(builder);
    } else if (auto member = dynamic_cast<MemberExpr*>(MemberDecl)) {
        member->emitC(builder);
    }
}

BinaryOperator::BinaryOperator(const ASTContext &C, Expr *lhs, Expr *rhs, BinaryOperator::Opcode opc, QualType ResTy, ExprValueKind VK, ExprObjectKind OK)
    : Expr(CompoundAssignOperatorClass, ResTy, VK, OK) {
    BinaryOperatorBits.Opc = opc;
    // assert(isCompoundAssignmentOp() && "Use CompoundAssignOperator for compound assignments");
    SubExprs[LHS] = lhs;
    SubExprs[RHS] = rhs;
}

BinaryOperator *BinaryOperator::Create(const ASTContext &C, Expr *lhs, Expr *rhs, BinaryOperator::Opcode opc, QualType ResTy, ExprValueKind VK, ExprObjectKind OK) {
    return new (C) BinaryOperator(C, lhs, rhs, opc, ResTy, VK, OK);
}

std::string BinaryOperator::emitStmt(int level) const {
    return "BINARY OPERATION STMT";
}

void BinaryOperator::emitC(CodeBuilder * builder) const {
    Expr *LHS = getLHS();
    Expr *RHS = getRHS();

    if (LHS) {
        if (auto E = dynamic_cast<DeclRefExpr*>(LHS)) {
            E->emitC(builder);
        } else if (auto E = dynamic_cast<MemberExpr*>(LHS)) {
            E->emitC(builder);
        } else if (auto E = dynamic_cast<ArraySubscriptExpr*>(LHS)) {
            E->emitC(builder);
        }
    }

    switch (BinaryOperatorBits.Opc)
    {
    case BO_Assign:
        builder->append("=");
        break;
    
    default:
        break;
    }

    if (RHS) {
        if (auto E = dynamic_cast<DeclRefExpr*>(RHS)) {
            E->emitC(builder);
            // builder->endOfStatement(true);
        } else if (auto E = dynamic_cast<IntegerLiteral*>(RHS)) {
            E->emitC(builder);
            // builder->endOfStatement(true);
        } else if (auto E = dynamic_cast<StringLiteral*>(RHS)) {
            E->emitC(builder);
            // builder->endOfStatement(true);
        } else if (auto E = dynamic_cast<UnaryOperator*>(RHS)) {
            E->emitC(builder);
        } else if (auto E = dynamic_cast<CallExpr*>(RHS)) {
            E->emitC(builder);
        }
        // builder->endOfStatement(true);
    }
}

ArraySubscriptExpr *ArraySubscriptExpr::Create(const ASTContext &C, Expr *lhs, Expr *rhs, QualType t, ExprValueKind VK, ExprObjectKind OK) {
    return new (C) ArraySubscriptExpr(lhs, rhs, t, VK, OK);
}

std::string ArraySubscriptExpr::emitStmt(int level) const {
    std::string out;
    Expr *LHS = getLHS();
    Expr *RHS = getRHS();

    if (LHS) {
        if (auto E = dynamic_cast<DeclRefExpr*>(LHS)) {
            out = absl::StrCat(out, E->emitStmt());
        } else if (auto E = dynamic_cast<MemberExpr*>(LHS)) {
            out = absl::StrCat(out, E->emitStmt());
        }
    }

    out = absl::StrCat(out, "[");

    if (RHS) {
        if (auto E = dynamic_cast<DeclRefExpr*>(RHS)) {
            out = absl::StrCat(out, E->emitStmt());
        } else if (auto E = dynamic_cast<IntegerLiteral*>(RHS)) {
            out = absl::StrCat(out, E->emitStmt());
        } else if (auto E = dynamic_cast<StringLiteral*>(RHS)) {
            out = absl::StrCat(out, E->emitStmt());
        } else if (auto E = dynamic_cast<UnaryOperator*>(RHS)) {
            out = absl::StrCat(out, E->emitStmt());
        } else if (auto E = dynamic_cast<CallExpr*>(RHS)) {
            out = absl::StrCat(out, E->emitStmt());
        }
    }

    out = absl::StrCat(out, "]");
    return out;
}

void ArraySubscriptExpr::emitC(CodeBuilder * builder) const {
    Expr *LHS = getLHS();
    Expr *RHS = getRHS();

    if (LHS) {
        if (auto E = dynamic_cast<DeclRefExpr*>(LHS)) {
            E->emitC(builder);
        } else if (auto E = dynamic_cast<MemberExpr*>(LHS)) {
            E->emitC(builder);
        }
    }

    builder->append("[");

    if (RHS) {
        if (auto E = dynamic_cast<DeclRefExpr*>(RHS)) {
            E->emitC(builder);
        } else if (auto E = dynamic_cast<IntegerLiteral*>(RHS)) {
            E->emitC(builder);
        } else if (auto E = dynamic_cast<StringLiteral*>(RHS)) {
            E->emitC(builder);
        } else if (auto E = dynamic_cast<UnaryOperator*>(RHS)) {
            E->emitC(builder);
        } else if (auto E = dynamic_cast<CallExpr*>(RHS)) {
            E->emitC(builder);
        }
    }

    builder->append("]");
}
