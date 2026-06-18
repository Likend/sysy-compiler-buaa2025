#include "grammer.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "error.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "util/assert.hpp"
#include "util/lambda_overload.hpp"

[[nodiscard]] std::string_view astnode_type_name(ASTNode::Type type) {
    switch (type) {
        case ASTNode::Type::COMP_UNIT:
            return "CompUnit";
        case ASTNode::Type::DECL:
            return "Decl";
        case ASTNode::Type::CONST_DECL:
            return "ConstDecl";
        case ASTNode::Type::CONST_DEF:
            return "ConstDef";
        case ASTNode::Type::CONST_INIT_VAL:
            return "ConstInitVal";
        case ASTNode::Type::VAR_DECL:
            return "VarDecl";
        case ASTNode::Type::VAR_DEF:
            return "VarDef";
        case ASTNode::Type::INIT_VAL:
            return "InitVal";
        case ASTNode::Type::FUNC_DEF:
            return "FuncDef";
        case ASTNode::Type::MAIN_FUNC_DEF:
            return "MainFuncDef";
        case ASTNode::Type::FUNC_TYPE:
            return "FuncType";
        case ASTNode::Type::FUNC_PARAMS:
            return "FuncFParams";
        case ASTNode::Type::FUNC_PARAM:
            return "FuncFParam";
        case ASTNode::Type::BLOCK:
            return "Block";
        case ASTNode::Type::STMT:
            return "Stmt";
        case ASTNode::Type::FOR_STMT:
            return "ForStmt";
        case ASTNode::Type::EXP:
            return "Exp";
        case ASTNode::Type::COND:
            return "Cond";
        case ASTNode::Type::L_VAL:
            return "LVal";
        case ASTNode::Type::PRIMARY_EXP:
            return "PrimaryExp";
        case ASTNode::Type::NUMBER:
            return "Number";
        case ASTNode::Type::UNARY_EXP:
            return "UnaryExp";
        case ASTNode::Type::UNARY_OP:
            return "UnaryOp";
        case ASTNode::Type::FUNC_RPARAMS:
            return "FuncRParams";
        case ASTNode::Type::MUL_EXP:
            return "MulExp";
        case ASTNode::Type::ADD_EXP:
            return "AddExp";
        case ASTNode::Type::REL_EXP:
            return "RelExp";
        case ASTNode::Type::EQ_EXP:
            return "EqExp";
        case ASTNode::Type::LAND_EXP:
            return "LAndExp";
        case ASTNode::Type::LOR_EXP:
            return "LOrExp";
        case ASTNode::Type::CONST_EXP:
            return "ConstExp";
    }
    UNREACHABLE();
}

std::ostream& operator<<(std::ostream& os, ASTNode::Type type) {
    os << astnode_type_name(type);
    return os;
}

std::ostream& operator<<(std::ostream& os, const ASTNode& node) {
    auto visitor = overloaded{
        [&os](const Token& token) {
            os << token.type << ' ' << token.content << std::endl;
        },
        [&os](const std::unique_ptr<ASTNode>& ast_node) { os << *ast_node; }};

#ifdef NDEBUG
    switch (node.type) {
        // binary operations
        case ASTNode::Type::MUL_EXP:
        case ASTNode::Type::ADD_EXP:
        case ASTNode::Type::REL_EXP:
        case ASTNode::Type::EQ_EXP:
        case ASTNode::Type::LAND_EXP:
        case ASTNode::Type::LOR_EXP: {
            bool odd_flag = true;
            for (const auto& i : node.children) {
                std::visit(visitor, i);
                if (odd_flag) os << '<' << node.type << '>' << std::endl;

                odd_flag = !odd_flag;
            }
            break;
        }
        default:
            for (const auto& i : node.children) {
                std::visit(visitor, i);
            }
            os << '<' << node.type << '>' << std::endl;
    }
    return os;
#else
    static uint32_t ident = 0;
    os << '<' << node.type << '>' << std::endl;
    ident++;
    for (const auto& i : node.children) {
        for (size_t i = 0; i < ident; i++) {
            os << '\t';
        }
        std::visit(visitor, i);
    }
    ident--;
    return os;
#endif
}

using Element = ASTNode::Element;
using Map     = ASTNode::Map;

struct ParserContext {
    Lexer::iterator& it;
    Map&             container;
    Token&           last_token;
    bool             strict;

    struct Marker {
        Lexer::iterator  it;
        Map::ScopeMarker map_marker;
        Token            token_packup;
    };

