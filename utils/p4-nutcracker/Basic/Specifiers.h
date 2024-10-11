#ifndef _BASIC_SPECIFIERS_H_
#define _BASIC_SPECIFIERS_H_

enum StorageClass {
    // These are legal on both functions and variables.
    SC_None,
    SC_Extern,
    SC_Static,
    SC_PrivateExtern,

    // These are only legal on variables.
    SC_Auto,
    SC_Register
};

enum ExprValueKind {
    /// A pr-value expression (in the C++11 taxonomy)
    /// produces a temporary value.
    VK_PRValue,

    /// An l-value expression is a reference to an object with
    /// independent storage.
    VK_LValue,

    /// An x-value expression is a reference to an object with
    /// independent storage but which can be "moved", i.e.
    /// efficiently cannibalized for its resources.
    VK_XValue
};

enum ExprObjectKind {
    /// An ordinary object is located at an address in memory.
    OK_Ordinary,

    /// A bitfield object is a bitfield on a C or C++ record.
    OK_BitField,

    /// A vector component is an element or range of elements on a vector.
    OK_VectorComponent,

    /// An Objective-C property is a logical field of an Objective-C
    /// object which is read and written via Objective-C method calls.
    OK_ObjCProperty,

    /// An Objective-C array/dictionary subscripting which reads an
    /// object or writes at the subscripted array/dictionary element via
    /// Objective-C method calls.
    OK_ObjCSubscript,

    /// A matrix component is a single element of a matrix.
    OK_MatrixComponent
};
  
#endif  /* _BASIC_SPECIFIERS_H_ */