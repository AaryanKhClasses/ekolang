#pragma once

#include <iostream>
#include <vector>
#include <optional>

using namespace std;

enum class TokenType {
    EXIT, NUMBER, IDENTIFIER, LET, IF, ELSE,
    EQUALS, PLUS, TIMES, MINUS, DIVIDE,
    PAR_OPEN, PAR_CLOSE, CUR_OPEN, CUR_CLOSE
};

optional<int> binaryPrecedence(TokenType type) {
    switch(type) {
        case TokenType::PLUS:
        case TokenType::MINUS:
            return 0;
        case TokenType::TIMES:
        case TokenType::DIVIDE:
            return 1;
        default:
            return {};
    }
}

struct Token {
    TokenType type;
    string value;
    int line;
};

class Tokenizer {
    public:
        inline explicit Tokenizer(const string& src): _src(src) { }

        inline vector<Token> tokenize() {
            vector<Token> tokens;
            string buffer;
            int lineCount = 0;

            while(peek().has_value()) {
                if(isalpha(peek().value())) {
                    buffer.push_back(consume());
                    while(peek().has_value() && isalnum(peek().value())) buffer.push_back(consume());

                    if(buffer == "exit") {
                        tokens.push_back({.type = TokenType::EXIT, .value = buffer, .line = lineCount});
                        buffer.clear();
                        continue;
                    } else if(buffer == "let") {
                        tokens.push_back({.type = TokenType::LET, .value = buffer, .line = lineCount});
                        buffer.clear();
                        continue;
                    } else if(buffer == "if") {
                        tokens.push_back({.type = TokenType::IF, .value = buffer, .line = lineCount});
                        buffer.clear();
                        continue;
                    } else if(buffer == "else") {
                        tokens.push_back({.type = TokenType::ELSE, .value = buffer, .line = lineCount});
                        buffer.clear();
                        continue;
                    } else {
                        tokens.push_back({.type = TokenType::IDENTIFIER, .value = buffer, .line = lineCount});
                        buffer.clear();
                        continue;
                    }
                } else if(isdigit(peek().value())) {
                    buffer.push_back(consume());
                    while(peek().has_value() && isdigit(peek().value())) buffer.push_back(consume());
                    tokens.push_back({.type = TokenType::NUMBER, .value = buffer, .line = lineCount});
                    buffer.clear();
                    continue;
                } else if(peek().value() == '/' && peek(1).has_value() && peek(1).value() == '/') {
                    consume(); consume();
                    while(peek().has_value() && peek().value() != '\n') consume();
                    continue;
                } else if(peek().value() == '/' && peek(1).has_value() && peek(1).value() == '*') {
                    consume(); consume();
                    while(peek().has_value()) {
                        if(peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/') break;
                        if(peek().value() == '\n') lineCount++;
                        consume();
                    }
                    lineCount++;
                    consume(); consume();
                    continue;
                } else if(peek().value() == '(') {
                    tokens.push_back({.type = TokenType::PAR_OPEN, .value = "(", .line = lineCount});
                    consume();
                    continue;
                } else if(peek().value() == ')') {
                    tokens.push_back({.type = TokenType::PAR_CLOSE, .value = ")", .line = lineCount});
                    consume();
                    continue;
                } else if(peek().value() == '=') {
                    tokens.push_back({.type = TokenType::EQUALS, .value = "=", .line = lineCount});
                    consume();
                    continue;
                } else if(peek().value() == '+') {
                    tokens.push_back({.type = TokenType::PLUS, .value = "+", .line = lineCount});
                    consume();
                    continue;
                } else if(peek().value() == '-') {
                    tokens.push_back({.type = TokenType::MINUS, .value = "-", .line = lineCount});
                    consume();
                    continue;
                } else if(peek().value() == '*') {
                    tokens.push_back({.type = TokenType::TIMES, .value = "*", .line = lineCount});
                    consume();
                    continue;
                } else if(peek().value() == '/') {
                    tokens.push_back({.type = TokenType::DIVIDE, .value = "/", .line = lineCount});
                    consume();
                    continue;
                } else if(peek().value() == '{') {
                    tokens.push_back({.type = TokenType::CUR_OPEN, .value = "{", .line = lineCount});
                    consume();
                    continue;
                } else if(peek().value() == '}') {
                    tokens.push_back({.type = TokenType::CUR_CLOSE, .value = "}", .line = lineCount});
                    consume();
                    continue;
                } else if(peek().value() == '\n') {
                    lineCount++;
                    consume();
                    continue;
                } else if(isspace(peek().value())) {
                    consume();
                    continue;
                } else {
                    cerr << "Invalid Syntax: Unexpected character `" << peek().value() << "` at line " << lineCount << "." << endl;
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