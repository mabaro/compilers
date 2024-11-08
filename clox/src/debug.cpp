#include "debug.h"

#include <cstdio>

#include "chunk.h"

codepos_t simpleInstruction(const char* name, codepos_t offset)
{
    printf("%s\n", name);
    return offset + 1;
}
codepos_t byteInstruction(const char* name, const Chunk& chunk, codepos_t offset)
{
    const uint8_t* code          = chunk.getCode();
    const uint8_t  slot = code[offset + 1];
    printf("%-16s [%04d]='\n", name, slot);

    return offset + 2;
}
codepos_t constantInstruction(const char* name, const Chunk& chunk, codepos_t offset)
{
    const uint8_t* code          = chunk.getCode();
    const uint8_t  constantIndex = code[offset + 1];
    printf("%-16s [%04d]='", name, constantIndex);
    printValueDebug(chunk.getConstants().getValue(constantIndex));
    printf("'\n");

    return offset + 2;
}
codepos_t jumpInstruction(const char* name, const Chunk& chunk, codepos_t codePos)
{
    const uint8_t* code       = chunk.getCode();
    jump_t         jumpOffset = (uint16_t)(code[codePos + 1] << 8);
    jumpOffset |= code[codePos + 2];

    printf("%-16s %4d -> %d\n", name, codePos, codePos + 3 + jumpOffset);
    return codePos + 3;
}
codepos_t scopeInstruction(const char* name, const Chunk& /*chunk*/, uint16_t offset)
{
    printf("%s\n", name);
    return offset + 1;
}

codepos_t disassembleInstruction(const Chunk& chunk, codepos_t offset, bool linesAvailable, uint16_t* scopeCount,
                                 OpCode* o_op)
{
    ASSERT(offset < chunk.getCodeSize());
    const OpCode instruction = OpCode(chunk.getCode()[offset]);
    if (o_op != nullptr)
    {
        *o_op = instruction;
    }

    printf("%04d ", offset);
    if (instruction == OpCode::ScopeEnd)
    {
        if (*scopeCount > 0)
        {
            --(*scopeCount);
        // } else { // this should be a OpCode::Break
        //     printf("Unpaired Op::EndScope\n");
        }
    }

    for (int i = *scopeCount; i > 0; --i)
    {
        printf("#");
    }

    if (linesAvailable)
    {
        if (offset > 0u && chunk.getLine(offset) == chunk.getLine(offset - 1))
        {
            printf("| ");
        }
        else
        {
            printf("%zu ", chunk.getLine(offset));
        }
    }
    switch (instruction)
    {
        case OpCode::Return: return simpleInstruction("OP_RETURN", offset);
        case OpCode::Null: return simpleInstruction("OP_NULL", offset);
        case OpCode::True: return simpleInstruction("OP_TRUE", offset);
        case OpCode::False: return simpleInstruction("OP_FALSE", offset);
        case OpCode::Negate: return simpleInstruction("OP_NEGATE", offset);
        case OpCode::Not: return simpleInstruction("OP_NOT", offset);
        case OpCode::Add: return simpleInstruction("OP_ADD", offset);
        case OpCode::Subtract: return simpleInstruction("OP_SUBTRACT", offset);
        case OpCode::Multiply: return simpleInstruction("OP_MULTIPLY", offset);
        case OpCode::Divide: return simpleInstruction("OP_DIVIDE", offset);
        case OpCode::Assignment: return simpleInstruction("OP_ASSIGNMENT", offset);
        case OpCode::Equal: return simpleInstruction("OP_EQUAL", offset);
        case OpCode::Greater: return simpleInstruction("OP_GREATER", offset);
        case OpCode::Less: return simpleInstruction("OP_LESS", offset);
        case OpCode::Print: return simpleInstruction("OP_PRINT", offset);
        case OpCode::Pop: return simpleInstruction("OP_POP", offset);
        case OpCode::Constant: return constantInstruction("OP_CONSTANT", chunk, offset);
        case OpCode::GlobalVarDef: return constantInstruction("OP_GLOBAL_VAR_DEFINE", chunk, offset);
        case OpCode::GlobalVarSet: return constantInstruction("OP_LOCAL_VAR_SET", chunk, offset);
        case OpCode::GlobalVarGet: return constantInstruction("OP_GLOBAL_VAR_GET", chunk, offset);
        case OpCode::LocalVarSet: return byteInstruction("OP_LOCAL_VAR_SET", chunk, offset);
        case OpCode::LocalVarGet: return byteInstruction("OP_LOCAL_VAR_GET", chunk, offset);
        case OpCode::Jump: return jumpInstruction("OP_JUMP", chunk, offset);
        case OpCode::JumpIfFalse: return jumpInstruction("OP_JUMP_IF_FALSE", chunk, offset);
        case OpCode::JumpIfTrue: return jumpInstruction("OP_JUMP_IF_TRUE", chunk, offset);
        case OpCode::ScopeBegin: ++(*scopeCount); return scopeInstruction("OP_SCOPE_BEGIN", chunk, offset);
        case OpCode::ScopeEnd: return scopeInstruction("OP_SCOPE_END", chunk, offset);
        default: printf("Unknown opcode %d\n", (int)instruction); return offset + 1;
    }
}

void disassemble(const Chunk& chunk, const char* name)
{
    printf("== %s ==\n", name);

    const bool linesAvailable = chunk.getLineCount() > 0;
    uint16_t   scopeCount     = 0;
    for (codepos_t offset = 0; offset < chunk.getCodeSize();)
    {
        offset = static_cast<codepos_t>(
            disassembleInstruction(chunk, static_cast<uint16_t>(offset), linesAvailable, &scopeCount));
    }
}
