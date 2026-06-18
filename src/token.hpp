#pragma once

#include <cassert>
#include <ostream>
#include <string_view>
#include <unordered_map>

#define EXPAND_CONSTANT_KEYWORDS     \
    HANDEL_CONSTANT_KEYWORD(IDENFR)  \
    HANDEL_CONSTANT_KEYWORD(INTCON)  \
    HANDEL_CONSTANT_KEYWORD(STRCON)  \
    HANDEL_CONSTANT_KEYWORD(COMMENT) \
    HANDEL_CONSTANT_KEYWORD(WHITESPACE)

#define EXPAND_RESERVED_KEYWORDS                    \
    HANDLE_RESERVED_KEYWORD(CONSTTK, "const")       \
    HANDLE_RESERVED_KEYWORD(INTTK, "int")           \
    HANDLE_RESERVED_KEYWORD(VOIDTK, "void")         \
    HANDLE_RESERVED_KEYWORD(STATICTK, "static")     \
    HANDLE_RESERVED_KEYWORD(BREAKTK, "break")       \
    HANDLE_RESERVED_KEYWORD(CONTINUETK, "continue") \
    HANDLE_RESERVED_KEYWORD(IFTK, "if")             \
    HANDLE_RESERVED_KEYWORD(ELSETK, "else")         \
    HANDLE_RESERVED_KEYWORD(FORTK, "for")           \
    HANDLE_RESERVED_KEYWORD(RETURNTK, "return")     \
    HANDLE_RESERVED_KEYWORD(MAINTK, "main")         \
    HANDLE_RESERVED_KEYWORD(PRINTFTK, "printf")

#define EXPAND_DELIMITER_KEYWORDS           \
    HANDLE_DELIMITER_KEYWORDS(NOT, "!")     \
    HANDLE_DELIMITER_KEYWORDS(AND, "&&")    \
    HANDLE_DELIMITER_KEYWORDS(OR, "||")     \
    HANDLE_DELIMITER_KEYWORDS(PLUS, "+")    \
    HANDLE_DELIMITER_KEYWORDS(MINU, "-")    \
    HANDLE_DELIMITER_KEYWORDS(MULT, "*")    \
    HANDLE_DELIMITER_KEYWORDS(DIV, "/")     \
    HANDLE_DELIMITER_KEYWORDS(MOD, "%")     \
    HANDLE_DELIMITER_KEYWORDS(SEMICN, ";")  \
    HANDLE_DELIMITER_KEYWORDS(COMMA, ",")   \
    HANDLE_DELIMITER_KEYWORDS(LSS, "<")     \
    HANDLE_DELIMITER_KEYWORDS(LEQ, "<=")    \
    HANDLE_DELIMITER_KEYWORDS(GRE, ">")     \
    HANDLE_DELIMITER_KEYWORDS(GEQ, ">=")    \
    HANDLE_DELIMITER_KEYWORDS(EQL, "==")    \
    HANDLE_DELIMITER_KEYWORDS(NEQ, "!=")    \
    HANDLE_DELIMITER_KEYWORDS(LPARENT, "(") \
    HANDLE_DELIMITER_KEYWORDS(RPARENT, ")") \
    HANDLE_DELIMITER_KEYWORDS(LBRACK, "[")  \
    HANDLE_DELIMITER_KEYWORDS(RBRACK, "]")  \
    HANDLE_DELIMITER_KEYWORDS(LBRACE, "{")  \
    HANDLE_DELIMITER_KEYWORDS(RBRACE, "}")  \
    HANDLE_DELIMITER_KEYWORDS(ASSIGN, "=")

class Token {
   public:
    // clang-format off
    enum class Type {
        NONE, // not a token

#define HANDEL_CONSTANT_KEYWORD(X) X,
#define HANDLE_RESERVED_KEYWORD(X, V) X,
#define HANDLE_DELIMITER_KEYWORDS(X, V) X,
        EXPAND_CONSTANT_KEYWORDS
        EXPAND_RESERVED_KEYWORDS
        EXPAND_DELIMITER_KEYWORDS
#undef HANDEL_CONSTANT_KEYWORD
#undef HANDLE_RESERVED_KEYWORD
#undef HANDLE_DELIMITER_KEYWORDS

        ERROR=-1 // error
    } type = Token::Type::NONE;
    // clang-format on

    std::string_view content = {};

    size_t line = 1;
    size_t col  = 1;

    constexpr operator bool() const { return type != Token::Type::NONE; }
};

// constexpr Token TokNone = {};

[[nodiscard]] std::string_view token_type_name(Token::Type type);

std::ostream& operator<<(std::ostream& os, Token::Type type);

extern const std::unordered_map<std::string_view, Token::Type>
    RESERVED_KEYWORDS_MAP;
