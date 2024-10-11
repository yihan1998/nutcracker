#include "Decl.h"
#include "Expr.h"
#include "backends/codeGen.h"

std::string TranslationUnitDecl::emitDecl() const {
    return "";
};

void TranslationUnitDecl::emitC(CodeBuilder * builder) const {
    // for (const auto& pair : DeclsMap) {
    // for (auto it = DeclsMap.rbegin(); it != DeclsMap.rend(); ++it) {
    //     const std::string& key = it->first;
    //     Decl* decl = it->second;

    //     if (auto* namedDecl = dynamic_cast<NamedDecl*>(decl)) {
    //         if (auto* functionDecl = dynamic_cast<FunctionDecl*>(namedDecl)) {
    //             if (functionDecl->isMain()) continue;
    //             namedDecl->emitC(builder);
    //         } else if (auto* recordDecl = dynamic_cast<RecordDecl*>(namedDecl)) {
    //             recordDecl->emitC(builder);
    //         } else if (auto* enumDecl = dynamic_cast<EnumDecl*>(namedDecl)) {
    //             enumDecl->emitC(builder);
    //         }
    //     }
    // }
    for (const auto decl : Decls) {
        if (auto* namedDecl = dynamic_cast<NamedDecl*>(decl)) {
            if (auto* functionDecl = dynamic_cast<FunctionDecl*>(namedDecl)) {
                if (functionDecl->isMain()) continue;
                namedDecl->emitC(builder);
            } else if (auto* recordDecl = dynamic_cast<RecordDecl*>(namedDecl)) {
                recordDecl->emitC(builder);
            } else if (auto* enumDecl = dynamic_cast<EnumDecl*>(namedDecl)) {
                enumDecl->emitC(builder);
            }
        }
    }
};

/* ==================== NAMED DECL ==================== */
std::string NamedDecl::emitDecl() const {
    return getDeclName().getName();
}

void NamedDecl::emitC(CodeBuilder * builder) const {
    builder->append(getDeclName().getName());
}

/* ==================== VALUE DECL ==================== */
std::string ValueDecl::emitDecl() const {
    return NamedDecl::emitDecl(); 
}

void ValueDecl::emitC(CodeBuilder * builder) const {
    auto name = getName();
    auto type = getType().getTypePtr();
    if (auto * constantArrayType = dynamic_cast<ConstantArrayType *>(type)) {
        auto elementType = constantArrayType->getElementType();
        auto length = *constantArrayType->getSize().getRawData();
        builder->append(absl::StrCat(elementType.emitType(), " ", name, "[", length, "]"));
    } else {
        builder->append(absl::StrCat(getType().emitType(), " ", name));
    }
}

/* ==================== DECLARATOR DECL ==================== */
std::string DeclaratorDecl::emitDecl() const {
    return ValueDecl::emitDecl();
}

void DeclaratorDecl::emitC(CodeBuilder * builder) const {
    ValueDecl::emitC(builder);
}

/* ==================== FUNCTION DECL ==================== */
FunctionDecl::FunctionDecl(Kind DK, ASTContext &C, DeclContext *DC, const DeclarationNameInfo &NameInfo, QualType T, StorageClass S)
    : DeclaratorDecl(DK, DC, NameInfo.getDeclarationName(), T), DeclContext(DK)
{
    FunctionDeclBits.SClass = S;
}

FunctionDecl * 
FunctionDecl::Create(ASTContext &C, DeclContext *DC, const DeclarationNameInfo &NameInfo, QualType T, StorageClass SC) {
    return new (C, DC) FunctionDecl(Function, C, DC, NameInfo, T, SC);
}

void FunctionDecl::Visit() {
    std::cout << "Return typr: ";
    getReturnType().print();
    std::cout << std::endl;
}

unsigned FunctionDecl::getNumParams() const {
    const auto FPT = dynamic_cast<FunctionProtoType*>(getType().getTypePtr());
    return FPT ? FPT->getNumParams() : 0;
}

void FunctionDecl::setParams(ASTContext &C, std::vector<ParmVarDecl *> NewParamInfo) {
    ParamInfo = NewParamInfo;
}

static bool isNamed(const NamedDecl *ND, const std::string &Str) {
    const IdentifierInfo *II = ND->getIdentifier();
    return II && II->getName() == Str;
}

bool FunctionDecl::isMain() const {
    return isNamed(this, "main");
}

std::string FunctionDecl::emitDecl() const {
    std::string FuncDecl;
    FuncDecl = absl::StrCat(FuncDecl, getReturnType().emitType(), " ");
    FuncDecl = absl::StrCat(FuncDecl, getName(), " ");
    FuncDecl = absl::StrCat(FuncDecl, "(");
    int NumParam = getNumParams();
    if (NumParam > 0) {
        auto params = getParmArray();
        for (int i = 0; i < NumParam; ++i) {
            FuncDecl = absl::StrCat(FuncDecl, params[i]->emitDecl());
            if (i < NumParam - 1) {
               FuncDecl = absl::StrCat(FuncDecl, ", ");
            }
        }
    } else {
        FuncDecl = absl::StrCat(FuncDecl, "void");
    }
    FuncDecl = absl::StrCat(FuncDecl, ")");
    std::cout << FuncDecl << std::endl;
    return FuncDecl;
}

