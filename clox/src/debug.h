#pragma once
#include <cstdio>

#include "chunk.h"
#include "utils/common.h"

int disassembleInstruction(const Chunk& chunk, uint16_t offset, bool linesAvailable);

void disassemble(const Chunk& chunk, const char* name);
