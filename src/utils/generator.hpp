#pragma once

#include <iostream>
#include <sstream>
#include <unordered_map>
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
                    if(!generator->_vars.contains(identifier->identifier.value)) {
                        cerr << "Invalid Syntax: Identifier `" << identifier->identifier.value << "` does not exist!" << endl;
                        exit(EXIT_FAILURE);
                    }
                    const auto& var = generator->_vars.at(identifier->identifier.value);
                    stringstream offset;
                    offset << "QWORD [rsp + " << (generator->_stackSize - var.stackPos - 1) * 8 << "]";
                    generator->push(offset.str()); // push the value onto the stack
                }
            };

            TermVisitor visitor{.generator = this};
            visit(visitor, term->var);
        }

        void generateExpression(const NodeExpression* expression) {
            struct ExpressionVisitor {
                Generator* generator;

                void operator()(const NodeTerm* term) const {
                    generator->generateTerm(term);
                }

                void operator()(const BinaryExpression* binaryExpression) const {
                    generator->generateExpression(binaryExpression->add->left);
                    generator->generateExpression(binaryExpression->add->right);
                    generator->pop("rax"); // pop the right operand into rax
                    generator->pop("rbx"); // pop the left operand into rbx
                    generator->_out << "    add rax, rbx\n"; // add the two operands
                    generator->push("rax"); // push the result onto the stack
                }
            };

            ExpressionVisitor visitor{.generator = this};
            visit(visitor, expression->var);
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
                    if(generator->_vars.contains(let->identifier.value)) {
                        cerr << "Identifier `" << let->identifier.value << "` already exists!" << endl;
                        exit(EXIT_FAILURE);
                    }
                    generator->_vars.insert({let->identifier.value, Var {.stackPos = generator->_stackSize} });
                    generator->generateExpression(let->value);
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
        struct Var { size_t stackPos; };
        unordered_map<string, Var> _vars {};

        void push(const string& reg) {
            _out << "    push " << reg << "\n"; // push the register onto the stack
            _stackSize++;
        }

        void pop(const string& reg) {
            _out << "    pop " << reg << "\n"; // pop the top of the stack into the register
            _stackSize--;
        }
};