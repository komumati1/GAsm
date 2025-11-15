#include <iostream>
#include "GAsmParser.h"
#include "GAsm.h"

int main() {
    auto gasm = GAsmParser("../asm.txt");
    size_t length = 128;
    size_t zipped_length = 10;
    auto* zipped = new uint64_t[zipped_length];
    uint8_t *bytecode = GAsmParser::unzip(zipped, length, zipped_length);
    std::cout << GAsmParser::bytecode2Text(bytecode, length) << std::endl;
    return 0;
}
