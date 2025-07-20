#pragma once

#include <iostream>
#include <vector>
#include <optional>
#include <variant>
#include "tokenizer.hpp"

using namespace std;

struct NodeExpressionNumber { Token number; };
struct NodeExpressionIdentifier { Token identifier; };
struct NodeExpression { variant<NodeExpressionNumber, NodeExpressionIdentifier> var; };

struct NodeExit { NodeExpression exp; };
struct NodeLet { Token identifier; NodeExpression value; };
struct NodeStatement { variant<NodeExit, NodeLet> var; };
struct NodeProgram { vector<NodeStatement> statements; };

class Parser {
    public:
        inline explicit Parser(const vector<Token>& tokens): _tokens(move(tokens)) { }

        optional<NodeExpression> parseExp() {
            if(peek().has_value() && peek().value().type == TokenType::NUMBER) {
                return NodeExpression{.var = NodeExpressionNumber{.number = consume()}};
            } else if(peek().has_value() && peek().value().type == TokenType::IDENTIFIER) {
                return NodeExpression{.var = NodeExpressionIdentifier{.identifier = consume()}};
            } else return {};
        }

        optional<NodeStatement> parseStatement() {
            if(peek().value().type == TokenType::EXIT) {
                if(peek(1).has_value() && peek(1).value().type == TokenType::PAR_OPEN) {
                    consume(); consume();
                    NodeExit exitStatement;
                    if(auto nodeExp = parseExp()) {
                        exitStatement = NodeExit{.exp = nodeExp.value()};
                    } else {
                        cerr << "Failed to parse exit expression." << endl;
                        exit(EXIT_FAILURE);
                    }

                    if(peek().has_value() && peek().value().type == TokenType::PAR_CLOSE) consume();
                    else {
                        cerr << "Invalid Syntax: Expected `)`." << endl;
                        exit(EXIT_FAILURE);
                    }
                    return NodeStatement{.var = exitStatement};
                } else {
                    cerr << "Invalid Syntax: Expected `(`." << endl;
                    exit(EXIT_FAILURE);
                }
            } else if(peek().has_value() && peek().value().type == TokenType::LET) {
                if(peek(1).has_value() && peek(1).value().type == TokenType::IDENTIFIER) {
                    if(peek(2).has_value() && peek(2).value().type == TokenType::EQUALS) {
                        consume(); // consume 'let'
                        NodeLet letStatement = NodeLet{.identifier = consume()}; // consume identifier
                        consume(); // consume '='
                        if(auto nodeExp = parseExp()) {
                            letStatement.value = nodeExp.value();
                            return NodeStatement{.var = letStatement};
                        } else {
                            cerr << "Failed to parse let value expression." << endl;
                            exit(EXIT_FAILURE);
                        }
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

        [[nodiscard]] inline optional<Token> peek(int num = 0) const {
            if(_index + num >= _tokens.size()) return {};
            else return _tokens.at(_index + num);
        }

        inline Token consume() { return _tokens.at(_index++); }
};