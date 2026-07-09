#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "value.h"
#include "table.h"
#include "object.h"
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

// 代表了一个函数的调用
typedef struct
{
    ObjClosure *closure;
    uint8_t *ip;
    // 函数可以使用的第一个槽
    value *slots;

} CallFrame;

typedef struct
{
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    value stack[STACK_MAX];
    value *stackTop;
    Table globals;
    Table strings;
    Obj *objects;

    ObjString *initString;
    ObjUpvalue *openUpvalues;

    size_t bytesAllocated;
    size_t nextGC;

    int grayCount;
    int grayCapacity;
    Obj **grayStack;
} VM;

typedef enum
{
    INTERPRET_OK,
    INTERPRET_ERROR,
    INTERPRET_RUNTIME,
} interpretResult;
extern VM vm;

void initVM(void);
void freeVM(void);
void runtimeError(const char *format, ...);
void defineNative(const char *name, NativeFn function);
void push(value value);
value pop(void);

interpretResult interpret(const char *source);

#endif
