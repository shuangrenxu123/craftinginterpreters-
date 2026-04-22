#ifndef DEBUG_H
#define DEBUG_H

#include "chunk.h"
void disassembleChunk(chunk *chunk, const char *name);
int disassembleInstruction(chunk *chunk, int offset);
#endif