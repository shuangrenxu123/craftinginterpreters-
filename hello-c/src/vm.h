#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
    chunk *chunk;
    // Points to the next bytecode instruction to execute.
    uint8_t *ip;

    value stack[STACK_MAX];
    value *stackTop;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_ERROR,
    INTERPRET_RUNTIME,
} interpretResult;

void initVM(void);
void freeVM(void);
void push(value value);
value pop(void);

interpretResult interpret(const char *source);

#endif
