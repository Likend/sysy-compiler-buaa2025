#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string_view>
#include <utility>
#include <variant>

#include "lexer.hpp"
#include "token.hpp"
#include "util/lambda_overload.hpp"
#include "util/scope_hash_set.hpp"

struct ASTNode;

struct ASTNode {
    enum class Type : int {
        COMP_UNIT,
        DECL,
        CONST_DECL,
        CONST_DEF,
        CONST_INIT_VAL,
        VAR_DECL,
        VAR_DEF,
        INIT_VAL,
        FUNC_DEF,
        MAIN_FUNC_DEF,
        FUNC_TYPE,
        FUNC_PARAMS,
        FUNC_PARAM,
        BLOCK,
        STMT,
        FOR_STMT,
        EXP,
        COND,
        L_VAL,
        PRIMARY_EXP,
        NUMBER,
        UNARY_EXP,
        UNARY_OP,
        FUNC_RPARAMS,
        MUL_EXP,
        ADD_EXP,
        REL_EXP,
        EQ_EXP,
        LAND_EXP,
        LOR_EXP,
        CONST_EXP,
    } type;

    ASTNode() = default;
    ASTNode(Type type) : type(type) {};

    using Element = std::variant<std::unique_ptr<ASTNode>, Token>;

    // Warper for ScopeHashSet<Element, MapHash, MapPred>
    class Map {
       private:
        using ElementKey = std::variant<ASTNode::Type, Token::Type>;

        static inline ElementKey element_key(const Element& element) {
            return std::visit(
                overloaded{
                    [](const std::unique_ptr<ASTNode>& node) {
                        return ElementKey{node->type};
                    },
                    [](const Token& token) { return ElementKey{token.type}; }},
                element);
        }

        struct MapHash {
            size_t operator()(const ElementKey& key) const {
                return std::hash<ElementKey>{}(key);
            }
            size_t operator()(const Element& child_node) const {
                return this->operator()(element_key(child_node));
            }
        };

        struct MapPred {
            bool operator()(const ElementKey& a, const ElementKey& b) const {
                return a == b;
            }
            bool operator()(const ElementKey& a, const Element& b) const {
                return this->operator()(a, element_key(b));
            }
            bool operator()(const Element& a, const ElementKey& b) const {
                return this->operator()(element_key(a), b);
            }
            bool operator()(const Element& a, const Element& b) const {
                return this->operator()(element_key(a), element_key(b));
            }
        };

        using SetType = ScopeHashSet<Element, MapHash, MapPred, true>;
        SetType set;

       public:
        size_t size() const { return set.size(); }
        bool   empty() const { return set.empty(); }

        void insert(Element value) { set.insert(std::move(value)); }

        template <typename... Args>
        void emplace_back(Args&&... args) {
            return set.emplace_back(std::forward<Args>(args)...);
        }

        using ScopeMarker = SetType::ScopeMarker;
        ScopeMarker get_scope_marker() const { return set.get_scope_marker(); }
        void        pop_scope(ScopeMarker marker) { set.pop_scope(marker); }

        using iterator = SetType::iterator;
        iterator begin() const { return set.begin(); }
        iterator end() const { return set.end(); }

        const Token* get(Token::Type type) const {
            if (auto it = set.find(ElementKey{type}); it != set.end())
                return &std::get<Token>(*it);
            else
                return nullptr;
        }

        const ASTNode* get(ASTNode::Type type) const {
            if (auto it = set.find(ElementKey{type}); it != set.end())
                return std::get<std::unique_ptr<ASTNode>>(*it).get();
            else
                return nullptr;
        }

        struct TokenEqualRange {
            SetType::EqualRange<ElementKey> inner_range;
            struct Iterator {
                SetType::EqualRange<ElementKey>::iterator inner_it;

                const Token& operator*() const {
                    return std::get<Token>(*inner_it);
                }
                Iterator& operator++() {
                    ++inner_it;
                    return *this;
                }
                Iterator operator++(int) {
                    auto temp = *this;
                    ++(*this);
                    return temp;
                }
                bool operator==(const Iterator& other) const noexcept {
                    return inner_it == other.inner_it;
                }
                bool operator!=(const Iterator& other) const noexcept {
                    return !this->operator==(other);
                }
            };

            Iterator begin() const { return {inner_range.begin()}; }
            Iterator end() const { return {inner_range.end()}; }
        };

        struct ASTNodeEqualRange {
            SetType::EqualRange<ElementKey> inner_range;
            struct Iterator {
                SetType::EqualRange<ElementKey>::iterator inner_it;

                const ASTNode& operator*() const {
                    return *std::get<std::unique_ptr<ASTNode>>(*inner_it);
                }
                Iterator& operator++() {
                    ++inner_it;
                    return *this;
                }
                Iterator operator++(int) {
                    auto temp = *this;
                    ++(*this);
                    return temp;
                }
                bool operator==(const Iterator& other) const noexcept {
                    return inner_it == other.inner_it;
                }
                bool operator!=(const Iterator& other) const noexcept {
                    return !this->operator==(other);
                }
            };

            Iterator begin() const { return {inner_range.begin()}; }
            Iterator end() const { return {inner_range.end()}; }
        };

        TokenEqualRange equal_range(Token::Type type) const {
            return {set.equal_range(ElementKey{type})};
        }
        ASTNodeEqualRange equal_range(ASTNode::Type type) const {
            return {set.equal_range(ElementKey{type})};
        }
    };

    Map children;

    using iterator = Map::iterator;
    iterator begin() { return children.begin(); }
    iterator end() { return children.end(); }
};

#ifdef DEBUG_TOKEN_TYPE_NAME
[[nodiscard]] std::string_view astnode_type_name(ASTNode::Type type);

std::ostream& operator<<(std::ostream& os, ASTNode::Type type);
#endif
std::ostream& operator<<(std::ostream& os, const ASTNode& node);

std::optional<ASTNode::Map> parse_grammer(Lexer::iterator& it);
