#include "evaluate.hpp"

#include <algorithm>
#include <memory>
#include <optional>

#include "ir/BasicBlock.hpp"
#include "ir/Function.hpp"
#include "ir/Instructions.hpp"
#include "ir/IRBuilder.hpp"
#include "ir/Value.hpp"
#include "symbol_table.hpp"
#include "util/assert.hpp"

ir::Value* IntVarExp::rvalue(ir::IRBuilder& builder) {
    if (auto val = test_constexpr()) {
        return builder.getInt32(*val);
    } else {
        ir::Value* addr_value = var_symbol.attr.addr_value;
        return builder.CreateLoad(builder.getInt32Ty(), addr_value,
                                  "load." + var_symbol.name);
    }
}

ir::Value* IntVarExp::lvalue([[maybe_unused]] ir::IRBuilder& builder) {
    return var_symbol.attr.addr_value;
}

std::optional<int32_t> IntVarExp::test_constexpr() {
    if (var_symbol.attr.is_const()) {
        ASSERT(var_symbol.attr.const_values.size() == 1);
        return var_symbol.attr.const_values[0];
    } else {
        return std::nullopt;
    }
}

ir::Value* PtrVarExp::rvalue([[maybe_unused]] ir::IRBuilder& builder) {
    return var_symbol.attr.addr_value;
}

ir::Value* FuncCallExp::rvalue(ir::IRBuilder& builder) {
    auto* function_value =
        dynamic_cast<ir::Function*>(func_symbol.attr.addr_value);
    ASSERT(function_value);
    // return void 函数不能有 name
    std::string call_name;
    if (type() != T_VOID) {
        call_name = "call." + func_symbol.name;
    }

    std::vector<ir::Value*> args_value;
    std::transform(params.begin(), params.end(), std::back_inserter(args_value),
                   [&builder](const std::unique_ptr<Exp>& param) {
                       return param->rvalue(builder);
                   });

    return builder.CreateCall(function_value->getFunctionType(), function_value,
                              args_value, call_name);
}

Exp::Type FuncCallExp::type() {
    if (func_symbol.attr.type.base_type == SymbolBaseType::VOID) {
        return T_VOID;
    } else {
        return T_INT;
    }
}

ir::Value* ArrayAccessExp::rvalue(ir::IRBuilder& builder) {
    if (auto test = test_constexpr()) {
        return builder.getInt32(*test);
    }
    ir::Value* attr = lvalue(builder);
    return builder.CreateLoad(builder.getInt32Ty(), attr,
                              "load." + record.name);
}

ir::Value* ArrayAccessExp::lvalue(ir::IRBuilder& builder) {
    ir::Value* index_val = index->rvalue(builder);
    ir::Value* addr_val  = record.attr.addr_value;

    return builder.CreateGEP(builder.getInt32Ty(), addr_val, {index_val},
                             "gep." + record.name);
}

std::optional<int32_t> ArrayAccessExp::test_constexpr() {
    if (record.attr.is_const()) {
        if (auto ind = index->test_constexpr()) {
            size_t i = *ind;
            if (i < record.attr.const_values.size())
                return record.attr.const_values.at(i);
        }
    }
    return std::nullopt;
}

ir::Value* BinaryOpIntExp::rvalue(ir::IRBuilder& builder) {
    if (auto val = test_constexpr()) {
        return builder.getInt32(*val);
    }

    ir::Value* l = lhs->rvalue(builder);
    ir::Value* r = rhs->rvalue(builder);

    switch (op) {
        case Token::Type::PLUS:  // +
            return builder.CreateNSWAdd(l, r, "add");
        case Token::Type::MINU:  // -
            return builder.CreateNSWSub(l, r, "sub");
        case Token::Type::MULT:  // *
            return builder.CreateNSWMul(l, r, "mul");
        case Token::Type::DIV:  // /
            return builder.CreateSDiv(l, r, "div");
        case Token::Type::MOD: {  // %
            return builder.CreateSRem(l, r, "mod");
        }
        case Token::Type::LSS: {  // <
            auto* v = builder.CreateICmpSLT(l, r, "slt");
            return builder.CreateZExt(v, builder.getInt32Ty(), "ext");
        }
        case Token::Type::LEQ: {  // <=
            auto* v = builder.CreateICmpSLE(l, r, "sle");
            return builder.CreateZExt(v, builder.getInt32Ty(), "ext");
        }
        case Token::Type::GRE: {  // >
            auto* v = builder.CreateICmpSGT(l, r, "sgt");
            return builder.CreateZExt(v, builder.getInt32Ty(), "ext");
        }
        case Token::Type::GEQ: {  // >=
            auto* v = builder.CreateICmpSGE(l, r, "sge");
            return builder.CreateZExt(v, builder.getInt32Ty(), "ext");
        }
        case Token::Type::EQL: {  // ==
            auto* v = builder.CreateICmpEQ(l, r, "eq");
            return builder.CreateZExt(v, builder.getInt32Ty(), "ext");
        }
        case Token::Type::NEQ: {  // !=
            auto* v = builder.CreateICmpNE(l, r, "ne");
            return builder.CreateZExt(v, builder.getInt32Ty(), "ext");
        }
            DEFAULT_UNREACHABLE();
    }
}

