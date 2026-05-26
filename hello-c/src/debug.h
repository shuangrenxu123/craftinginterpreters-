#ifndef DEBUG_H
#define DEBUG_H

#include "chunk.h"
#include "scanner.h"

const char *tokenTypeName(tokenType type);
void printTokenType(tokenType type);
void disassembleChunk(chunk *chunk, const char *name);
int disassembleInstruction(chunk *chunk, int offset);
#endif
