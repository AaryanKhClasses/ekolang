#pragma once

#include <iostream>
#include <vector>
#include <optional>
#include <variant>
#include "allocator.hpp"
#include "tokenizer.hpp"

using namespace std;

struct NodeExpression;

struct NodeTermNumber { Token number; };
struct NodeTermIdentifier { Token identifier; };
struct NodeTermParentheses { NodeExpression* expression; };
struct NodeTerm { variant<NodeTermNumber*, NodeTermIdentifier*, NodeTermParentheses*> var; };

struct BinaryExpressionAdd { NodeExpression* left; NodeExpression* right; };
struct BinaryExpressionSubtract { NodeExpression* left; NodeExpression* right; };
struct BinaryExpressionMultiply { NodeExpression* left; NodeExpression* right; };
struct BinaryExpressionDivide { NodeExpression* left; NodeExpression* right; };
struct BinaryExpression { variant<BinaryExpressionAdd*, BinaryExpressionSubtract*, BinaryExpressionMultiply*, BinaryExpressionDivide*> var; };
struct NodeExpression { variant<NodeTerm*, BinaryExpression*> var; };

struct NodeExit { NodeExpression* exp; };
struct NodeLet { Token identifier; NodeExpression* value; };
struct NodeStatement { variant<NodeExit*, NodeLet*> var; };
struct NodeProgram { vector<NodeStatement*> statements; };

class Parser {
    public:
        inline explicit Parser(const vector<Token>& tokens): _tokens(move(tokens)), _allocator(1024*1024*4) { }

        optional<NodeTerm*> parseTerm() {
            if(auto number = tryConsume(TokenType::NUMBER)) {
                auto numberTerm = _allocator.allocate<NodeTermNumber>();
                numberTerm->number = number.value();
                auto term = _allocator.allocate<NodeTerm>();
                term->var = numberTerm;
                return term;
            } else if(auto identifier = tryConsume(TokenType::IDENTIFIER)) {
                auto identifierTerm = _allocator.allocate<NodeTermIdentifier>();
                identifierTerm->identifier = identifier.value();
                auto term = _allocator.allocate<NodeTerm>();
                term->var = identifierTerm;
                return term;
            } else if(auto parOpen = tryConsume(TokenType::PAR_OPEN)) {
                auto expression = parseExp();
                if(!expression.has_value()) {
                    cerr << "Failed to parse expression inside parentheses." << endl;
                    exit(EXIT_FAILURE);
                }
                tryConsume(TokenType::PAR_CLOSE, "Invalid Syntax: Expected `)` after expression.");
                auto parenthesesTerm = _allocator.allocate<NodeTermParentheses>();
                parenthesesTerm->expression = expression.value();
                auto term = _allocator.allocate<NodeTerm>();
                term->var = parenthesesTerm;
                return term;
            } else {
                return {};
            }
        }

        optional<NodeExpression*> parseExp(int minPrecedence = 0) {
            optional<NodeTerm*> leftTerm = parseTerm();
            if(!leftTerm.has_value()) return {};

            auto leftExpression = _allocator.allocate<NodeExpression>();
            leftExpression->var = leftTerm.value();

            while(true) {
                optional<Token> current = peek();
                optional<int> precedence;
                if(current.has_value()) {
                    precedence = binaryPrecedence(current->type);
                    if(!precedence.has_value() || precedence < minPrecedence) break;
                } else break;

                Token op = consume(); // consume the operator token
                int nextPrecedence = precedence.value() + 1;
                auto rightTerm = parseExp(nextPrecedence);
                if(!rightTerm.has_value()) {
                    cerr << "Failed to parse right term after operator." << endl;
                    exit(EXIT_FAILURE);
                }

                auto expression = _allocator.allocate<BinaryExpression>();
                auto newLeftExpression = _allocator.allocate<NodeExpression>();
                if(op.type == TokenType::PLUS) {
                    auto addExpression = _allocator.allocate<BinaryExpressionAdd>();
                    newLeftExpression->var = leftExpression->var;
                    addExpression->left = newLeftExpression;
                    addExpression->right = rightTerm.value();
                    expression->var = addExpression;
                } else if(op.type == TokenType::MINUS) {
                    auto subtractExpression = _allocator.allocate<BinaryExpressionSubtract>();
                    newLeftExpression->var = leftExpression->var;
                    subtractExpression->left = newLeftExpression;
                    subtractExpression->right = rightTerm.value();
                    expression->var = subtractExpression;
                } else if(op.type == TokenType::TIMES) {
                    auto multiplyExpression = _allocator.allocate<BinaryExpressionMultiply>();
                    newLeftExpression->var = leftExpression->var;
                    multiplyExpression->left = newLeftExpression;
                    multiplyExpression->right = rightTerm.value();
                    expression->var = multiplyExpression;
                } else if(op.type == TokenType::DIVIDE) {
                    auto divideExpression = _allocator.allocate<BinaryExpressionDivide>();
                    newLeftExpression->var = leftExpression->var;
                    divideExpression->left = newLeftExpression;
                    divideExpression->right = rightTerm.value();
                    expression->var = divideExpression;
                }
                leftExpression->var = expression;
            }
            return leftExpression;
        }