    Marker save_marker() {
        return {it, container.get_scope_marker(), last_token};
    }

    void reserve_marker(const Marker& marker) {
        it = marker.it;
        container.pop_scope(marker.map_marker);
        last_token = marker.token_packup;
    }

    ParserContext update_container(Map& new_container) const {
        return {it, new_container, last_token, strict};
    }

    ParserContext update_strict(bool new_strict) const {
        return {it, container, last_token, new_strict};
    }

    ParserContext update_token(Token& new_token) const {
        return {it, container, new_token, strict};
    }
};

template <ASTNode::Type type>
struct ParseNode {};

template <Token::Type type>
struct ParseToken {
    bool operator()(ParserContext bundle) {
        Token token = *bundle.it;
        if (token.type == type) {
            bundle.container.insert(token);
            ++bundle.it;
            bundle.last_token = token;
            return true;
        }
        if (token.type == Token::Type::ERROR) {
            if ((*token.content.data() == '&' && type == Token::Type::AND) ||
                (*token.content.data() == '|' && type == Token::Type::OR)) {
                reportError(ErrorInfo::Type::ERROR_TOKEN, token, "Error token");
                bundle.container.insert(token);
                ++bundle.it;
                bundle.last_token = token;
                return true;
            }
        }
        return false;
    }
};

template <Token::Type type, ErrorInfo::Type error_type>
struct ParseTokenRequired {
    bool operator()(ParserContext bundle) {
        bool result = ParseToken<type>{}(bundle.update_strict(true));
        if (!result) {
            Token error;
            error.line = bundle.last_token.line;
            error.col =
                bundle.last_token.col + bundle.last_token.content.size();
            error_infos.emplace_back(ErrorInfo{error_type, error, ""});
            bundle.container.insert(Token{type, token_type_name(type), 0, 0});
        }
        return true;
    }
};

template <typename... Parser>
struct Or {
    bool operator()(ParserContext bundle) {
        bundle.strict = true;
        return (Parser{}(bundle) || ...);
    }
};

template <typename... Parser>
struct Concat {
    bool operator()(ParserContext bundle) {
        if (bundle.strict)
            return invoke(bundle, std::true_type{});
        else
            return invoke(bundle, std::false_type{});
    }

   private:
    inline bool invoke(ParserContext bundle, std::true_type) {
        auto marker = bundle.save_marker();
        bool result = invoke(bundle, std::false_type{});
        if (!result) {
            bundle.reserve_marker(marker);
        }
        return result;
    }

    inline bool invoke(ParserContext bundle, std::false_type) {
        return (Parser{}(bundle) && ...);
    }
};

template <typename Parser>
struct Optional {
    bool operator()(ParserContext bundle) {
        bundle.strict = true;
        Parser{}(bundle);
        return true;
    }
};

template <typename Parser>
struct Several {
    bool operator()(ParserContext bundle) {
        bundle.strict = true;
        bool result;
        do {
            result = Parser{}(bundle);
        } while (result);
        return true;
    }
};

template <ASTNode::Type type, typename Parser>
struct Define {
    bool operator()(ParserContext bundle) {
        auto ast_node = std::make_unique<ASTNode>(type);
        bool res      = Parser{}(bundle.update_container(ast_node->children));
        if (res) {
            bundle.container.insert(std::move(ast_node));
        }
        return res;
    }
};

#define DEFINE(type, definition)          \
    template <>                           \
    struct ParseNode<ASTNode::Type::type> \
        : Define<ASTNode::Type::type, definition> {}

// NOLINTBEGIN(bugprone-macro-parentheses)
#define ASSIGN(type, definition) \
    template <>                  \
    struct ParseNode<ASTNode::Type::type> : definition {}
// NOLINTEND(bugprone-macro-parentheses)

#define OR(...) Or<__VA_ARGS__>
#define CONCAT(...) Concat<__VA_ARGS__>
#define OPTIONAL(parser) Optional<parser>
#define SEVERAL(parser) Several<parser>
#define TOKEN(type) ParseToken<Token::Type::type>
#define TOKEN_R(type, err) \
    ParseTokenRequired<Token::Type::type, ErrorInfo::Type::err>
#define NODE(type) ParseNode<ASTNode::Type::type>

