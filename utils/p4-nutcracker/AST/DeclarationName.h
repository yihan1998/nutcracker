#ifndef _AST_DECLARATIONNAME_H_
#define _AST_DECLARATIONNAME_H_

#include "Basic/IdentifierTable.h"
#include "Type.h"

class DeclarationName {
    friend class NamedDecl;
public:
    DeclarationName(IdentifierInfo *idInfo) : idInfo(idInfo) {}

    const IdentifierInfo* getAsIdentifierInfo() const {
        return idInfo;
    }

    void print() const {
        if (idInfo) {
            std::cout << idInfo->getName();
        }
    }

    std::string getName() const { return idInfo->getName(); }

private:
    IdentifierInfo *idInfo;
};


class DeclarationNameInfo {
public:
    DeclarationNameInfo(DeclarationName name) : name(name) {}

    const DeclarationName& getDeclarationName() const { return name; }
    std::string getName() const { return name.getName(); }

    void print() const {
        name.print();
    }

private:
    DeclarationName name;
};

#endif  /* _AST_DECLARATIONNAME_H_ */