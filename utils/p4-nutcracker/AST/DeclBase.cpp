#include "Decl.h"
#include "DeclBase.h"

DeclContext::DeclContext(Decl::Kind K) {
    DeclContextBits.DeclKind = K;
}

void DeclContext::emitDeclContext() const {
    // for (const auto& pair : DeclsMap) {
    //     // const std::string& key = pair.first;
    //     Decl* decl = pair.second;

    //     if (auto* tuDecl = dynamic_cast<TranslationUnitDecl*>(decl)) {
    //         // std::cout << "TranslationUnitDecl" << std::endl;
    //         // tuDecl->emit();
    //     } else if (auto* namedDecl = dynamic_cast<NamedDecl*>(decl)) {
    //         if (auto* functionDecl = dynamic_cast<FunctionDecl*>(namedDecl)) {
    //             if (functionDecl->isMain()) continue;
    //         }
    //         namedDecl->emitDecl();
    //     } else {
    //         std::cout << __func__ << ":" << __LINE__ << "Unknown Decl type" << std::endl;
    //     }
    // }
    // for (const auto& pair : DeclsMap) {
    for (auto it = DeclsMap.rbegin(); it != DeclsMap.rend(); ++it) {
        // const std::string& key = it->first;
        Decl* decl = it->second;

        if (auto* functionDecl = dynamic_cast<FunctionDecl*>(decl)) {
            functionDecl->emitDecl();
            functionDecl->emitBody(1);
            std::cout << std::endl;
        } else {
            std::cout << __func__ << ":" << __LINE__ << "Unknown Decl type" << std::endl;
        }
    }
}

void DeclContext::emitDeclContext(CodeBuilder *builder) const {
    // for (auto it = DeclsMap.rbegin(); it != DeclsMap.rend(); ++it) {
    //     const std::string& key = it->first;
    //     Decl* decl = it->second;

    //     if (auto* tuDecl = dynamic_cast<TranslationUnitDecl*>(decl)) {
    //         std::cout << "Emit TranslationUnitDecl" << std::endl;
    //         tuDecl->emitC(builder);
    //     } else if (auto* namedDecl = dynamic_cast<NamedDecl*>(decl)) {
    //         if (auto* functionDecl = dynamic_cast<FunctionDecl*>(namedDecl)) {
    //             if (functionDecl->isMain()) continue;
    //             namedDecl->emitC(builder);
    //         } else if (auto* recordDecl = dynamic_cast<RecordDecl*>(namedDecl)) {
    //             recordDecl->emitC(builder);
    //         } else if (auto* enumDecl = dynamic_cast<EnumDecl*>(namedDecl)) {
    //             enumDecl->emitC(builder);
    //         }
    //     } else {
    //         std::cout << __func__ << ":" << __LINE__ << "Unknown Decl type" << std::endl;
    //     }
    // }
    for (const auto decl : Decls) {
        if (auto* namedDecl = dynamic_cast<NamedDecl*>(decl)) {
            if (auto* functionDecl = dynamic_cast<FunctionDecl*>(namedDecl)) {
                if (functionDecl->isMain()) continue;
                if (functionDecl->isExternC()) builder->append("extern ");
                namedDecl->emitC(builder);
                builder->endOfStatement(true);
            } else if (auto* recordDecl = dynamic_cast<RecordDecl*>(namedDecl)) {
                recordDecl->emitC(builder);
            } else if (auto* enumDecl = dynamic_cast<EnumDecl*>(namedDecl)) {
                enumDecl->emitC(builder);
            }
        } else {
            std::cout << __func__ << ":" << __LINE__ << "Unknown Decl type" << std::endl;
        }
    }

    builder->newline();

    for (const auto& pair : DeclsMap) {
        const std::string& key = pair.first;
        Decl* decl = pair.second;

        if (auto* functionDecl = dynamic_cast<FunctionDecl*>(decl)) {
            if (functionDecl->isExternC()) continue;
            functionDecl->emitC(builder);
            builder->newline();
            functionDecl->getBody()->emitC(builder);
            builder->newline();
        }
    }
}

void DeclContext::addHiddenDecl(Decl *D) {
    if (auto *Record = dynamic_cast<RecordDecl*>(this)) {
        Record->addedMember(D);
    } else if (auto *Record = dynamic_cast<EnumDecl*>(this)) {
        Record->addedEnumerators(D);
    }
}

void DeclContext::addDecl(Decl *D) {
    addHiddenDecl(D);

    if (auto *ND = dynamic_cast<NamedDecl*>(D)) {
        ND->getDeclContext()->getPrimaryContext()->makeDeclVisible(ND);
    }
}

DeclContext *DeclContext::getPrimaryContext() {
    switch (getDeclKind()) {
        case Decl::Function:
        case Decl::ExternCContext:
            return this;

        case Decl::TranslationUnit:
            // return static_cast<TranslationUnitDecl *>(this)->getFirstDecl();
        default:
            return this;
    }
    return this;
}

void DeclContext::makeDeclVisible(NamedDecl *D) {
    // DeclsMap[D->getDeclName().getAsIdentifierInfo()->getName()] = D;
    DeclsMap[D->getDeclName().getAsIdentifierInfo()->getName()] = D;
    Decls.push_back(D);
}