void FunctionDecl::emitC(CodeBuilder * builder) const {
    builder->append(getReturnType().emitType());
    builder->append(" ");
    builder->append(getName());
    builder->append("(");

    int NumParam = getNumParams();
    if (NumParam > 0) {
        auto params = getParmArray();
        for (int i = 0; i < NumParam; ++i) {
            params[i]->emitC(builder);
            if (i < NumParam - 1) {
               builder->append(", ");
            }
        }
    } else {
        builder->append("void");
    }

    builder->append(")");
}

void FunctionDecl::emitBody(int level = 0) const {
    std::string body;
    body = absl::StrCat(body, "{\n");
    if (Body) body = absl::StrCat(body, Body->emitStmt(level));
    body = absl::StrCat(body, "}\n");
    std::cout << body << std::endl;
}

std::string FunctionDecl::getFunctionName() const {
    return DeclaratorDecl::getName();
}

std::string TypeDecl::emitDecl() const {
    return NamedDecl::emitDecl();
}

/* ==================== PARAMETER VAR DECL ==================== */
ParmVarDecl *
ParmVarDecl::Create(ASTContext &C, DeclContext *DC, IdentifierInfo *Id, QualType T, StorageClass S) {
    return new (C, DC) ParmVarDecl(ParmVar, C, DC, Id, T, S);
}

std::string ParmVarDecl::emitDecl() const {
    return VarDecl::emitDecl();
}

void ParmVarDecl::emitC(CodeBuilder * builder) const {
    return VarDecl::emitC(builder);
}

/* ==================== VAR DECL ==================== */
VarDecl::VarDecl(Kind DK, ASTContext &C, DeclContext *DC, IdentifierInfo *Id, QualType T, StorageClass SC) 
    : DeclaratorDecl(DK, DC, Id, T) {}

VarDecl *
VarDecl::Create(ASTContext &C, DeclContext *DC, IdentifierInfo *Id, QualType T, StorageClass S) {
    return new (C, DC) VarDecl(Var, C, DC, Id, T, S);
}

std::string VarDecl::emitDecl() const {
    return DeclaratorDecl::emitDecl();
}

void VarDecl::emitC(CodeBuilder * builder) const {
    return DeclaratorDecl::emitC(builder);
}

TagDecl::TagDecl(Kind DK, TagKind TK, const ASTContext &C, DeclContext *DC, IdentifierInfo *Id, TagDecl *PrevDecl)
    : TypeDecl(DK, DC, Id), DeclContext(DK) {
    setTagKind(TK);
}

std::string TagDecl::emitDecl() const {
    return TypeDecl::emitDecl();
}

/* ==================== FIELD DECL ==================== */
FieldDecl *
FieldDecl::Create(ASTContext &C, DeclContext *DC, IdentifierInfo *Id, QualType T, Expr *BW) {
    return new (C, DC) FieldDecl(Field, DC, Id, T, BW);
}

std::string FieldDecl::emitDecl() const {
    auto name = getName();
    auto type = getType().getTypePtr();

    if (auto * builtinType = dynamic_cast<BuiltinType *>(type)) {
        std::cout << builtinType->emitType() << " " << name << std::endl;
    } else if (auto * constantArrayType = dynamic_cast<ConstantArrayType *>(type)) {
        auto elementType = constantArrayType->getElementType();
        auto length = *constantArrayType->getSize().getRawData();
        std::cout << elementType.emitType() << " " << name << "[" << length << "]" << std::endl;
    } else if (auto * recordType = dynamic_cast<RecordType*>(type)) {
        auto decl = dynamic_cast<RecordDecl*>(recordType->getDecl());
        if (decl->isUnion()) {
            std::cout << recordType->emitType() << "{" << std::endl;
            for (auto unionField : decl->getFields()) {
                dynamic_cast<FieldDecl*>(unionField)->emitDecl();
            }
            std::cout << "}" << name << std::endl;
        } else {
            std::cout << recordType->emitType() << " " << name << std::endl;
        }
    } else if (auto * enumType = dynamic_cast<EnumType*>(type)) {
        std::cout << enumType->emitType() << " " << name << std::endl;
    } else {
        std::cout << "Other type " << std::endl;
    }

    if (BitWidth != nullptr) {
        std::cout << ":";
        if (auto * expr = dynamic_cast<IntegerLiteral *>(BitWidth)) {
            std::cout << expr->emitStmt();
        }
        std::cout << std::endl;
    }

    return "";
}

