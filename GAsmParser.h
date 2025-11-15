//
// Parses the program and runs it
//

#ifndef GASM_GASMPARSER_H
#define GASM_GASMPARSER_H


#include <cstdint>
#include <string>
#include <unordered_map>

class GAsmParser {
private:
    uint8_t* _bytecode;
    size_t _length;
    static const std::unordered_map<std::string, uint8_t> _string2Opcode;
    static std::string* const _opcode2String;
    static uint8_t* const _opcode2Base32;
    static uint8_t* const _base322Opcode;
    static std::string* initOpcode2String();
    static uint8_t* initOpcode2Base32();
    static uint8_t* initBase322Opcode();
public:
    explicit GAsmParser(const std::string& filename);
    GAsmParser(const uint8_t* bytecode, size_t length);
    GAsmParser(const GAsmParser& other);
    GAsmParser& operator=(const GAsmParser& other);
    GAsmParser(GAsmParser&& other) noexcept;
    GAsmParser& operator=(GAsmParser&& other) noexcept;
    ~GAsmParser();

    [[nodiscard]] size_t length() const {return _length;}
    [[nodiscard]] uint8_t* bytecode() const {return _bytecode;}

//    std::string toText();
//    std::uint8_t* toBase32();
//    std::string toAscii();
//    std::uint64_t* zip();

    static uint8_t* text2Bytecode(const std::string& text, size_t& length);
    static std::string bytecode2Text(const uint8_t* bytecode, size_t length);
    static uint8_t* base322Bytecode(const uint8_t* base32, size_t length);
    static uint8_t* bytecode2Base32(const uint8_t* bytecode, size_t length);
    static uint8_t* ascii2Bytecode(const std::string& ascii, size_t& length);
    static std::string bytecode2Ascii(const uint8_t* bytecode, size_t length);
    static uint64_t* zip(const uint8_t* bytecode, size_t bytecode_length, size_t& zipped_length);

    /**
     * @brief yes
     *
     * yes yes
     *
     * @param zipped
     * @param bytecode_length: you must know the length of the individual
     * @param output_length
     *
     * @return
     */
    static uint8_t* unzip(const uint64_t* zipped, size_t bytecode_length, size_t zipped_length);

    friend std::ostream& operator<<(std::ostream& os, const GAsmParser& gAsmParser);
};

#endif //GASM_GASMPARSER_H
