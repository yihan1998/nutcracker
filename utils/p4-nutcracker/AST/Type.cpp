#include "Type.h"
#include "Decl.h"

void QualType::print() const {
    if (qualifiers & Const) std::cout << "const ";
    if (qualifiers & Volatile) std::cout << "volatile ";
    if (type) type->print();
}

std::string QualType::emitType() const {
    std::string result;
    if (qualifiers & Const) result = "const ";
    if (qualifiers & Volatile) result = "volatile ";
    if (type) result += type->emitType();
    return result;
}

unsigned QualType::getTypeWidth() const { 
    return type->getWidth(); 
}

FunctionProtoType::FunctionProtoType(QualType result, std::vector<QualType> &args)
    : FunctionType(FunctionProto, result), args(args) {
    FunctionTypeBits.NumParams = args.size();
}

std::string TagType::emitType() const { return decl->getName(); }

RecordType::RecordType(const RecordDecl *D) 
    : TagType(Record, dynamic_cast<const TagDecl*>(D)) {}
RecordType::RecordType(TypeClass TC, RecordDecl *D) 
    : TagType(TC, dynamic_cast<const TagDecl*>(D)) {}

RecordDecl *RecordType::getDecl() const {
    return dynamic_cast<RecordDecl*>(TagType::getDecl());
}

std::string RecordType::emitType() const { return getDecl()->getName(); }

void RecordType::print() const {
    std::cout << getDecl()->getName() << std::endl;
}

EnumType::EnumType(const EnumDecl *D) 
    : TagType(Enum, dynamic_cast<const TagDecl*>(D)) {}

EnumDecl *EnumType::getDecl() const {
    return dynamic_cast<EnumDecl*>(TagType::getDecl());
}

std::string EnumType::emitType() const {
    return getDecl()->getName();
}

void EnumType::print() const { return; }

std::string ArrayType::emitType() const {
    return ElementType.emitType();
}

void ArrayType::print() const {
    std::cout << emitType() << std::endl;
}

std::string ConstantArrayType::emitElementType() const { return ArrayType::emitType(); }

std::string ConstantArrayType::emitType() const {
    return absl::StrCat(ArrayType::emitType(), "[", Size.toString(10, false), "]");
}

void ConstantArrayType::print() const {
    std::cout << emitType() << "[" << Size.getRawData() << "]" << std::endl;
}