void FieldDecl::emitC(CodeBuilder * builder) {
    std::string field;
    auto name = getName();
    auto type = getType().getTypePtr();
    if (auto * builtinType = dynamic_cast<BuiltinType *>(type)) {
        builder->append(absl::StrCat(builtinType->emitType(), " ", name));
    } else if (auto * pointerType = dynamic_cast<PointerType *>(type)) {
        builder->append(absl::StrCat(pointerType->emitType(), " ", name));
    } else if (auto * constantArrayType = dynamic_cast<ConstantArrayType *>(type)) {
        auto elementType = constantArrayType->getElementType();
        auto length = *constantArrayType->getSize().getRawData();
        builder->append(absl::StrCat(elementType.emitType(), " ", name, "[", length, "]"));
    } else if (auto * recordType = dynamic_cast<RecordType*>(type)) {
        auto decl = dynamic_cast<RecordDecl*>(recordType->getDecl());
        if (decl->isUnion()) {
            builder->append(recordType->emitType().c_str());
            builder->blockStart();
            for (auto unionField : decl->getFields()) {
                builder->emitIndent();
                dynamic_cast<FieldDecl*>(unionField)->emitC(builder);
                builder->appendLine(";");
            }
            builder->blockEnd(false);
        } else {
            builder->append(absl::StrCat(recordType->emitType(), " ", name));
        }
    } else if (auto * enumType = dynamic_cast<EnumType*>(type)) {
        builder->append(absl::StrCat(enumType->emitType(), " ", name));
    } else {
        std::cout << "Other type " << std::endl;
    }

    if (BitWidth != nullptr) {
        if (auto * expr = dynamic_cast<IntegerLiteral *>(BitWidth)) {
            builder->append(absl::StrCat(field, ":", expr->emitStmt()));
        }
    }
}

/* ==================== RECORD DECL ==================== */
RecordDecl *
RecordDecl::Create(ASTContext &C, TagKind TK, DeclContext *DC, IdentifierInfo *Id, RecordDecl* PrevDecl) {
    return new (C, DC) RecordDecl(Record, TK, C, DC, Id, PrevDecl);
}

void RecordDecl::emitC(CodeBuilder * builder) {
    builder->append(getName().c_str());
    auto Fields = getFields();
    if (Fields.size() > 0) {
        builder->newline();
        builder->blockStart();

        for (const auto Field : Fields) {
            builder->emitIndent();
            dynamic_cast<FieldDecl*>(Field)->emitC(builder);
            builder->endOfStatement(true);
        }

        builder->blockEnd(false);
    }
    if (hasAttrs()) {
        auto attrs = getAttrs();
        builder->append(" __attribute__((");
        int NumAttr = attrs.size();
        for (int i = 0; i < NumAttr; ++i) {
            attrs[i]->emitC(builder);
            if (i < NumAttr - 1) {
                builder->append(",");
            }
        }
        builder->append("))");
    }
    builder->endOfStatement(true);
    builder->newline();
}

std::string RecordDecl::emitDecl() const {
    std::string type = TagDecl::getName();
    std::cout << type << std::endl;
    std::cout << "    {" << std::endl;

    for (const auto Field : getFields()) {
        dynamic_cast<FieldDecl*>(Field)->emitDecl();
    }

    std::cout << "    }" << std::endl;

    return TagDecl::emitDecl();
}

EnumDecl *EnumDecl::Create(ASTContext &C, DeclContext *DC, IdentifierInfo *Id, EnumDecl *PrevDecl) {
    return new (C, DC) EnumDecl(C, DC, Id, PrevDecl);
}

std::string EnumDecl::emitDecl() const {
    std::string type = TagDecl::getName();
    std::cout << type << std::endl;
    std::cout << "    {" << std::endl;

    for (const auto enumerator : getEnumerators()) {
        if (auto* valueDecl = dynamic_cast<ValueDecl*>(enumerator)) {
            std::cout << "    " << (dynamic_cast<ValueDecl*>(valueDecl))->emitDecl() << std::endl;
        }
    }

    std::cout << "    }" << std::endl;

    return TagDecl::emitDecl();
}

void EnumDecl::emitC(CodeBuilder * builder) {
    builder->appendLine(getName().c_str());
    builder->blockStart();

    for (const auto enumerator : getEnumerators()) {
        if (auto* enumConstantDecl = dynamic_cast<EnumConstantDecl*>(enumerator)) {
            enumConstantDecl->emitC(builder);
        }
    }

    builder->blockEnd(false);
    builder->endOfStatement(true);
    builder->newline();
}

EnumConstantDecl *EnumConstantDecl::Create(ASTContext &C, EnumDecl *DC, IdentifierInfo *Id, QualType T, Expr *E, const APSInt &V) {
    return new (C, DC) EnumConstantDecl(DC, Id, T, E, V);
}

void EnumConstantDecl::emitC(CodeBuilder * builder) {
    builder->emitIndent();
    builder->append(NamedDecl::getName());
    if (Init) {
        IntegerLiteral *initVal = dynamic_cast<IntegerLiteral *>(Init);
        builder->append("=");
        initVal->emitC(builder);
    }
    builder->appendLine(",");
}