#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "utils/generator.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    if(argc != 2) {
        cerr << "Incorrect Usage of the Tool!\nCorrect Usage: \"eko <file_name.eko>\"" << endl;
        return EXIT_FAILURE;
    }
    
    string contents;
    {
        stringstream content_stream;
        fstream input(argv[1], ios::in);
        content_stream << input.rdbuf();
        contents = content_stream.str();
        input.close();
    }

    Tokenizer tokenizer(move(contents));
    vector<Token> tokens = tokenizer.tokenize();

    Parser parser(move(tokens));
    optional<NodeExit> tree = parser.parse();

    if(!tree.has_value()) {
        cerr << "Failed to parse the input file." << endl;
        exit(EXIT_FAILURE);
    }

    Generator generator(tree.value());

    {
        fstream output("../out/output.asm", ios::out);
        output << generator.generate();
        output.close();
    }

    system("nasm -felf64 ../out/output.asm");
    system("ld -o ../out/output ../out/output.o");

    return EXIT_SUCCESS;
}