DEFINE(COMP_UNIT, SEVERAL(OR(NODE(FUNC_DEF), NODE(MAIN_FUNC_DEF), NODE(DECL))));
ASSIGN(DECL, OR(NODE(CONST_DECL), NODE(VAR_DECL)));
DEFINE(CONST_DECL, CONCAT(TOKEN(CONSTTK), TOKEN(INTTK), NODE(CONST_DEF),
                          SEVERAL(CONCAT(TOKEN(COMMA), NODE(CONST_DEF))),
                          TOKEN_R(SEMICN, MISSING_SEMICOLON)));
DEFINE(CONST_DEF, CONCAT(TOKEN(IDENFR),
                         OPTIONAL(CONCAT(TOKEN(LBRACK), NODE(CONST_EXP),
                                         TOKEN_R(RBRACK, MISSING_RBRACKET))),
                         TOKEN(ASSIGN), NODE(CONST_INIT_VAL)));
DEFINE(
    CONST_INIT_VAL,
    OR(NODE(CONST_EXP),
       CONCAT(TOKEN(LBRACE),
              OPTIONAL(CONCAT(NODE(CONST_EXP),
                              SEVERAL(CONCAT(TOKEN(COMMA), NODE(CONST_EXP))))),
              TOKEN(RBRACE))));
DEFINE(VAR_DECL, CONCAT(OPTIONAL(TOKEN(STATICTK)), TOKEN(INTTK), NODE(VAR_DEF),
                        SEVERAL(CONCAT(TOKEN(COMMA), NODE(VAR_DEF))),
                        TOKEN_R(SEMICN, MISSING_SEMICOLON)));
DEFINE(VAR_DEF, CONCAT(TOKEN(IDENFR),
                       OPTIONAL(CONCAT(TOKEN(LBRACK), NODE(CONST_EXP),
                                       TOKEN_R(RBRACK, MISSING_RBRACKET))),
                       OPTIONAL(CONCAT(TOKEN(ASSIGN), NODE(INIT_VAL)))));
DEFINE(INIT_VAL,
       OR(NODE(EXP),
          CONCAT(TOKEN(LBRACE),
                 OPTIONAL(CONCAT(NODE(EXP),
                                 SEVERAL(CONCAT(TOKEN(COMMA), NODE(EXP))))),
                 TOKEN(RBRACE))));
DEFINE(FUNC_DEF, CONCAT(NODE(FUNC_TYPE), TOKEN(IDENFR), TOKEN(LPARENT),
                        OPTIONAL(NODE(FUNC_PARAMS)),
                        TOKEN_R(RPARENT, MISSING_RPAREN), NODE(BLOCK)));
DEFINE(MAIN_FUNC_DEF, CONCAT(TOKEN(INTTK), TOKEN(MAINTK), TOKEN(LPARENT),
                             TOKEN_R(RPARENT, MISSING_RPAREN), NODE(BLOCK)));
DEFINE(FUNC_TYPE, OR(TOKEN(VOIDTK), TOKEN(INTTK)));
DEFINE(FUNC_PARAMS, CONCAT(NODE(FUNC_PARAM),
                           SEVERAL(CONCAT(TOKEN(COMMA), NODE(FUNC_PARAM)))));
DEFINE(FUNC_PARAM, CONCAT(TOKEN(INTTK), TOKEN(IDENFR),
                          OPTIONAL(CONCAT(TOKEN(LBRACK),
                                          TOKEN_R(RBRACK, MISSING_RBRACKET)))));
DEFINE(BLOCK, CONCAT(TOKEN(LBRACE), SEVERAL(OR(NODE(DECL), NODE(STMT))),
                     TOKEN(RBRACE)));
DEFINE(STMT, OR(CONCAT(NODE(L_VAL), TOKEN(ASSIGN), NODE(EXP),
                       TOKEN_R(SEMICN, MISSING_SEMICOLON)),
                CONCAT(NODE(EXP), TOKEN_R(SEMICN, MISSING_SEMICOLON)),
                TOKEN(SEMICN), NODE(BLOCK),
                CONCAT(TOKEN(IFTK), TOKEN(LPARENT), NODE(COND),
                       TOKEN_R(RPARENT, MISSING_RPAREN), NODE(STMT),
                       OPTIONAL(CONCAT(TOKEN(ELSETK), NODE(STMT)))),
                CONCAT(TOKEN(FORTK), TOKEN(LPARENT), OPTIONAL(NODE(FOR_STMT)),
                       TOKEN(SEMICN), OPTIONAL(NODE(COND)), TOKEN(SEMICN),
                       OPTIONAL(NODE(FOR_STMT)),
                       TOKEN_R(RPARENT, MISSING_RPAREN), NODE(STMT)),
                CONCAT(TOKEN(BREAKTK), TOKEN_R(SEMICN, MISSING_SEMICOLON)),
                CONCAT(TOKEN(CONTINUETK), TOKEN_R(SEMICN, MISSING_SEMICOLON)),
                CONCAT(TOKEN(RETURNTK), OPTIONAL(NODE(EXP)),
                       TOKEN_R(SEMICN, MISSING_SEMICOLON)),
                CONCAT(TOKEN(PRINTFTK), TOKEN(LPARENT), TOKEN(STRCON),
                       SEVERAL(CONCAT(TOKEN(COMMA), NODE(EXP))),
                       TOKEN_R(RPARENT, MISSING_RPAREN),
                       TOKEN_R(SEMICN, MISSING_SEMICOLON))));
