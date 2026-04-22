#ifndef CHUNK_h
#define CHUNK_h

#include "common.h"
#include "value.h"
typedef struct
{
    int line;
    int startIndex;
    int endIndex;
    int count;
} lineInfo;

typedef enum
{
    OP_NEGATE, // 取反

    OP_ADD,      // +
    OP_SUBTRACT, // -
    OP_MULTIPLY, // *
    OP_DIVIDE,   // /

    OP_CONSTANT_LONG, // 扩容常量
    OP_CONSTANT,      // 常量
    OP_RETURN,
} OpCode;

typedef struct
{
    uint8_t *code;
    valueArray constants;
    int count;
    int capacity;

    // LineInfo
    lineInfo *lineInfos;
    int lineCount;
    int lineCapacity;

} chunk;

void initChunk(chunk *chunk);
void writeChunk(chunk *chunk, uint8_t byte, int line);

void writeConstant(chunk *chunk, value value, int line);

void freeChunk(chunk *chunk);

int getLine(chunk *chunk, int index);

#endif