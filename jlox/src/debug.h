#pragma once
#include "common.h"
#include "chunk.h"
#include <cstdio>

static void printValue(Value value) {
  printf("%g", value);
}
static int simpleInstruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}
static int constantInstruction(const char* name, const Chunk& chunk, int offset) {
  const uint8_t constantIndex = chunk.getCode()[offset + 1];
  printf("%-16s %4d '", name, constantIndex);
  printValue(chunk.getConstants().getValue(constantIndex));
  printf("'\n");

  return offset + 2;
}

static int disassembleInstruction(const Chunk& chunk, uint16_t offset);

static void disassemble(const Chunk& chunk, const char *name)
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk.getCodeSize();)
    {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int disassembleInstruction(const Chunk& chunk, uint16_t offset)
{
    assert(offset < chunk.getCodeSize());
    printf("%04d ", offset);
    if (offset > 0 && chunk.getLine(offset) == chunk.getLine(offset - 1)) {
        printf("   | ");
    }
    else {
        printf("%u ", chunk.getLine(offset));
    }

    const uint8_t instruction = chunk.getCode()[offset];
    switch (instruction)
    {
    case (uint8_t)OpCode::Return:
        return simpleInstruction("OP_RETURN", offset);
    case (uint8_t)OpCode::Constant:
        return constantInstruction("OP_CONSTANT", chunk, offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}
