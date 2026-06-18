#pragma once

#include <cstddef>
#include <ostream>
#include <vector>
#include <cstdint>

#include "util/assert.hpp"
#include "util/iterator_range.hpp"

namespace ir {
class LLVMContext;

class Type {
   public:
    enum TypeId {
        VoidTyID,   ///< type with no size
        LabelTyID,  ///< Labels

        IntegerTyID,   ///< Arbitrary bit width integers
        FunctionTyID,  ///< Functions
        PointerTyID,   ///< Pointers
        ArrayTyID,     ///< Arrays
    };

   private:
    LLVMContext& context;
    TypeId       id;

   protected:
    friend class LLVMContextImpl;

    explicit Type(LLVMContext& context, TypeId tid)
        : context(context), id(tid) {}

    uint64_t           subclassData = 0;
    std::vector<Type*> containedTys;

   public:
    // virtual ~Type() = default;
    LLVMContext& getContext() const { return context; }
    TypeId       getTypeID() const { return id; }

    bool isVoidTy() const { return getTypeID() == VoidTyID; }
    bool isLabelTy() const { return getTypeID() == LabelTyID; }
    bool isIntegerTy() const { return getTypeID() == IntegerTyID; }
    bool isIntegerTy(unsigned bitWidth) const {
        return getTypeID() == IntegerTyID && subclassData == bitWidth;
    }
    bool isFunctionTy() const { return getTypeID() == FunctionTyID; }
    bool isArrayTy() const { return getTypeID() == ArrayTyID; }
    bool isPointerTy() const { return getTypeID() == PointerTyID; }

    static Type* getVoidTy(LLVMContext& c);
    static Type* getLabelTy(LLVMContext& c);
};

class IntegerType : public Type {
    friend class LLVMContextImpl;
    explicit IntegerType(LLVMContext& c,
                         unsigned     numBits);  // currently NumBits = 32

   public:
    static IntegerType* get(LLVMContext& c, unsigned NumBits);
    unsigned            getBitWidth() const { return subclassData; }
};
static_assert(sizeof(Type) == sizeof(IntegerType));

class FunctionType : public Type {
    friend class LLVMContextImpl;
    explicit FunctionType(Type* returnType, const std::vector<Type*>& params,
                          bool isVarArgs);

   public:
    FunctionType(const FunctionType&)            = delete;
    FunctionType& operator=(const FunctionType&) = delete;

    static FunctionType* get(
        Type*                     returnType,  // support: int | int*
        const std::vector<Type*>& params,      // support: int | void
        bool                      isVarArg     // unuse
    );

    // const std::vector<Type*> params() const { return containedTypes; }
    using param_iterator = decltype(containedTys.cbegin());
    using param_range    = iterator_range<param_iterator>;
    param_iterator param_begin() const { return containedTys.cbegin() + 1; }
    param_iterator param_end() const { return containedTys.cend(); }
    param_range    params() const {
        return param_range{param_begin(), param_end()};
    }
    Type* getParamType(size_t i) const {
        ASSERT(i < getNumParams());
        return containedTys[i + 1];
    }
    size_t getNumParams() const { return containedTys.size() - 1; }

    Type* getReturnType() const { return containedTys[0]; }
};
static_assert(sizeof(Type) == sizeof(FunctionType));

class ArrayType : public Type {
    friend class LLVMContextImpl;
    explicit ArrayType(Type* elemType, size_t elemNum);

   public:
    static ArrayType* get(Type*  elemType,  // support: int
                          size_t elemNum);

    size_t getNumElements() const { return subclassData; }
    Type*  getElementType() const { return containedTys[0]; }
};
static_assert(sizeof(Type) == sizeof(ArrayType));

class PointerType : public Type {
    friend class LLVMContextImpl;
    explicit PointerType(Type* pointTy, unsigned addrSpace);

   public:
    PointerType(const PointerType&)            = delete;
    PointerType& operator=(const PointerType&) = delete;

    static PointerType* get(Type* pointeeTy, unsigned addrSpace);

    Type* getPointeeType() const { return containedTys[0]; }
};

std::ostream& operator<<(std::ostream& os, const Type* type);
}  // namespace ir
