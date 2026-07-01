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

    OP_NIL,
    OP_TRUE,
    OP_FALSE,

    OP_ADD,      // +
    OP_SUBTRACT, // -
    OP_MULTIPLY, // *
    OP_DIVIDE,   // /

    OP_NOT,

    OP_TOSTRING,
    OP_EQUAL,   //=
    OP_GREATER, //>
    OP_LESS,    //<

    OP_CONSTANT_LONG, // 扩容常量
    OP_CONSTANT,      // 常量
    OP_PRINT,
    OP_RETURN,
    OP_DEFINE_GLOBAL,
    OP_POP,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,

    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_DUP, // 复制栈顶值
    OP_LOOP,

    OP_CALL,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE
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