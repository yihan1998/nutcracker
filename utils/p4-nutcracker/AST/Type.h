#ifndef _AST_TYPE_H_
#define _AST_TYPE_H_

#include <iostream>
#include <vector>

#include "absl/strings/str_cat.h"
#include "ADT/APInt.h"

/// The kind of a tag type.
enum TagTypeKind {
  /// The "struct" keyword.
  TTK_Struct,

  /// The "__interface" keyword.
  TTK_Interface,

  /// The "union" keyword.
  TTK_Union,

  /// The "class" keyword.
  TTK_Class,

  /// The "enum" keyword.
  TTK_Enum
};

/// The elaboration keyword that precedes a qualified type name or
/// introduces an elaborated-type-specifier.
enum ElaboratedTypeKeyword {
  /// The "struct" keyword introduces the elaborated-type-specifier.
  ETK_Struct,

  /// The "__interface" keyword introduces the elaborated-type-specifier.
  ETK_Interface,

  /// The "union" keyword introduces the elaborated-type-specifier.
  ETK_Union,

  /// The "class" keyword introduces the elaborated-type-specifier.
  ETK_Class,

  /// The "enum" keyword introduces the elaborated-type-specifier.
  ETK_Enum,

  /// The "typename" keyword precedes the qualified type name, e.g.,
  /// \c typename T::type.
  ETK_Typename,

  /// No keyword precedes the qualified type name.
  ETK_None
};

class Qualifiers {
public:
    enum TQ { // NOTE: These flags must be kept in sync with DeclSpec::TQ.
        Const    = 0x1,
        Restrict = 0x2,
        Volatile = 0x4,
        CVRMask = Const | Volatile | Restrict
    };
private:
    uint32_t Mask = 0;
};

class Type;
class TagDecl;
class RecordDecl;
class EnumDecl;

class QualType {
public:
    enum Qualifiers {
        Const = 1 << 0,
        Volatile = 1 << 1
    };

    QualType(Type *type = nullptr, unsigned qualifiers = 0)
        : type(type), qualifiers(qualifiers) {}

    void print() const;

    std::string emitType() const;

    bool isConstQualified() const {
        return qualifiers & Const;
    }

    bool isVolatileQualified() const {
        return qualifiers & Volatile;
    }

    Type* getTypePtr() const { return type; }

    unsigned getTypeWidth() const;

private:
    Type *type;
    unsigned qualifiers;
};

class Type {
public:
    enum TypeClass {
        Builtin,
        Pointer,
        Record,
        Enum,
        Array,
        ConstantArray,
        Function,
        FunctionProto,
    };

    class TypeBitfields {
        friend class Type;
        unsigned TC : 8;
    };
    enum { NumTypeBits = 8 };

    class ArrayTypeBitfields {
        unsigned : NumTypeBits;
        unsigned IndexTypeQuals : 3;
        unsigned SizeModifier : 3;
    };

    class BuiltinTypeBitfields {
        friend class BuiltinType;
        unsigned : NumTypeBits;
        unsigned Kind : 8;
    };

    class FunctionTypeBitfields {
        friend class FunctionProtoType;
        friend class FunctionType;
        unsigned : NumTypeBits;
        unsigned NumParams : 16;
    };

    union {
        TypeBitfields TypeBits;
        ArrayTypeBitfields ArrayTypeBits;
        BuiltinTypeBitfields BuiltinTypeBits;
        FunctionTypeBitfields FunctionTypeBits;
    };

    Type(TypeClass tc) {
        TypeBits.TC = tc;
    }

    virtual ~Type() = default;
    virtual void print() const = 0;
    virtual std::string emitType() const = 0;
    virtual unsigned getWidth() const = 0;
};

class TagType : public Type {
    TagDecl *decl;

protected:
    TagType(TypeClass TC, const TagDecl *D)
        : Type(TC), decl(const_cast<TagDecl *>(D)) {}

public:
    TagDecl *getDecl() const { return decl; }

    std::string emitType() const;
    void print() const override {
        std::cout << "TAG" << std::endl;
    }
    unsigned getWidth() const { return sizeof(void *); };
};

class RecordType : public TagType {
protected:
    friend class ASTContext; // ASTContext creates these.

public:
    RecordType(const RecordDecl *D);
    RecordType(TypeClass TC, RecordDecl *D);

    RecordDecl *getDecl() const;

    std::string emitType() const;
    void print() const override;
    unsigned getWidth() const { return sizeof(void *); };
};

class EnumType : public TagType {
    friend class ASTContext; // ASTContext creates these.

public:
    EnumType(const EnumDecl *D);