std::optional<int32_t> BinaryOpIntExp::test_constexpr() {
    if (auto lval = lhs->test_constexpr(), rval = rhs->test_constexpr();
        lval && rval) {
        switch (op) {
            case Token::Type::PLUS:  // +
                return *lval + *rval;
            case Token::Type::MINU:  // -
                return *lval - *rval;
            case Token::Type::MULT:  // *
                return *lval * *rval;
            case Token::Type::DIV:  // /
                return *lval / *rval;
            case Token::Type::MOD:  // %
                return *lval % *rval;
            case Token::Type::LSS:  // <
                return *lval < *rval;
            case Token::Type::LEQ:  // <=
                return *lval <= *rval;
            case Token::Type::GRE:  // >
                return *lval > *rval;
            case Token::Type::GEQ:  // >=
                return *lval >= *rval;
            case Token::Type::EQL:  // ==
                return *lval == *rval;
            case Token::Type::NEQ:  // !=
                return *lval != *rval;
            default:
                UNREACHABLE();
        }
    }
    return std::nullopt;
}

ir::Value* UnaryExp::rvalue(ir::IRBuilder& builder) {
    if (auto val = test_constexpr()) {
        return builder.getInt32(*val);
    }

    ir::Value* val = exp->rvalue(builder);
    switch (op) {
        case Token::Type::PLUS:
            return val;
        case Token::Type::MINU:
            return builder.CreateNSWNeg(val, "neg");
        case Token::Type::NOT:
            val = builder.CreateICmpEQ(val, builder.getInt32(0), "cmp0");
            return builder.CreateZExt(val, builder.getInt32Ty(), "ext");
        default:
            UNREACHABLE();
    }
}

std::optional<int32_t> UnaryExp::test_constexpr() {
    if (auto val = exp->test_constexpr()) {
        switch (op) {
            case Token::Type::PLUS:
                return *val;
            case Token::Type::MINU:
                return -*val;
            case Token::Type::NOT:
                return !*val;
            default:
                UNREACHABLE();
        }
    }
    return std::nullopt;
}

// Conditions
void SingleCond::gen_code(ir::BasicBlock* true_bb, ir::BasicBlock* false_bb,
                          ir::IRBuilder& builder) {
    ir::Value* llvm_value = exp->rvalue(builder);
    llvm_value =
        builder.CreateICmpNE(llvm_value, builder.getInt32(0), "tobool");
    builder.CreateCondBr(llvm_value, true_bb, false_bb);
}

void LAndCond::gen_code(ir::BasicBlock* true_bb, ir::BasicBlock* false_bb,
                        ir::IRBuilder& builder) {
    ir::BasicBlock* next_bb = ir::BasicBlock::Create(
        builder.getContext(), "land.next", builder.GetInsertBlock()->parent());
    lhs->gen_code(next_bb, false_bb, builder);
    builder.SetInsertPoint(next_bb);
    rhs->gen_code(true_bb, false_bb, builder);
}

void LOrCond::gen_code(ir::BasicBlock* true_bb, ir::BasicBlock* false_bb,
                       ir::IRBuilder& builder) {
    ir::BasicBlock* next_bb = ir::BasicBlock::Create(
        builder.getContext(), "lor.next", builder.GetInsertBlock()->parent());
    lhs->gen_code(true_bb, next_bb, builder);
    builder.SetInsertPoint(next_bb);
    rhs->gen_code(true_bb, false_bb, builder);
}
