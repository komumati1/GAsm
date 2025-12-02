//
// Parses the program and runs it
//

#ifndef GASM_GASMPARSER_H
#define GASM_GASMPARSER_H

#define INSTRUCTION_GROUPS 7

#define MOV_GROUP_LENGTH 6
#define MOV_GROUP 0x00,0x01,0x02,0x03,0x04,0x05
#define MOV_P_A 0x00
#define MOV_A_P 0x01
#define MOV_A_R 0x02
#define MOV_A_I 0x03
#define MOV_R_A 0x04
#define MOV_I_A 0x05

#define ARITHMETIC_R_GROUP_LENGTH 7
#define ARITHMETIC_R_GROUP 0x10,0x11,0x12,0x13,0x14,0x15,0x16
#define ADD_R 0x10
#define SUB_R 0x11
#define DIV_R 0x12
#define MUL_R 0x13
#define SIN_R 0x14
#define COS_R 0x15
#define EXP_R 0x16

#define ARITHMETIC_I_GROUP_LENGTH 7
#define ARITHMETIC_I_GROUP 0x20,0x21,0x22,0x23,0x24,0x25,0x26
#define ADD_I 0x20
#define SUB_I 0x21
#define DIV_I 0x22
#define MUL_I 0x23
#define SIN_I 0x24
#define COS_I 0x25
#define EXP_I 0x26

#define UNARY_GROUP_LENGTH 4
#define UNARY_GROUP 0x30,0x31,0x32,0x33
#define INC 0x30
#define DEC 0x31
#define RES 0x32
#define SET 0x33

#define LOOP_GROUP_LENGTH 3
#define LOOP_GROUP 0x40,0x41,0x42
#define FOR 0x40
#define LOP_A 0x41
#define LOP_P 0x42

#define IF_GROUP_LENGTH 3
#define IF_GROUP 0x50,0x51,0x52
#define JMP_I 0x50
#define JMP_R 0x51
#define JMP_P 0x52

#define TERMINATION_GROUP_LENGTH 2
#define TERMINATION_GROUP 0x60,0x61
#define END 0x60
#define RNG 0x61

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

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
    static constexpr int instructionGroups = INSTRUCTION_GROUPS;
    static constexpr int instructionGroupLengths[INSTRUCTION_GROUPS] = {
            MOV_GROUP_LENGTH,ARITHMETIC_R_GROUP_LENGTH,ARITHMETIC_I_GROUP_LENGTH,
            UNARY_GROUP_LENGTH,LOOP_GROUP_LENGTH,IF_GROUP_LENGTH,TERMINATION_GROUP_LENGTH
    };
    static constexpr uint8_t end = END;
    static constexpr size_t normalOpcodesLength = MOV_GROUP_LENGTH + ARITHMETIC_R_GROUP_LENGTH +
                                                  ARITHMETIC_I_GROUP_LENGTH + UNARY_GROUP_LENGTH + 1;
    static constexpr uint8_t normalOpcodes[normalOpcodesLength] = {
            MOV_GROUP,ARITHMETIC_R_GROUP,ARITHMETIC_I_GROUP,UNARY_GROUP,RNG
    };
    static constexpr size_t structuralOpcodesLength = LOOP_GROUP_LENGTH + IF_GROUP_LENGTH;
    static constexpr uint8_t structuralOpcodes[structuralOpcodesLength] = {
            LOOP_GROUP,IF_GROUP
    };

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

    static uint8_t base322Bytecode(uint8_t base32);

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
