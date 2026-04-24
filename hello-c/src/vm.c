#include "vm.h"
#include "common.h"
#include "debug.h"
#include "value.h"
#include <stdio.h>

#include "comiler.h"
VM vm;

static void resetStack()
{
    vm.stackTop = vm.stack;
}
void push(value value)
{
    *vm.stackTop = value;
    vm.stackTop++;
}
value pop()
{
    vm.stackTop--;
    return *vm.stackTop;
}
void initVM()
{
    resetStack();
}
void freeVM()
{
}

static inline uint8_t read_byte()
{
    return *vm.ip++;
}

static inline value read_constant()
{
    uint8_t index = read_byte();
    return vm.chunk->constants.value[index];
}
static inline value read_constant_long()
{
    uint8_t index_1 = read_byte();
    uint8_t index_2 = read_byte();
    uint8_t index_3 = read_byte();
    value constant = index_1 << 16 | index_2 << 8 | index_3;
    return constant;
}

#define BINARY_OP(op)    \
    do                   \
    {                    \
        value b = pop(); \
        value a = pop(); \
        push(a op b);    \
    } while (false)

static interpretResult run()
{
    while (1)
    {

#ifdef DEBUG_TRACE_EXECUTION
        printf("       ");
        for (value *slot = vm.stack; slot < vm.stackTop; slot++)
        {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = read_byte())
        {
        case OP_CONSTANT_LONG:
        {
            value constant = read_constant_long();
            // printValue(constant);
            push(constant);
            printf("\n");
            break;
        }
        case OP_CONSTANT:
        {
            value constant = read_constant();
            // printValue(constant);
            push(constant);
            printf("\n");
            break;
        }
        case OP_NEGATE:
        {
            push(-pop());
            break;
        }
        case OP_ADD:
            BINARY_OP(+);
            break;
        case OP_SUBTRACT:
            BINARY_OP(-);
            break;
        case OP_MULTIPLY:
            BINARY_OP(*);
            break;
        case OP_DIVIDE:
            BINARY_OP(/);
            break;
        case OP_RETURN:
        {
            printValue(pop());
            printf("\n");
            return INTERPRET_OK;
        };
        }
    }
}



interpretResult interpret(const char *source)
{
    compile(source);
    return INTERPRET_OK;
}
