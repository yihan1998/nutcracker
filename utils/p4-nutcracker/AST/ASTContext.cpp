#include "ASTContext.h"
#include "Type.h"

QualType ASTContext::getFunctionTypeInternal(QualType ResultTy, std::vector<QualType> &ArgArray) const {
    auto FTP = FunctionProtoType::CreateFunctionProtoType(ResultTy, ArgArray);
    return QualType(FTP);
}

QualType ASTContext::getPointerType(QualType T) const {
    auto New = new PointerType(T);
    return QualType(New);
}