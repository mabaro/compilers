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
    Chunk& operator=(Chunk&& ) = delete;
    Chunk& operator=(const Chunk& ) = delete;

    Result<void> serialize(std::ostream& o_stream) const;
    Result<void> deserialize(std::istream& i_stream);

    const char* getSourcePath() const { return _sourcepath.c_str(); }

    const uint8_t* getCode() const { return _code.data(); }
    const size_t getCodeSize() const { return _code.size(); }

    const uint16_t* getLines() const { return _lines.data(); }
    const uint16_t getLine(uint16_t index) const { return _lines[index]; }
    const size_t getLineCount() const { return _lines.size(); }

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
        auto constantIt = std::find_if(_constants.begin(), _constants.end(), [&value](const Value& constant){
            return constant == value;
        } );
        if (constantIt != _constants.end())
        {
            const size_t constantIndex = std::distance(_constants.begin(), constantIt);
            return static_cast<int>(constantIndex);
        }

        _constants.write(value);
        return static_cast<int>(_constants.size()) - 1;
    }

#if DEBUG_TRACE_EXECUTION
   public:  // helpers
    void printConstants() const
    {
        printf(" Constants: ");
        for (size_t i = 0; i < getConstants().size(); ++i)
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