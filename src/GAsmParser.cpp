//
// Created by mateu on 14.11.2025.
//

#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <iostream>
#include <cmath>
#include "../include/GAsmParser.h"


// initialize natural language to bytecode
const std::unordered_map<std::string, uint8_t> GAsmParser::_string2Opcode = {
        { "MOV P, A", 0x00 },
        { "MOV A, P", 0x01 },
        { "MOV A, R", 0x02 },
        { "MOV A, I", 0x03 },
        { "MOV R, A", 0x04 },
        { "MOV I, A", 0x05 },

        { "ADD R", 0x10 },
        { "SUB R", 0x11 },
        { "DIV R", 0x12 },
        { "MUL R", 0x13 },
        { "SIN R", 0x14 },
        { "COS R", 0x15 },
        { "EXP R", 0x16 },

        { "ADD I", 0x20 },
        { "SUB I", 0x21 },
        { "DIV I", 0x22 },
        { "MUL I", 0x23 },
        { "SIN I", 0x24 },
        { "COS I", 0x25 },
        { "EXP I", 0x26 },

        { "INC", 0x30 },
        { "DEC", 0x31 },
        { "RES", 0x32 },

        { "LOP", 0x40 },
        { "END", 0x41 },

        { "JMP I", 0x50 },
        { "JMP R", 0x51 },
        { "JMP P", 0x52 },

        {"NOP", 0x60},
        {"NO1", 0x61},
        {"NO2", 0x62},
        {"NO3", 0x63},
};

// the rest of conversion initializers based on the natural language
std::string* GAsmParser::initOpcode2String() {
    auto* const opcode2String = new std::string[256];
    for (auto &p: GAsmParser::_string2Opcode) {
        opcode2String[p.second] = p.first;
    }
    return opcode2String;
}

std::string* const GAsmParser::_opcode2String = GAsmParser::initOpcode2String();

uint8_t* GAsmParser::initOpcode2Base32() {
    auto opcode2Base32 = new uint8_t[256];
    std::fill(opcode2Base32, opcode2Base32 + 256, 255); // fill with max
    int i = 0;
    for (auto &p: GAsmParser::_string2Opcode) {
        opcode2Base32[p.second] = i;
        i++;
    }
    return opcode2Base32;
}

std::uint8_t* const GAsmParser::_opcode2Base32 = GAsmParser::initOpcode2Base32();

std::uint8_t *GAsmParser::initBase322Opcode() {
    auto* const base322Opcode = new uint8_t[32];
    for (int i = 0; i < 256; i++) {
        if (GAsmParser::_opcode2Base32[i] == 255) continue; // skip the max
        base322Opcode[GAsmParser::_opcode2Base32[i]] = i;
    }
    return base322Opcode;
}

std::uint8_t* const GAsmParser::_base322Opcode = GAsmParser::initBase322Opcode();

// Constructors
GAsmParser::GAsmParser(const std::string &filename) : _bytecode(nullptr), _length(0) {
    auto size = std::filesystem::file_size(filename);
    std::string content(size, '\0');
    std::ifstream file(filename);
    file.read(&content[0], (long long)size);
    file.close();
    _bytecode = GAsmParser::text2Bytecode(content, this->_length);
}

GAsmParser::GAsmParser(const uint8_t *bytecode, size_t length) : _bytecode(nullptr), _length(0) {
    if (length == 0) {
        return;
    }
    if (bytecode == nullptr) {
        throw std::invalid_argument("GAsmParser: bytecode is null but length > 0");
    }
    _bytecode = new uint8_t[length];
    std::copy_n(bytecode, length, _bytecode);
    _length = length;
}

GAsmParser::GAsmParser(const GAsmParser &other) : _bytecode(nullptr), _length(0) {
    _bytecode = new uint8_t[other._length];
    std::copy_n(other._bytecode, other._length, _bytecode);
    _length = other._length;
}

GAsmParser& GAsmParser::operator=(const GAsmParser& other) {
    if (this != &other) {
        delete[] _bytecode;
        _length = 0;
        _bytecode = nullptr;
        _bytecode = new uint8_t[other._length];
        std::copy_n(other._bytecode, other._length, _bytecode);
        _length = other._length;
    }
    return *this;
}

GAsmParser::GAsmParser(GAsmParser &&other) noexcept : _bytecode(other._bytecode), _length(other._length) {
    other._bytecode = nullptr;
    other._length = 0;
}

GAsmParser& GAsmParser::operator=(GAsmParser&& other) noexcept {
    if (this != &other) {
        delete[] _bytecode;
        _bytecode = other._bytecode;
        _length = other._length;
        other._bytecode = nullptr;
        other._length = 0;
    }

    return *this;
}

GAsmParser::~GAsmParser() {
    delete[] _bytecode;
}

std::ostream &operator<<(std::ostream &os, const GAsmParser &gAsmParser) {
    return os << GAsmParser::bytecode2Text(gAsmParser._bytecode, gAsmParser._length);
}


