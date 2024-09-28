#pragma once

#include <string>
#include <vector>

#include "common.h"
#include "value.h"

#include "named_enum.hpp"

MAKE_NAMED_ENUM_CLASS_WITH_TYPE(OpCode, uint8_t,
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

    // Core methods
    Print,

    Variable,
    GlobalVarDef,
    GlobalVarGet,
    GlobalVarSet,

    Pop,
    
    Skip,  // helper for semicolon

    Undefined// = 0x0FF
);

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
};