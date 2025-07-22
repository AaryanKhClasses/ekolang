#pragma once

#include <iostream>
#include <vector>
#include <optional>

using namespace std;

enum class TokenType {
    EXIT, NUMBER, IDENTIFIER, LET, EQUALS, PLUS,
    PAR_OPEN, PAR_CLOSE,
};

struct Token {
    TokenType type;
    string value;
};

class Tokenizer {
    public:
        inline explicit Tokenizer(const string& src): _src(src) { }

        inline vector<Token> tokenize() {
            vector<Token> tokens;
            string buffer;

            while(peek().has_value()) {
                if(isalpha(peek().value())) {
                    buffer.push_back(consume());
                    while(peek().has_value() && isalnum(peek().value())) buffer.push_back(consume());

                    if(buffer == "exit") {
                        tokens.push_back({.type = TokenType::EXIT, .value = buffer});
                        buffer.clear();
                        continue;
                    } else if(buffer == "let") {
                        tokens.push_back({.type = TokenType::LET, .value = buffer});
                        buffer.clear();
                        continue;
                    } else {
                        tokens.push_back({.type = TokenType::IDENTIFIER, .value = buffer});
                        buffer.clear();
                        continue;
                    }
                } else if(isdigit(peek().value())) {
                    buffer.push_back(consume());
                    while(peek().has_value() && isdigit(peek().value())) buffer.push_back(consume());
                    tokens.push_back({.type = TokenType::NUMBER, .value = buffer});
                    buffer.clear();
                    continue;
                } else if(peek().value() == '(') {
                    tokens.push_back({.type = TokenType::PAR_OPEN, .value = "("});
                    consume();
                    continue;
                } else if(peek().value() == ')') {
                    tokens.push_back({.type = TokenType::PAR_CLOSE, .value = ")"});
                    consume();
                    continue;
                } else if(peek().value() == '=') {
                    tokens.push_back({.type = TokenType::EQUALS, .value = "="});
                    consume();
                    continue;
                } else if(peek().value() == '+') {
                    tokens.push_back({.type = TokenType::PLUS, .value = "+"});
                    consume();
                    continue;
                } else if(isspace(peek().value())) {
                    consume();
                    continue;
                } else {
                    cerr << "Invalid Syntax: Unknown character `" << peek().value() << "`" << endl;
                    exit(EXIT_FAILURE);
                }
            }
            _index = 0;
            return tokens;
        }

    private:
        const string _src;
        size_t _index = 0;

        [[nodiscard]] inline optional<char> peek(int num = 0) const {
            if(_index + num >= _src.length()) return {};
            else return _src.at(_index + num);
        }

        inline char consume() { return _src.at(_index++); }
};