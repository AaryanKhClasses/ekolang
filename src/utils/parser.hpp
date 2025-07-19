#pragma once

#include <iostream>
#include <vector>
#include <optional>
#include "tokenizer.hpp"

using namespace std;

struct NodeExpression {
    Token number;
};

struct NodeExit {
    NodeExpression exp;
};

class Parser {
    public:
        inline explicit Parser(const vector<Token>& tokens): _tokens(move(tokens)) { }

        optional<NodeExpression> parseExp() {
            if(peek().has_value() && peek().value().type == TokenType::NUMBER) {
                return NodeExpression{.number = consume()};
            } else return {};
        }

        optional<NodeExit> parse() {
            optional<NodeExit> exitNode;
            while(peek().has_value()) {
                if(peek().value().type == TokenType::EXIT) {
                    consume();
                    if(auto nodeExp = parseExp()) {
                        exitNode = NodeExit{.exp = nodeExp.value()};
                    } else {
                        cerr << "Failed to parse exit expression." << endl;
                        exit(EXIT_FAILURE);
                    }
                }
            }

            _index = 0;
            return exitNode;
        }

    private:
        const vector<Token> _tokens;
        size_t _index = 0;

        [[nodiscard]] inline optional<Token> peek(int num = 1) const {
            if(_index + num > _tokens.size()) return {};
            else return _tokens.at(_index);
        }

        inline Token consume() { return _tokens.at(_index++); }
};