// conversions
uint8_t* GAsmParser::text2Bytecode(const std::string& text, size_t& length) {
    // Make text uppercase
    std::string upper = text;
    std::transform(
            upper.begin(), upper.end(),
            upper.begin(),
            [](unsigned char c){ return std::toupper(c); }
    );

    // Split by newline
    std::istringstream iss(upper);
    std::string line;
    std::vector<uint8_t> codes;

    while (std::getline(iss, line)) {
        // 1. Remove every whitespace and non-printable character
        {
            std::string filtered;
            filtered.reserve(line.size());
            for (unsigned char c : line) {
                if (std::isprint(c) && !std::isspace(c))
                    filtered.push_back((char) c);
            }
            line.swap(filtered);
        }
        // If empty after filtering → continue
        if (line.empty())
            continue;
        // 2. Detect // and remove everything after it
        {
            size_t commentPos = line.find("//");
            if (commentPos != std::string::npos) {
                line.erase(commentPos);
            }
        }
        if (line.empty())
            continue;
        // 3. After the 3rd letter, add one space
        if (line.size() > 3) {
            line.insert(3, " ");
        }
        // 4. If the string contains a comma, add one space after it
        {
            size_t pos = line.find(',');
            if (pos != std::string::npos && pos + 1 < line.size()) {
                // Insert space only if not already a space
                if (line[pos + 1] != ' ')
                    line.insert(pos + 1, " ");
            }
        }
        // 5. Lookup opcode
        auto it = _string2Opcode.find(line); // lookup uses only instruction name
        if (it == _string2Opcode.end()) {
            throw std::runtime_error("Unknown instruction: " + line);
        }

        codes.push_back(it->second);
    }


    // Allocate byte buffer
    length = codes.size();
    auto* buffer = new uint8_t[length];

    for (size_t i = 0; i < length; ++i)
        buffer[i] = codes[i];

    return buffer;
}

std::string GAsmParser::bytecode2Text(const uint8_t *bytecode, size_t length) {
    std::string out;
    out.reserve(length * 8); // pre-allocate some reasonable space

    for (size_t i = 0; i < length; i++) {
        uint8_t op = bytecode[i];
        const std::string text = _opcode2String[op];

        if (text.empty()) {
            // Unknown opcode → print as hex comment
            out += "UNKNOWN_0x";
            char buf[5];
            snprintf(buf, sizeof(buf), "%02X", op);
            out += buf;
        } else {
            out += text;
        }

        if (i + 1 < length)
            out += '\n';
    }
    return out;
}

uint8_t *GAsmParser::base322Bytecode(const uint8_t *base32, size_t length) {
    auto* bytecode = new uint8_t[length];
    for (int i = 0; i < length; i++) {
        bytecode[i] = _base322Opcode[base32[i]];
    }
    return bytecode;
}

uint8_t *GAsmParser::bytecode2Base32(const uint8_t *bytecode, size_t length) {
    auto* base32 = new uint8_t[length];
    for (int i = 0; i < length; i++) {
        base32[i] = _opcode2Base32[bytecode[i]];
    }
    return base32;
}

uint8_t *GAsmParser::ascii2Bytecode(const std::string &ascii, size_t& length) {
    auto* bytecode = new uint8_t[ascii.length()];
    length = ascii.length();
    for (int i = 0; i < length; i++) {
        bytecode[i] = (uint8_t)(_base322Opcode[ascii.at(i) - 65]);
    }
    return bytecode;
}

std::string GAsmParser::bytecode2Ascii(const uint8_t *bytecode, size_t length) {
    auto ascii = std::string();
    for (int i = 0; i < length; i++) {
        ascii.push_back((char)(_opcode2Base32[bytecode[i]] + 65)); // offset to A
    }
    return ascii;
}

uint64_t *
GAsmParser::zip(const uint8_t *bytecode, size_t bytecode_length, size_t &zipped_length) {
    size_t _zipped_length = std::ceil((double) bytecode_length / 8.0 * 5.0 / 64.0);
    auto* zipped = new uint64_t[_zipped_length];
    zipped_length = _zipped_length;
    {
        int zi = 0;
        int buf_length = 0;
        uint64_t buffer = 0;
        uint8_t base32;

        for (int bi = 0; bi < bytecode_length; bi++) {
            base32 = _opcode2Base32[bytecode[bi]];
            if (buf_length < 60) { // we can still insert data
                // append data at the end
                buffer = buffer | base32;
                buffer <<= 5;
                buf_length += 5;
            } else if (buf_length == 64) { // perfect data fit
                zipped[zi] = buffer;
                buf_length = 0;
                buffer = 0;
                zi++;
            } else { // handle 64bit overflow, save to zipped, append the reminder
                buffer = buffer | base32 >> (buf_length - 59); // shift by reminder
                zipped[zi] = buffer;
                buffer = base32;
                buffer <<= (buf_length - 59 + 5);
                buf_length = 5 + (buf_length - 59);
                zi++;
            }
        }
        // insert last data if necessary
        if (zi < zipped_length) {
            buffer <<= (59 - buf_length);
            zipped[zi] = buffer;
        }
    }
    return zipped;
}

uint8_t *
GAsmParser::unzip(const uint64_t *zipped, size_t bytecode_length, size_t zipped_length) {
    auto* bytecode = new uint8_t[bytecode_length];
    {
        int zi = 0;
        int buf_length = 64;
        uint8_t base32;
        uint64_t buffer;

        for (int bi = 0; bi < bytecode_length; bi++) {
            buf_length -= 5;
            buffer = zipped[zi];
            if (buf_length < 0) {
                buf_length *= -1;
                buffer <<= buf_length;
                base32 = buffer;
                zi++;
                buf_length = 64 - buf_length;
                buffer = zipped[zi];
                buffer >>= buf_length;
                base32 |= buffer;
            } else {
                buffer >>= buf_length;
                base32 = buffer;
            }
            base32 &= 0b0011111;
            bytecode[bi] = _base322Opcode[base32];
        }
    }
    return bytecode;
}

uint8_t GAsmParser::base322Bytecode(const uint8_t base32) {
    return GAsmParser::_base322Opcode[base32];
}
