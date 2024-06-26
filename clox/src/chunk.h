#pragma once

#include <string>
#include <vector>

#include "common.h"
#include "value.h"

enum class OpCode
{
    Return,
    Constant,

    // Literal ops
    Null,
    True,
    False,

    // Unary ops
    Negate,
    Not,

    // Binary ops
    Assignment,
    Equal,
    Greater,
    Less,

    // Arithmetic
    Add,
    Subtract,
    Multiply,
    Divide,

    Skip,  // helper for semicolon

    Undefined = 0x0FF
};

struct Chunk
{
    using ValueArray = RandomAccessContainer<Value>;

    Chunk(const char* sourcePath) : _sourcepath(sourcePath) {}
    ~Chunk() {}

    const char* getSourcePath() const { return _sourcepath.c_str(); }
    const uint8_t* getCode() const { return _code.data(); }
    const uint16_t* getLines() const { return _lines.data(); }
    const uint16_t getLine(uint16_t index) const { return _lines[index]; }
    const size_t getCodeSize() const { return _code.size(); }

    const ValueArray& getConstants() const { return _constants; }
    const ValueArray& getVariables() const { return _variables; }

    void init()
    {
        _code.clear();
        _lines.clear();
    }
    void write(OpCode code, uint16_t line) { write((uint8_t)code, line); }
    void write(uint8_t byte, uint16_t line)
    {
        _code.push_back(byte);
        _lines.push_back(line);
    }
    int addConstant(const Value& value)
    {
        _constants.write(value);
        return static_cast<int>(_constants.getSize()) - 1;
    }
    int addVariable(const Value& value)
    {
        _variables.write(value);
        return static_cast<int>(_variables.getSize()) - 1;
    }

#if DEBUG_TRACE_EXECUTION
   public:  // helpers
    void printConstants() const
    {
        printf(" Constants: ");
        for (size_t i = 0; i < getConstants().getSize(); ++i)
        {
            printf("%zu[", i);
            printValue(getConstants()[i]);
            printf("]");
        }
        printf("\n");
    }
#endif  // #if DEBUG_TRACE_EXECUTION

   protected:
    std::string _sourcepath;
    std::vector<uint8_t> _code;
    std::vector<uint16_t> _lines;
    ValueArray _constants;
    ValueArray _variables;
};