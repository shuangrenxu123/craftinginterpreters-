#ifndef COMPILER_H
#define COMPILER_H
#include <stdbool.h>
#include "chunk.h"
bool compile(const char *source, chunk *chunk);

#endif
