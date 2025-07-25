#pragma once

#include <iostream>
#include <vector>
#include <optional>
#include <variant>
#include "allocator.hpp"
#include "tokenizer.hpp"

using namespace std;

struct NodeExpression;
struct NodeStatement;

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
struct NodeAssignment { Token identifier; NodeExpression* value; };
struct NodeScope { vector<NodeStatement*> statements; };
struct NodeIf { NodeExpression* condition; NodeScope* scope; };
struct NodeElse { NodeScope* scope; };
struct NodeStatement { variant<NodeExit*, NodeLet*, NodeScope*, NodeIf*, NodeElse*, NodeAssignment*> var; };
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
                    cerr << "Failed to parse expression inside parentheses at line " << parOpen->line << "." << endl;
                    exit(EXIT_FAILURE);
                }
                tryConsume(TokenType::PAR_CLOSE, "Invalid Syntax: Expected `)` after expression at line " + to_string(parOpen->line));
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
                    cerr << "Failed to parse right term after operator at line " << op.line << "." << endl;
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

        optional<NodeScope*> parseScope() {
            auto curOpenTokenOpt = tryConsume(TokenType::CUR_OPEN);
            if(!curOpenTokenOpt.has_value()) return {};
            Token curOpenToken = curOpenTokenOpt.value();
            NodeScope* scopeNode = _allocator.allocate<NodeScope>();
            while (true) {
                if (peek().has_value() && peek().value().type == TokenType::CUR_CLOSE) break;
                if (auto statement = parseStatement()) scopeNode->statements.push_back(statement.value());
                else break;
            }
            tryConsume(TokenType::CUR_CLOSE, "Invalid Syntax: Expected `}` to close scope at line " + to_string(curOpenToken.line));
            return scopeNode;
        }

        optional<NodeStatement*> parseStatement() {
            if(peek().value().type == TokenType::EXIT) {
                    Token exitToken = consume(); // consume 'exit'
                    if(peek().has_value() && peek().value().type == TokenType::PAR_OPEN) {
                        Token parOpenToken = consume(); // consume '('
                        auto exitStatement = _allocator.allocate<NodeExit>();
                        if(auto nodeExp = parseExp()) {
                            exitStatement->exp = nodeExp.value();
                        } else {
                            cerr << "Failed to parse exit expression at line " << exitToken.line << "." << endl;
                            exit(EXIT_FAILURE);
                        }
                        tryConsume(TokenType::PAR_CLOSE, "Invalid Syntax: Expected `)` after exit expression at line " + to_string(exitToken.line));
                        auto statementNode = _allocator.allocate<NodeStatement>();
                        statementNode->var = exitStatement;
                        return statementNode;
                    } else {
                        cerr << "Invalid Syntax: Expected `(` after `exit` at line " << exitToken.line << "." << endl;
                        exit(EXIT_FAILURE);
                    }
            } else if(peek().has_value() && peek().value().type == TokenType::LET) {
                Token letToken = peek().value();
                if(peek(1).has_value() && peek(1).value().type == TokenType::IDENTIFIER) {
                    Token identifierToken = peek(1).value();
                    if(peek(2).has_value() && peek(2).value().type == TokenType::EQUALS) {
                        consume(); // consume 'let'
                        auto letNode = _allocator.allocate<NodeLet>();
                        letNode->identifier = consume(); // consume identifier
                        consume(); // consume '='
                        if(auto nodeExp = parseExp()) {
                            letNode->value = nodeExp.value();
                        } else {
                            cerr << "Failed to parse let value expression at line " << letToken.line << "." << endl;
                            exit(EXIT_FAILURE);
                        }
                        auto statementNode = _allocator.allocate<NodeStatement>();
                        statementNode->var = letNode;
                        return statementNode;
                    } else {
                        cerr << "Invalid Syntax: Expected `=` after identifier at line " << identifierToken.line << "." << endl;
                        exit(EXIT_FAILURE);
                    }
                } else {
                    cerr << "Invalid Syntax: Expected identifier after `let` at line " << letToken.line << "." << endl;
                    exit(EXIT_FAILURE);
                }
            } else if(peek().has_value() && peek().value().type == TokenType::IDENTIFIER) {
                if(peek(1).has_value() && peek(1).value().type == TokenType::EQUALS) {
                    auto assignmentNode = _allocator.allocate<NodeAssignment>();
                    assignmentNode->identifier = consume(); // consume identifier
                    consume(); // consume '='
                    if(auto expression = parseExp()) {
                        assignmentNode->value = expression.value();
                        auto statementNode = _allocator.allocate<NodeStatement>();
                        statementNode->var = assignmentNode;
                        return statementNode;
                    } else {
                        cerr << "Failed to parse assignment expression at line " << assignmentNode->identifier.line << "." << endl;
                        exit(EXIT_FAILURE);
                    }
                }
            } else if(auto _if = tryConsume(TokenType::IF)) {
                tryConsume(TokenType::PAR_OPEN, "Invalid Syntax: Expected `(` after `if` at line " + to_string(_if->line) + ".");
                auto ifStatement = _allocator.allocate<NodeIf>();
                if(auto condition = parseExp()) ifStatement->condition = condition.value();
                else {
                    cerr << "Failed to parse if condition at line " << _if->line << "." << endl;
                    exit(EXIT_FAILURE);
                }
                tryConsume(TokenType::PAR_CLOSE, "Invalid Syntax: Expected `)` after if condition at line " + to_string(_if->line) + ".");
                if(auto scopeNode = parseScope()) ifStatement->scope = scopeNode.value();
                else {
                    cerr << "Failed to parse if scope at line " << _if->line << "." << endl;
                    exit(EXIT_FAILURE);
                }
                auto statementNode = _allocator.allocate<NodeStatement>();
                statementNode->var = ifStatement;
                return statementNode;
            } else if(auto _else = tryConsume(TokenType::ELSE)) {
                if(peek().has_value() && peek().value().type == TokenType::CUR_OPEN) {
                    auto elseNode = _allocator.allocate<NodeElse>();
                    if(auto scopeNode = parseScope()) {
                        elseNode->scope = scopeNode.value();
                        auto statementNode = _allocator.allocate<NodeStatement>();
                        statementNode->var = elseNode;
                        return statementNode;
                    } else {
                        cerr << "Failed to parse else scope at line " << _else->line << "." << endl;
                        exit(EXIT_FAILURE);
                    }
                } else {
                    cerr << "Invalid Syntax: Expected `{` after `else` at line " << _else->line << "." << endl;
                    exit(EXIT_FAILURE);
                }
            } else if(peek().has_value() && peek().value().type == TokenType::CUR_OPEN) {
                if(auto scopeNode = parseScope()) {
                    auto statementNode = _allocator.allocate<NodeStatement>();
                    statementNode->var = scopeNode.value();
                    return statementNode;
                } else {
                    cerr << "Failed to parse scope at line " << peek().value().line << "." << endl;
                    exit(EXIT_FAILURE);
                }
            } else {
                if(peek().has_value()) {
                    cerr << "Invalid Syntax: Unexpected token `" << peek().value().value << "` at line " << peek().value().line << "." << endl;
                } else {
                    cerr << "Invalid Syntax: Unexpected end of input at line " << _tokens.back().line << "." << endl;
                }
                exit(EXIT_FAILURE);
            }
        }

        optional<NodeProgram> parse() {
            NodeProgram program;
            while(peek().has_value()) {
                if(auto nodeStatement = parseStatement()) {
                    program.statements.push_back(nodeStatement.value());
                } else {
                    cerr << "Failed to parse statement at line " << _tokens.back().line << "." << endl;
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
            cerr << error << endl;
            exit(EXIT_FAILURE);
        }

        inline optional<Token> tryConsume(TokenType type) {
            if(peek().has_value() && peek().value().type == type) return consume();
            else return {};
        }
};