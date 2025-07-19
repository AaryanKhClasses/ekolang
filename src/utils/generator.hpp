#pragma once

#include <iostream>
#include <sstream>
#include "parser.hpp"

using namespace std;

class Generator {
    public:
        inline explicit Generator(NodeExit root): _root(move(root)) { }
        
        [[nodiscard]] string generate() const {
            stringstream out;
            out << "global _start\n_start:\n"; // initiatlize the sstream
            out << "    mov rax, 60\n"; // syscall number for exit
            out << "    mov rdi, " << _root.exp.number.value << "\n"; // move the exit value to rdi
            out << "    syscall\n"; // make the syscall
            return out.str();
        }

    private:
        const NodeExit _root;
};