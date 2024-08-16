#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

#include "tokenizer.hpp"
#include "parser.hpp"
#include "generator.hpp"

int main(int argc, char* argv[]) {
    if(argc != 2) {
        std::cerr << "No input files provided" << std::endl;
        return EXIT_FAILURE;
    }

    std::fstream input(argv[1], std::ios::in);
    std::stringstream tempBuffer;
    tempBuffer << input.rdbuf();

    input.close();

    std::string buffer = tempBuffer.str();

    Tokenizer tokenizer(std::move(buffer));
    std::vector<Token> tokens = tokenizer.tokenize();

    Parser parser(std::move(tokens));
    std::optional<NodeProg> tree = parser.parse_prog();

    if(!tree.has_value()) {
        std::cerr << "Parsing failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    Generator generator(tree.value());

    std::fstream file("out.asm", std::ios::out);
    file << generator.gen_prog();
    file.close();

    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}
