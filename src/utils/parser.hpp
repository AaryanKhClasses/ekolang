#pragma once

#include <iostream>
#include <vector>
#include <optional>
#include <variant>
#include "allocator.hpp"
#include "tokenizer.hpp"

using namespace std;

struct NodeExpressionNumber { Token number; };
struct NodeExpressionIdentifier { Token identifier; };
struct NodeExpression;

struct BinaryExpressionAdd { NodeExpression* left; NodeExpression* right; };
struct BinaryExpression { BinaryExpressionAdd* add; };
struct NodeExpression { variant<NodeExpressionNumber*, NodeExpressionIdentifier*, BinaryExpression*> var; };

struct NodeExit { NodeExpression* exp; };
struct NodeLet { Token identifier; NodeExpression* value; };
struct NodeStatement { variant<NodeExit*, NodeLet*> var; };
struct NodeProgram { vector<NodeStatement*> statements; };

class Parser {
    public:
        inline explicit Parser(const vector<Token>& tokens): _tokens(move(tokens)), _allocator(1024*1024*4) { }

        optional<NodeExpression*> parseExp() {
            if(peek().has_value() && peek().value().type == TokenType::NUMBER) {
                auto numberNode = _allocator.allocate<NodeExpressionNumber>();
                numberNode->number = consume();
                auto expressionNode = _allocator.allocate<NodeExpression>();
                expressionNode->var = numberNode;
                return expressionNode;
            } else if(peek().has_value() && peek().value().type == TokenType::IDENTIFIER) {
                auto identifierNode = _allocator.allocate<NodeExpressionIdentifier>();
                identifierNode->identifier = consume();
                auto expressionNode = _allocator.allocate<NodeExpression>();
                expressionNode->var = identifierNode;
                return expressionNode;
            } else return {};
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

                    if(peek().has_value() && peek().value().type == TokenType::PAR_CLOSE) consume();
                    else {
                        cerr << "Invalid Syntax: Expected `)`." << endl;
                        exit(EXIT_FAILURE);
                    }
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
};