    EnumDecl *getDecl() const;

    std::string emitType() const;
    void print() const override;
    unsigned getWidth() const { return sizeof(void *); };
};

class ArrayType : public Type {
private:
    QualType ElementType;

public:
    ArrayType(TypeClass tc, QualType et) : Type(tc), ElementType(et) {};

    QualType getElementType() const { return ElementType; }

    std::string emitType() const override;
    void print() const override;
    unsigned getWidth() const { return 0; };
};

class ConstantArrayType : public ArrayType {
    APInt Size;

public:
    ConstantArrayType(QualType et, const APInt &size) : ArrayType(ConstantArray, et), Size(size) {}

    const APInt &getSize() const { return Size; }
    std::string emitElementType() const;
    std::string emitType() const override;
    void print() const override;
    unsigned getWidth() const { return Size.getNumWords(); };
};

class PointerType : public Type {
    QualType PointeeType;

public:
    PointerType(QualType Pointee) : Type(Pointer), PointeeType(Pointee) {}
    QualType getPointeeType() const { return PointeeType; }

    void print() const override {
        PointeeType.print();
        std::cout << "*";
    }

    std::string emitType() const {
        return absl::StrCat(PointeeType.emitType(), "*");
    }

    unsigned getWidth() const { return sizeof(void *); };
};

class FunctionType : public Type {
    QualType ResultType;
protected:
    FunctionType(TypeClass tc, QualType res) : Type(tc), ResultType(res) {}
public:
    QualType getReturnType() const { return ResultType; }
    void print() const {}
    std::string emitType() const { return nullptr; }
};

class FunctionProtoType : public FunctionType {
protected:
    std::vector<QualType> args;
private:
    FunctionProtoType(QualType result, std::vector<QualType> &args);
public:
    static Type* CreateFunctionProtoType(QualType result, std::vector<QualType> &args) { 
        return new FunctionProtoType(result, args);
    }
    unsigned getNumParams() const { return FunctionTypeBits.NumParams; }
    void print() const {}
    unsigned getWidth() const { return sizeof(void *); };
};

class BuiltinType : public Type {
public:
    enum Kind {
        Void,
        Bool,
        Char,
        Short,
        Int,
        UChar,
        UShort,
        UInt,
    };

private:
    Kind K;

public:
    BuiltinType(Kind K) : Type(Builtin), K(K) {
        BuiltinTypeBits.Kind = K;
    }

    Kind getKind() const { return K; }

    void print() const override {
        switch (K) {
            case Void: std::cout << "void"; break;
            case Bool: std::cout << "bool"; break;
            case Char: std::cout << "char"; break;
            case Short: std::cout << "short"; break;
            case Int: std::cout << "int"; break;
            case UChar: std::cout << "uint8_t"; break;
            case UShort: std::cout << "uint16_t"; break;
            case UInt: std::cout << "uint32_t"; break;
        }
    }

    std::string emitType() const {
        std::string result;
        switch (K) {
            case Void: result = "void"; break;
            case Bool: result = "bool"; break;
            case Char: result = "char"; break;
            case Short: result = "short"; break;
            case Int: result = "int"; break;
            case UChar: result = "uint8_t"; break;
            case UShort: result = "uint16_t"; break;
            case UInt: result = "uint32_t"; break;
        }
        return result;
    }

    unsigned getWidth() const {
        unsigned width = 0;
        switch (K) {
            case Void: width = 0; break;
            case Bool: width = sizeof(bool); break;
            case Char: width = sizeof(char); break;
            case Short: width = sizeof(short); break;
            case Int: width = sizeof(int); break;
            case UChar: width = sizeof(uint8_t); break;
            case UShort: width = sizeof(uint16_t); break;
            case UInt: width = sizeof(uint32_t); break;
        }
        return width; 
    };

    static bool classof(const Type *T) {
        return dynamic_cast<const BuiltinType*>(T) != nullptr;
    }
};

template <typename T = Type>
class CanQualType {
    QualType Stored;

public:
    CanQualType() = default;

    template <typename U>
    CanQualType(const CanQualType<U> &Other, std::enable_if_t<std::is_base_of<T, U>::value, int> = 0)
        : Stored(Other.getStored()) {}

    CanQualType(QualType qt) : Stored(qt) {}

    const T *getTypePtr() const {
        return dynamic_cast<const T*>(Stored.getTypePtr());
    }

    void print() const {
        Stored.print();
    }

private:
    const QualType& getStored() const {
        return Stored;
    }
};

#endif  /* _AST_TYPE_H_ */