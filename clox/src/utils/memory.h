#pragma once

#include <cstdlib>

#define ALLOCATE(Type) (Type *)malloc(sizeof(Type))
#define ALLOCATE_FLEX(Type, ExtraSize) (Type *)malloc(sizeof(Type) + ExtraSize)
#define ALLOCATE_N(Type, count) (Type *)malloc(sizeof(Type) * count)
#define REALLOCATE_FLEX(MEM, Type, ExtraSize) (Type *)realloc(MEM, sizeof(Type) + ExtraSize)
#define DEALLOCATE(Type, pointer) free(pointer)
#define DEALLOCATE_N(Type, pointer, N) free(pointer)