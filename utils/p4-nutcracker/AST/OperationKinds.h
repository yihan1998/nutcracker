#ifndef _AST_OPERATIONKINDS_H_
#define _AST_OPERATIONKINDS_H_

enum BinaryOperatorKind {
    BO_PtrMemD,
    BO_PtrMemI,
    BO_Mul,
    BO_Div,
    BO_Rem,
    BO_Add,
    BO_Sub,
    BO_Assign,
};

enum UnaryOperatorKind {
    UO_AddrOf,
};

#endif  /* _AST_OPERATIONKINDS_H_ */