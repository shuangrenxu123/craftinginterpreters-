#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256
typedef struct
{
    chunk *chunk;
    // 说是叫ip,但是其实是第一个执行 开始的byte地址
    uint8_t *ip;

    value stack[STACK_MAX];
    value *stackTop;
} VM;

typedef enum
{
    INTERPRET_OK,
    INTERPRET_ERROR,
    INTERPRET_RUNTIME,

} interpretResult;

void initVM();
void freeVM();
void push(value value);
value pop();

interpretResult interpret(chunk *chunk);

#endif