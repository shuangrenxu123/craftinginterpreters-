#include "vm.h"
#include "common.h"
#include "debug.h"
#include "value.h"
#include "memory.h"
#include "object.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "compiler.h"
VM vm;
static value clockNative(int argCount, value *args)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}
static void resetStack()
{
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
}

static void runtimeError(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--)
    {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = frame->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ",
                function->chunk.lineInfos[instruction].line);
        if (function->name == NULL)
        {
            fprintf(stderr, "script\n");
        }
        else
        {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
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
static bool call(ObjFunction *function, int argCount)
{
    if (argCount != function->arity)
    {
        runtimeError("Expected %d arguments but got %d.",
                     function->arity, argCount);
        return false;
    }
    if (vm.frameCount == FRAMES_MAX)
    {
        runtimeError("Stack overflow");
        return false;
    }
    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}
static bool callValue(value callee, int argCount)
{
    if (IS_OBJ(callee))
    {
        switch (OBJ_TYPE(callee))
        {
        case OBJ_FUNCTION:
            return call(AS_FUNCTION(callee), argCount);
            break;
        case OBJ_NATIVE: {
            NativeFn native = AS_NATIVE(callee);
            value result = native(argCount, vm.stackTop - argCount);
            vm.stackTop -= argCount + 1;
            push(result);
            return true;
        }
        default:
            break;
        }
    }
    runtimeError("can only call function and class");
    return false;
}
static void defineNative(const char *name, NativeFn function)
{
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

static bool isFalsey(value val)
{
    return IS_NIL(val) || (IS_BOOL(val) && !AS_BOOL(val));
}

void initVM()
{
    resetStack();
    vm.objects = NULL;
    initTable(&vm.strings);
    initTable(&vm.globals);
    defineNative("clock", clockNative);
}
void freeVM()
{
    freeTable(&vm.strings);
    freeTable(&vm.globals);
    freeObjects();
}

#define READ_BYTE() (*ip++)

#define READ_SHORT() \
    (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))

#define READ_CONSTANT()     (frame->function->chunk.constants.value[READ_BYTE()])

#define READ_CONSTANT_LONG()     (frame->function->chunk.constants.value[         (READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())
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
    CallFrame *frame = &vm.frames[vm.frameCount - 1];
    uint8_t *ip = frame->ip;

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
        disassembleInstruction(&frame->function->chunk,
                               (int)(ip - frame->function->chunk.code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
        case OP_CONSTANT_LONG:
        {
            value constant = READ_CONSTANT_LONG();
            push(constant);
            break;
        }
        case OP_CONSTANT:
        {
            value constant = READ_CONSTANT();
            push(constant);
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
            break;
        }

        case OP_GREATER:
            BINARY_OP(BOOL_VAL, >);
            break;
        case OP_LESS:
            BINARY_OP(BOOL_VAL, <);
            break;

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
        case OP_TOSTRING:
        {
            if (!IS_NUMBER(peek(0)))
            {
                runtimeError("operand must be a number");
                return INTERPRET_ERROR;
            }
            double v = AS_NUMBER(pop());
            char buffer[64];

            int length = snprintf(buffer, sizeof(buffer), "%g", v);
            if (length < 0 || length >= (int)sizeof(buffer))
            {
                runtimeError("string buffer overflow");
                return INTERPRET_ERROR;
            }

            push(OBJ_VAL(copyString(buffer, length)));
            break;
        }
        case OP_PRINT:
        {
            printValue(pop());
            printf("\n");
            break;
        }
        case OP_RETURN:
        {
            value result = pop();
            vm.frameCount--;
            if (vm.frameCount == 0)
            {
                pop();
                return INTERPRET_OK;
            }
            vm.stackTop = frame->slots;
            push(result);
            frame = &vm.frames[vm.frameCount - 1];
            ip = frame->ip;
            break;
        };
        case OP_POP:
            pop();
            break;
        case OP_DEFINE_GLOBAL:
        {
            ObjString *name = READ_STRING();
            tableSet(&vm.globals, name, peek(0));
            pop();
            break;
        }
        case OP_GET_GLOBAL:
        {
            ObjString *name = READ_STRING();
            value val;
            if (!tableGet(&vm.globals, name, &val))
            {
                runtimeError("Not find variable Name");
                return INTERPRET_ERROR;
            }
            push(val);
            break;
        }
        case OP_SET_GLOBAL:
        {
            ObjString *name = READ_STRING();
            if (tableSet(&vm.globals, name, peek(0)))
            {
                tableDelete(&vm.globals, name);
                runtimeError("Undefined variable '%s'.", name->chars);
                return INTERPRET_ERROR;
            }
            break;
        }
        case OP_GET_LOCAL:
        {
            uint8_t slot = READ_BYTE();
            push(frame->slots[slot]);
            break;
        }
        case OP_SET_LOCAL:
        {
            uint8_t slot = READ_BYTE();
            frame->slots[slot] = peek(0);
            break;
        }
        case OP_JUMP:
        {
            uint16_t offset = READ_SHORT();
            ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE:
        {
            uint16_t offset = READ_SHORT();
            if (isFalsey(peek(0)))
            {
                ip += offset;
            }
            break;
        }
        case OP_LOOP:
        {
            uint16_t offset = READ_SHORT();
            ip -= offset;
            break;
        }
        case OP_DUP:
        {
            push(peek(0));
            break;
        }
        case OP_CALL:
        {
            int argCount = READ_BYTE();
            frame->ip = ip;
            if (!callValue(peek(argCount), argCount))
            {
                return INTERPRET_ERROR;
            }
            frame = &vm.frames[vm.frameCount - 1];
            ip = frame->ip;
            break;
        }
        }
    }
}

interpretResult interpret(const char *source)
{
    ObjFunction *function = compile(source);
    if (function == NULL)
    {
        return INTERPRET_ERROR;
    }
    push(OBJ_VAL(function));
    vm.frames[vm.frameCount].function = function;
    vm.frames[vm.frameCount].ip = function->chunk.code;
    vm.frames[vm.frameCount].slots = vm.stack;
    vm.frameCount++;

    return run();
}
