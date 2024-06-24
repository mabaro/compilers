#pragma once
#include <cstdio>

#include "chunk.h"
#include "common.h"

int disassembleInstruction(const Chunk& chunk, uint16_t offset);

void disassemble(const Chunk& chunk, const char* name);
