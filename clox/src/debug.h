#pragma once
#include <cstdio>

#include "chunk.h"
#include "utils/common.h"

uint16_t disassembleInstruction(const Chunk& chunk, uint16_t offset, bool linesAvailable, uint16_t* scopeCount, OpCode* o_op = nullptr);

void disassemble(const Chunk& chunk, const char* name);
