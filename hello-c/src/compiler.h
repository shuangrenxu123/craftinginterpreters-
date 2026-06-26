#ifndef COMPILER_H
#define COMPILER_H
#include <stdbool.h>
#include "chunk.h"
#include "object.h"
ObjFunction *compile(const char *source);

#endif
