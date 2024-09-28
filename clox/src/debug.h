#pragma once
#include <cstdio>

#include "common.h"
#include "chunk.h"

int disassembleInstruction(const Chunk& chunk, uint16_t offset);

void disassemble(const Chunk& chunk, const char* name);
