#include "vm.h"
#include "common.h"
#include "debug.h"
#include "value.h"
#include <stdio.h>
#include <stdarg.h>

#include "compiler.h"
VM vm;

static void resetStack()
{
    vm.stackTop = vm.stack;
}

static void runtimeError(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lineInfos[instruction].line;
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
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

static value peek(int distance)
{
    return vm.stackTop[-1 - distance];
}

static bool isFalsey(value val)
{
    return IS_NIL(val) || (IS_BOOL(val) && !AS_BOOL(val));
}

void initVM()
{
    resetStack();
    vm.objects = NULL;
}
void freeVM()
{
    freeObject();
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
    uint32_t index = index_1 << 16 | index_2 << 8 | index_3;
    return vm.chunk->constants.value[index];
}

#define BINARY_OP(valueType, op)                        \
    do                                                  \
    {                                                   \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) \
        {                                               \
            runtimeError("operands must be numbers");   \
            return INTERPRET_ERROR;                     \
        }                                               \
        double b = AS_NUMBER(pop());                    \
        double a = AS_NUMBER(pop());                    \
        push(valueType(a op b));                        \
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
        case OP_ADD:
            BINARY_OP(NUMBER_VAL, +);
            break;
        case OP_SUBTRACT:
            BINARY_OP(NUMBER_VAL, -);
            break;
        case OP_MULTIPLY:
            BINARY_OP(NUMBER_VAL, *);
            break;
        case OP_DIVIDE:
            BINARY_OP(NUMBER_VAL, /);
            break;
        case OP_EQUAL:
        {
            value a = pop();
            value b = pop();
            push(BOOL_VAL(valuesEqual(a, b)));
        }

        case OP_GREATER:
            BINARY_OP(BOOL_VAL, >);
            break;
        case OP_LESS:
            BINARY_OP(BOOL_VAL, <);

        case OP_TRUE:
            push(BOOL_VAL(true));
            break;
        case OP_FALSE:
            push(BOOL_VAL(false));
            break;
        case OP_NIL:
            push(NIL_VAL);
            break;

        case OP_NEGATE:
        {

            if (!IS_NUMBER(peek(0)))
            {
                runtimeError("operand must be a number");
                return INTERPRET_ERROR;
            }
            push(NUMBER_VAL(-AS_NUMBER(pop())));
            break;
        }
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
    chunk chunk;
    initChunk(&chunk);
    if (!compile(source, &chunk))
    {
        freeChunk(&chunk);
        return INTERPRET_ERROR;
    }
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;
    interpretResult result = run();
    freeChunk(&chunk);

    return result;
}