DEFINE(FOR_STMT, CONCAT(NODE(L_VAL), TOKEN(ASSIGN), NODE(EXP),
                        SEVERAL(CONCAT(TOKEN(COMMA), NODE(L_VAL), TOKEN(ASSIGN),
                                       NODE(EXP)))));
DEFINE(EXP, NODE(ADD_EXP));
DEFINE(CONST_EXP, NODE(ADD_EXP));
DEFINE(COND, NODE(LOR_EXP));
DEFINE(L_VAL, CONCAT(TOKEN(IDENFR),
                     OPTIONAL(CONCAT(TOKEN(LBRACK), NODE(EXP),
                                     TOKEN_R(RBRACK, MISSING_RBRACKET)))));
DEFINE(PRIMARY_EXP,
       OR(CONCAT(TOKEN(LPARENT), NODE(EXP), TOKEN_R(RPARENT, MISSING_RPAREN)),
          NODE(L_VAL), NODE(NUMBER)));
DEFINE(NUMBER, TOKEN(INTCON));
DEFINE(UNARY_EXP,
       OR(CONCAT(TOKEN(IDENFR), TOKEN(LPARENT), OPTIONAL(NODE(FUNC_RPARAMS)),
                 TOKEN_R(RPARENT, MISSING_RPAREN)),
          NODE(PRIMARY_EXP), CONCAT(NODE(UNARY_OP), NODE(UNARY_EXP))));
DEFINE(UNARY_OP, OR(TOKEN(PLUS), TOKEN(MINU), TOKEN(NOT)));
DEFINE(FUNC_RPARAMS,
       CONCAT(NODE(EXP), SEVERAL(CONCAT(TOKEN(COMMA), NODE(EXP)))));

template <ASTNode::Type type, typename UpperParser, typename... OpParser>
struct ParseBinExp {
    // A -> B | A op B
    bool operator()(ParserContext bundle) {
        // parse first B
        bool result = UpperParser{}(bundle);
        if (!result) return false;

        while (true) {
            auto marker = bundle.save_marker();

            result = (OpParser{}(bundle.update_strict(true)) || ...);
            if (!result) break;

            result = UpperParser{}(bundle);

            if (!result) {
                bundle.reserve_marker(marker);
                break;
            }
        }
        return true;
    }
};

#define BINEXP(type, upper_parser, ...) \
    ParseBinExp<ASTNode::Type::type, upper_parser, __VA_ARGS__>

DEFINE(MUL_EXP,
       BINEXP(MUL_EXP, NODE(UNARY_EXP), TOKEN(MULT), TOKEN(DIV), TOKEN(MOD)));
DEFINE(ADD_EXP, BINEXP(ADD_EXP, NODE(MUL_EXP), TOKEN(PLUS), TOKEN(MINU)));
DEFINE(REL_EXP, BINEXP(REL_EXP, NODE(ADD_EXP), TOKEN(LSS), TOKEN(LEQ),
                       TOKEN(GRE), TOKEN(GEQ)));
DEFINE(EQ_EXP, BINEXP(EQ_EXP, NODE(REL_EXP), TOKEN(EQL), TOKEN(NEQ)));
DEFINE(LAND_EXP, BINEXP(LAND_EXP, NODE(EQ_EXP), TOKEN(AND)));
DEFINE(LOR_EXP, BINEXP(LOR_EXP, NODE(LAND_EXP), TOKEN(OR)));

std::optional<Map> parse_grammer(Lexer::iterator& it) {
    Map   ret;
    Token none;
    bool  result = ParseNode<ASTNode::Type::COMP_UNIT>{}(
        ParserContext{it, ret, none, false});
    if (!result) return std::nullopt;
    return ret;
}
