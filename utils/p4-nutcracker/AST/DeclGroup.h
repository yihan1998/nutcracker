#ifndef _AST_DECLGROUP_H_
#define _AST_DECLGROUP_H_

#include <vector>

class Decl;

class DeclGroupRef {
    Decl *D = nullptr;

public:
    DeclGroupRef(Decl* d) : D(d) {}
    Decl *getDecl() const { return D; }
};

#endif  /* _AST_DECLGROUP_H_ */