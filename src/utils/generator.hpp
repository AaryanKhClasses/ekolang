#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include "parser.hpp"

using namespace std;

class Generator {
    public:
        inline explicit Generator(NodeProgram program): _program(move(program)) { }
        bool _hasExplicitExit = false;

        void generateTerm(const NodeTerm* term) {
            struct TermVisitor {
                Generator* generator;

                void operator()(const NodeTermNumber* number) const {
                    generator->_out << "    mov rax, " << number->number.value << "\n"; // move the number to rax
                    generator->push("rax"); // push the value onto the stack
                }

                void operator()(const NodeTermIdentifier* identifier) const {
                    auto it = find_if(generator->_vars.cbegin(), generator->_vars.cend(), [&](const Var& var) {
                        return var.name == identifier->identifier.value;
                    });
                    if(it == generator->_vars.cend()) {
                        cerr << "Invalid Syntax: Identifier `" << identifier->identifier.value << "` does not exist!" << endl;
                        exit(EXIT_FAILURE);
                    }
                    stringstream offset;
                    offset << "QWORD [rsp + " << (generator->_stackSize - (*it).stackPos - 1) * 8 << "]";
                    generator->push(offset.str()); // push the value onto the stack
                }

                void operator()(const NodeTermParentheses* parentheses) const {
                    generator->generateExpression(parentheses->expression);
                }
            };

            TermVisitor visitor{.generator = this};
            visit(visitor, term->var);
        }

        void generateBinaryExpression(const BinaryExpression* binaryExpression) {
            struct BinaryExpressionVisitor {
                Generator* generator;

                void operator()(const BinaryExpressionAdd* add) const {
                    generator->generateExpression(add->left);
                    generator->generateExpression(add->right);
                    generator->pop("rax"); // pop the right operand into rax
                    generator->pop("rbx"); // pop the left operand into rbx
                    generator->_out << "    add rax, rbx\n"; // add the two operands
                    generator->push("rax"); // push the result onto the stack
                }

                void operator()(const BinaryExpressionSubtract* subtract) const {
                    generator->generateExpression(subtract->left);
                    generator->generateExpression(subtract->right);
                    generator->pop("rax"); // pop the right operand into rax
                    generator->pop("rbx"); // pop the left operand into rbx
                    generator->_out << "    sub rbx, rax\n"; // subtract the two operands
                    generator->push("rbx"); // push the result onto the stack
                }

                void operator()(const BinaryExpressionMultiply* multiply) const {
                    generator->generateExpression(multiply->left);
                    generator->generateExpression(multiply->right);
                    generator->pop("rax"); // pop the right operand into rax
                    generator->pop("rbx"); // pop the left operand into rbx
                    generator->_out << "    mul rbx\n"; // multiply the two operands
                    generator->push("rax"); // push the result onto the stack
                }

                void operator()(const BinaryExpressionDivide* divide) const {
                    generator->generateExpression(divide->left);
                    generator->generateExpression(divide->right);
                    generator->pop("rbx"); // pop the right operand (divisor) into rbx
                    generator->pop("rax"); // pop the left operand (dividend) into rax
                    generator->_out << "    xor rdx, rdx\n"; // zero RDX for division
                    generator->_out << "    div rbx\n"; // divide rax by rbx
                    generator->push("rax"); // push the result onto the stack
                }
            };

            BinaryExpressionVisitor visitor{.generator = this};
            visit(visitor, binaryExpression->var);
        }

        void generateExpression(const NodeExpression* expression) {
            struct ExpressionVisitor {
                Generator* generator;

                void operator()(const NodeTerm* term) const {
                    generator->generateTerm(term);
                }

                void operator()(const BinaryExpression* binaryExpression) const {
                    generator->generateBinaryExpression(binaryExpression);
                }
            };

            ExpressionVisitor visitor{.generator = this};
            visit(visitor, expression->var);
        }

        void generateScope(const NodeScope* scope) {
            beginScope();
            for(const NodeStatement* statement : scope->statements) generateStatement(statement);
            endScope();
        }
        
        void generateStatement(const NodeStatement* statement) {
            struct StatementVisitor {
                Generator* generator;
                void operator()(const NodeExit* exit) const {
                    generator->generateExpression(exit->exp);
                    generator->_out << "    mov rax, 60\n"; // syscall number for exit
                    generator->pop("rdi"); // pop the top of the stack into rdi
                    generator->_out << "    syscall\n"; // make the syscall
                    generator->_hasExplicitExit = true;
                }

                void operator()(const NodeLet* let) const {
                    auto it = find_if(generator->_vars.cbegin(), generator->_vars.cend(), [&](const Var& var) {
                        return var.name == let->identifier.value;
                    });
                    if(it != generator->_vars.cend()) {
                        cerr << "Identifier `" << let->identifier.value << "` already exists!" << endl;
                        exit(EXIT_FAILURE);
                    }
                    generator->_vars.push_back({.name = let->identifier.value, .stackPos = generator->_stackSize});
                    generator->generateExpression(let->value);
                }

                void operator()(const NodeScope* scope) const {
                    generator->generateScope(scope);
                }

                void operator()(const NodeIf* _if) const {
                    string label = generator->createLabel();
                    generator->generateExpression(_if->condition);
                    generator->pop("rax"); // pop the condition result into rax
                    generator->_out << "    cmp rax, 0\n"; // compare rax
                    generator->_out << "    je " << label << "\n"; // jump to label
                    generator->generateScope(_if->scope); // generate the scope if condition is true
                    generator->_out << "\n" << label << ":\n"; // label for the end of the if statement
                }
            };

            StatementVisitor visitor{.generator = this};
            visit(visitor, statement->var);
        }

        [[nodiscard]] string generateProgram() {
            _out << "global _start\n_start:\n"; // initiatlize the sstream

            for(const NodeStatement* statement : _program.statements) generateStatement(statement);
            if (!_hasExplicitExit) {
                _out << "    mov rax, 60\n"; // syscall number for exit
                _out << "    mov rdi, 0\n"; // move the exit value of 0 to rdi
                _out << "    syscall\n"; // make the syscall
            }
            return _out.str();
        }

    private:
        const NodeProgram _program;
        stringstream _out;
        size_t _stackSize = 0;
        struct Var { string name; size_t stackPos; };
        vector<Var> _vars {};
        vector<size_t> _scopes {};

        void push(const string& reg) {
            _out << "    push " << reg << "\n"; // push the register onto the stack
            _stackSize++;
        }

        void pop(const string& reg) {
            _out << "    pop " << reg << "\n"; // pop the top of the stack into the register
            _stackSize--;
        }

        void beginScope() {
            _scopes.push_back(_vars.size());
        }

        void endScope() {
            size_t count = _vars.size() - _scopes.back();
            _out << "    add rsp, " << count * 8 << "\n";
            _stackSize -= count;
            _vars.erase(_vars.begin() + _scopes.back(), _vars.end());
            _scopes.pop_back();
        }

        string createLabel() {
            static size_t labelCount = 0;
            return "label_" + to_string(labelCount++);
        }
};