        optional<NodeStatement*> parseStatement() {
            if(peek().value().type == TokenType::EXIT) {
                if(peek(1).has_value() && peek(1).value().type == TokenType::PAR_OPEN) {
                    consume(); consume();
                    auto exitStatement = _allocator.allocate<NodeExit>();
                    if(auto nodeExp = parseExp()) {
                        exitStatement->exp = nodeExp.value();
                    } else {
                        cerr << "Failed to parse exit expression." << endl;
                        exit(EXIT_FAILURE);
                    }
                    tryConsume(TokenType::PAR_CLOSE, "Invalid Syntax: Expected `)` after exit expression.");
                    auto statementNode = _allocator.allocate<NodeStatement>();
                    statementNode->var = exitStatement;
                    return statementNode;
                } else {
                    cerr << "Invalid Syntax: Expected `(`." << endl;
                    exit(EXIT_FAILURE);
                }
            } else if(peek().has_value() && peek().value().type == TokenType::LET) {
                if(peek(1).has_value() && peek(1).value().type == TokenType::IDENTIFIER) {
                    if(peek(2).has_value() && peek(2).value().type == TokenType::EQUALS) {
                        consume(); // consume 'let'
                        auto letNode = _allocator.allocate<NodeLet>();
                        letNode->identifier = consume(); // consume identifier
                        consume(); // consume '='
                        if(auto nodeExp = parseExp()) {
                            letNode->value = nodeExp.value();
                        } else {
                            cerr << "Failed to parse let value expression." << endl;
                            exit(EXIT_FAILURE);
                        }
                        auto statementNode = _allocator.allocate<NodeStatement>();
                        statementNode->var = letNode;
                        return statementNode;
                    } else {
                        cerr << "Invalid Syntax: Expected `=` after identifier." << endl;
                        exit(EXIT_FAILURE);
                    }
                } else {
                    cerr << "Invalid Syntax: Expected identifier after `let`." << endl;
                    exit(EXIT_FAILURE);
                }
            } else {
                cerr << "Invalid Syntax: Expected `exit` or `let`." << endl;
                exit(EXIT_FAILURE);
            }
        }

        optional<NodeProgram> parse() {
            NodeProgram program;
            while(peek().has_value()) {
                if(auto nodeStatement = parseStatement()) {
                    program.statements.push_back(nodeStatement.value());
                } else {
                    cerr << "Failed to parse statement." << endl;
                    exit(EXIT_FAILURE);
                }
            }
            return program;
        }

    private:
        const vector<Token> _tokens;
        size_t _index = 0;
        Allocator _allocator;

        [[nodiscard]] inline optional<Token> peek(int num = 0) const {
            if(_index + num >= _tokens.size()) return {};
            else return _tokens.at(_index + num);
        }

        inline Token consume() { return _tokens.at(_index++); }

        inline Token tryConsume(TokenType type, string error) {
            if(peek().has_value() && peek().value().type == type) return consume();
            else {
                cerr << error << endl;
                exit(EXIT_FAILURE);
            }
        }

        inline optional<Token> tryConsume(TokenType type) {
            if(peek().has_value() && peek().value().type == type) return consume();
            else return {};
        }
};