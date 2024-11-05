#pragma once

#include <string>
#include <vector>

#include "third_party/named_enum.hpp"
#include "utils/common.h"
#include "value.h"

MAKE_NAMED_ENUM_CLASS_WITH_TYPE(OpCode, uint8_t, Return, Constant,

                                // Literal ops
                                Null, True, False,

                                // Unary ops
                                Negate, Not,

                                // Binary ops
                                Assignment, Equal, Greater, Less,

                                // Arithmetic
                                Add, Subtract, Multiply, Divide,

                                // Core methods
                                Print,

                                // Global vars
                                GlobalVarDef, GlobalVarSet, GlobalVarGet,
                                // Local vars
                                LocalVarSet, LocalVarGet,

                                Pop,

                                Skip,  // helper for semicolon

                                // flow control
                                Jump, JumpIfFalse, JumpIfTrue,

                                ScopeBegin, ScopeEnd,

                                Undefined  // = 0x0FF
);

struct Chunk
{
    using ValueArray = RandomAccessContainer<Value>;

    Chunk(const char* sourcePath) : _sourcepath(sourcePath) {}

    ~Chunk() {}

    Chunk(const Chunk&)            = delete;
    Chunk(Chunk&&)                 = delete;
    Chunk& operator=(Chunk&&)      = delete;
    Chunk& operator=(const Chunk&) = delete;

    Result<void> serialize(std::ostream& o_stream) const;
    Result<void> deserialize(std::istream& i_stream);

    const char* getSourcePath() const { return _sourcepath.c_str(); }

    opcode_t* getCodeMut() { return _code.data(); }

    const opcode_t* getCode() const { return _code.data(); }

    codepos_t getCodeSize() const { return static_cast<codepos_t>(_code.size()); }

    size_t getLine(codepos_t codePos) const
    {
        size_t line = 0;
        if (_lines.empty())
        {
            return 0;
        }

        while (_lines[line] < codePos)
        {
            ++line;
        }
        return line;
    }

    size_t getLineCount() const { return _lines.size(); }

    const ValueArray& getConstants() const { return _constants; }

    void init()
    {
        _code.clear();
        _lines.clear();
    }

    void write(OpCode code, size_t line) { write((uint8_t)code, line); }

    void write(uint8_t byte, size_t line)
    {
        _code.push_back(byte);
        if (_lines.size() > line)
        {
            _lines.back() = static_cast<uint16_t>(_code.size());
        }
        else
        {
            _lines.push_back(static_cast<uint16_t>(_code.size()));
        }
    }

    int addConstant(const Value& value)
    {
        ASSERT_MSG(_constants.size() <= MAX_OPCODE_VALUE,
                   format("#constants(%d) > MaxConstants(%d)\n", _constants.size(), MAX_OPCODE_VALUE).c_str());
        auto constantIt = std::find_if(_constants.begin(), _constants.end(),
                                       [&value](const Value& constant) { return constant == value; });
        if (constantIt != _constants.end())
        {
            const ptrdiff_t constantIndex = std::distance(_constants.begin(), constantIt);
            return static_cast<int>(constantIndex);
        }

        _constants.write(value);
        return static_cast<int>(_constants.size()) - 1;
    }

#if DEBUG_TRACE_EXECUTION
   public:  // helpers
    void printConstants(const char* padding = "") const
    {
        if (getConstants().size() == 0)
        {
            return;
        }

        printf(padding);
        printf("Constants: ");
        for (size_t i = 0; i < getConstants().size(); ++i)
        {
            printf("%zu[", i);
            printValueDebug(getConstants()[i]);
            printf("]");
        }
        printf("\n");
    }
#endif  // #if DEBUG_TRACE_EXECUTION

   protected:
    std::string           _sourcepath;
    std::vector<opcode_t> _code;
    std::vector<uint16_t> _lines;
    ValueArray            _constants;
};