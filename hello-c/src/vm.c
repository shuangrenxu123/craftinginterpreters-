#include "vm.h"
#include "common.h"
#include "debug.h"
#include "value.h"
#include "memory.h"
#include "object.h"
#include "native.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "compiler.h"
VM vm;

static bool isFalsey(value val);

static void resetStack()
{
    vm.stackTop = vm.stack;
    vm.openUpvalues = NULL;
    vm.frameCount = 0;
}

void runtimeError(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--)
    {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = frame->closure->function;
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

static void concatenate()
{
    ObjString *b = AS_STRING(peek(0));
    ObjString *a = AS_STRING(peek(1));

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    pop();
    pop();
    push(OBJ_VAL(takeString(chars, length)));
}

static ObjString *valueToString(value val)
{
    char buffer[128];

    switch (val.type)
    {
    case VAL_BOOL:
        return copyString(AS_BOOL(val) ? "true" : "false",
                          AS_BOOL(val) ? 4 : 5);
    case VAL_NIL:
        return copyString("nil", 3);
    case VAL_NUMBER:
    {
        int length = snprintf(buffer, sizeof(buffer), "%g", AS_NUMBER(val));
        if (length < 0 || length >= (int)sizeof(buffer))
        {
            runtimeError("string buffer overflow");
            return NULL;
        }
        return copyString(buffer, length);
    }
    case VAL_OBJ:
        switch (OBJ_TYPE(val))
        {
        case OBJ_STRING:
            return AS_STRING(val);
        case OBJ_FUNCTION:
        {
            ObjFunction *function = AS_FUNCTION(val);
            if (function->name == NULL)
            {
                return copyString("<script>", 8);
            }
            int length = snprintf(buffer, sizeof(buffer), "<fn %s>", function->name->chars);
            if (length < 0 || length >= (int)sizeof(buffer))
            {
                runtimeError("string buffer overflow");
                return NULL;
            }
            return copyString(buffer, length);
        }
        case OBJ_CLOSURE:
        {
            ObjFunction *function = AS_CLOSURE(val)->function;
            if (function->name == NULL)
            {
                return copyString("<script>", 8);
            }
            int length = snprintf(buffer, sizeof(buffer), "<fn %s>", function->name->chars);
            if (length < 0 || length >= (int)sizeof(buffer))
            {
                runtimeError("string buffer overflow");
                return NULL;
            }
            return copyString(buffer, length);
        }
        case OBJ_NATIVE:
            return copyString("<native fn>", 11);
        case OBJ_UPVALUE:
            return copyString("<upvalue>", 9);
        }
    }

    return copyString("<unknown>", 9);
}

static bool call(ObjClosure *closure, int argCount)
{
    if (argCount != closure->function->arity)
    {
        runtimeError("Expected %d arguments but got %d.",
                     closure->function->arity, argCount);
        return false;
    }
    if (vm.frameCount == FRAMES_MAX)
    {
        runtimeError("Stack overflow");
        return false;
    }
    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}
static bool callValue(value callee, int argCount)
{
    if (IS_OBJ(callee))
    {
        switch (OBJ_TYPE(callee))
        {
        case OBJ_NATIVE:
        {
            NativeFn native = AS_NATIVE(callee);
            value result;
            if (!native(argCount, vm.stackTop - argCount, &result))
            {
                return false;
            }
            vm.stackTop -= argCount + 1;
            push(result);
            return true;
        }
        case OBJ_CLOSURE:
        {
            return call(AS_CLOSURE(callee), argCount);
        }
        default:
            break;
        }
    }
    runtimeError("can only call function and class");
    return false;
}

/// @brief 创建一个闭包值
/// @param local
/// @return
static ObjUpvalue *captureUpvalue(value *local)
{
    ObjUpvalue *preUpvalue = NULL;
    ObjUpvalue *upvalue = vm.openUpvalues;
    while (upvalue != NULL && upvalue->location > local)
    {
        preUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local)
    {
        return upvalue;
    }

    ObjUpvalue *createUpvalue = newUpvalue(local);

    createUpvalue->next = upvalue;
    if (preUpvalue == NULL)
    {
        vm.openUpvalues = createUpvalue;
    }
    else
    {
        preUpvalue->next = createUpvalue;
    }
    return createUpvalue;
}
static void closeUpvalues(value *last)
{
    while (vm.openUpvalues != NULL &&
           vm.openUpvalues->location >= last) // 比较指针的大小可以判断前后
    {
        ObjUpvalue *upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

void defineNative(const char *name, NativeFn function)
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
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;
    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;
    initTable(&vm.strings);
    initTable(&vm.globals);
    defineNativeFunctions();
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

#define READ_CONSTANT() (frame->closure->function->chunk.constants.value[READ_BYTE()])

#define READ_CONSTANT_LONG() (frame->closure->function->chunk.constants.value[(READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE()])

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
        disassembleInstruction(&frame->closure->function->chunk,
                               (int)(ip - frame->closure->function->chunk.code));
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
        {
            if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
            {
                concatenate();
            }
            else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
            {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a + b));
            }
            else
            {
                runtimeError("Operands must be two numbers or two strings.");
                return INTERPRET_RUNTIME;
            }
            break;
        }
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
            ObjString *string = valueToString(peek(0));
            if (string == NULL)
            {
                return INTERPRET_ERROR;
            }
            pop();
            push(OBJ_VAL(string));
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
            closeUpvalues(frame->slots);
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
        case OP_CLOSE_UPVALUE:
            closeUpvalues(vm.stackTop - 1);
            pop();
            break;
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
        case OP_CLOSURE:
        {
            ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
            ObjClosure *closure = newClosure(function);
            push(OBJ_VAL(closure));
            for (int i = 0; i < closure->upvalueCount; i++)
            {
                uint8_t isLocal = READ_BYTE();
                uint8_t index = READ_BYTE();
                if (isLocal)
                {
                    closure->upvalues[i] = captureUpvalue(frame->slots + index);
                }
                else
                {
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }
            break;
        }
        case OP_GET_UPVALUE:
        {
            uint8_t slot = READ_BYTE();
            push(*frame->closure->upvalues[slot]->location);
            break;
        }
        case OP_SET_UPVALUE:
        {
            uint8_t slot = READ_BYTE();
            *frame->closure->upvalues[slot]->location = peek(0);
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
    ObjClosure *closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    call(closure, 0);

    return run();
}
