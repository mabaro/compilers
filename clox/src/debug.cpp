#include "debug.h"

#include <cstdio>

#include "chunk.h"

int simpleInstruction(const char* name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}
int constantInstruction(const char* name, const Chunk& chunk, int offset)
{
    const uint8_t constantIndex = chunk.getCode()[offset + 1];
    printf("%-16s [%04d]='", name, constantIndex);
    printValue(chunk.getConstants().getValue(constantIndex));
    printf("'\n");

    return offset + 2;
}

int disassembleInstruction(const Chunk& chunk, uint16_t offset)
{
    ASSERT(offset < chunk.getCodeSize());
    printf("%04d ", offset);
    if (offset > 0 && chunk.getLine(offset) == chunk.getLine(offset - 1))
    {
        printf(" | ");
    }
    else
    {
        printf("%u ", chunk.getLine(offset));
    }

    const OpCode instruction = OpCode(chunk.getCode()[offset]);
    switch (instruction)
    {
        case OpCode::Return:
            return simpleInstruction("OP_RETURN", offset);
        case OpCode::Null:
            return simpleInstruction("OP_NULL", offset);
        case OpCode::True:
            return simpleInstruction("OP_TRUE", offset);
        case OpCode::False:
            return simpleInstruction("OP_FALSE", offset);
        case OpCode::Negate:
            return simpleInstruction("OP_NEGATE", offset);
        case OpCode::Not:
            return simpleInstruction("OP_NOT", offset);
        case OpCode::Add:
            return simpleInstruction("OP_ADD", offset);
        case OpCode::Subtract:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OpCode::Multiply:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OpCode::Divide:
            return simpleInstruction("OP_DIVIDE", offset);
        case OpCode::Assignment:
            return simpleInstruction("OP_ASSIGNMENT", offset);
        case OpCode::Equal:
            return simpleInstruction("OP_EQUAL", offset);
        case OpCode::Greater:
            return simpleInstruction("OP_GREATER", offset);
        case OpCode::Less:
            return simpleInstruction("OP_LESS", offset);
        case OpCode::Print:
            return simpleInstruction("OP_PRINT", offset);
        case OpCode::Pop:
            return simpleInstruction("OP_POP", offset);
        case OpCode::Constant:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OpCode::GlobalVarDef:
            return constantInstruction("OP_GLOBAL_VAR_DEFINE", chunk, offset);
        case OpCode::GlobalVarGet:
            return constantInstruction("OP_GLOBAL_VAR_GET", chunk, offset);
         case OpCode::GlobalVarSet:
            return constantInstruction("OP_GLOBAL_VAR_SET", chunk, offset);
        default:
            printf("Unknown opcode %d\n", (int)instruction);
            return offset + 1;
    }
}

void disassemble(const Chunk& chunk, const char* name)
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk.getCodeSize();)
    {
        offset = disassembleInstruction(chunk, offset);
    }
}
