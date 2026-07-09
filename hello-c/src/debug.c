#include <stdio.h>
#include "debug.h"
#include "object.h"
const char *tokenTypeName(tokenType type)
{
    switch (type)
    {
    case TOKEN_LEFT_PAREN:
        return "TOKEN_LEFT_PAREN";
    case TOKEN_RIGHT_PAREN:
        return "TOKEN_RIGHT_PAREN";
    case TOKEN_LEFT_BRACE:
        return "TOKEN_LEFT_BRACE";
    case TOKEN_RIGHT_BRACE:
        return "TOKEN_RIGHT_BRACE";
    case TOKEN_COMMA:
        return "TOKEN_COMMA";
    case TOKEN_DOT:
        return "TOKEN_DOT";
    case TOKEN_MINUS:
        return "TOKEN_MINUS";
    case TOKEN_PLUS:
        return "TOKEN_PLUS";
    case TOKEN_SEMICOLON:
        return "TOKEN_SEMICOLON";
    case TOKEN_SLASH:
        return "TOKEN_SLASH";
    case TOKEN_STAR:
        return "TOKEN_STAR";
    case TOKEN_BANG:
        return "TOKEN_BANG";
    case TOKEN_BANG_EQUAL:
        return "TOKEN_BANG_EQUAL";
    case TOKEN_EQUAL:
        return "TOKEN_EQUAL";
    case TOKEN_EQUAL_EQUAL:
        return "TOKEN_EQUAL_EQUAL";
    case TOKEN_GREATER:
        return "TOKEN_GREATER";
    case TOKEN_GREATER_EQUAL:
        return "TOKEN_GREATER_EQUAL";
    case TOKEN_LESS:
        return "TOKEN_LESS";
    case TOKEN_LESS_EQUAL:
        return "TOKEN_LESS_EQUAL";
    case TOKEN_IDENTIFIER:
        return "TOKEN_IDENTIFIER";
    case TOKEN_STRING:
        return "TOKEN_STRING";
    case TOKEN_NUMBER:
        return "TOKEN_NUMBER";
    case TOKEN_AND:
        return "TOKEN_AND";
    case TOKEN_CLASS:
        return "TOKEN_CLASS";
    case TOKEN_ELSE:
        return "TOKEN_ELSE";
    case TOKEN_FALSE:
        return "TOKEN_FALSE";
    case TOKEN_FOR:
        return "TOKEN_FOR";
    case TOKEN_FUN:
        return "TOKEN_FUN";
    case TOKEN_IF:
        return "TOKEN_IF";
    case TOKEN_NIL:
        return "TOKEN_NIL";
    case TOKEN_OR:
        return "TOKEN_OR";
    case TOKEN_PRINT:
        return "TOKEN_PRINT";
    case TOKEN_RETURN:
        return "TOKEN_RETURN";
    case TOKEN_SUPER:
        return "TOKEN_SUPER";
    case TOKEN_THIS:
        return "TOKEN_THIS";
    case TOKEN_TRUE:
        return "TOKEN_TRUE";
    case TOKEN_VAR:
        return "TOKEN_VAR";
    case TOKEN_WHILE:
        return "TOKEN_WHILE";
    case TOKEN_ERROR:
        return "TOKEN_ERROR";
    case TOKEN_EOF:
        return "TOKEN_EOF";
    case TOKEN_INTERPOLATION_START:
        return "TOKEN_INTERPOLATION_START";
    case TOKEN_INTERPOLATION_END:
        return "TOKEN_INTERPOLATION_END";
    default:
        return "TOKEN_UNKNOWN";
    }
}

void printTokenType(tokenType type)
{
    printf("%s", tokenTypeName(type));
}

void disassembleChunk(chunk *chunk, const char *name)
{
    printf("===== %s =====\n", name);
    for (int i = 0; i < chunk->count;)
    {
        i = disassembleInstruction(chunk, i);
        printf("\n");
    }
}

int simpleInstruction(char *name, int offset)

{
    printf("name is %s", name);
    return offset + 1;
}
static int constantInstruction(const char *name, chunk *chunk, int offset)
{
    uint8_t instruction = chunk->code[offset];
    uint32_t constant = 0;
    uint8_t constantOffset = 1;
    if (instruction == OP_CONSTANT_LONG)
    {
        constant = chunk->code[offset + 1] << 16 | chunk->code[offset + 2] << 8 | chunk->code[offset + 3];
        constantOffset = 3;
    }
    else
    {
        constant = chunk->code[offset + 1];
        constantOffset = 1;
    }

    printf("name is %s | index is : %d |", name, constant);
    printf("value is :");
    printValue(chunk->constants.value[constant]);

    return offset + 1 + constantOffset;
}
static int byteInstruction(const char *name, chunk *chunk,
                           int offset)
{
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}
static int jumpInstruction(const char *name, int sign,
                           chunk *chunk, int offset)
{
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset,
           offset + 3 + sign * jump);
    return offset + 3;
}
static int invokeInstruction(const char *name, chunk *chunk,
                             int offset)
{
    uint8_t constant = chunk->code[offset + 1];
    uint8_t argCount = chunk->code[offset + 2];
    printf("%-16s (%d args) %4d '", name, argCount, constant);
    printValue(chunk->constants.value[constant]);
    printf("'\n");
    return offset + 3;
}
int disassembleInstruction(chunk *chunk, int offset)
{
    printf("%04d :", offset);

    uint8_t instruction = chunk->code[offset];
    switch (instruction)
    {
    case OP_RETURN:
        printf("op_return");
        return offset + 1;
    case OP_CONSTANT:
    case OP_CONSTANT_LONG:
        return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_NEGATE:
        printf("op_negate");
        return offset + 1;
        break;
    case OP_ADD:
        return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
        return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
        return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
        return simpleInstruction("OP_DIVIDE", offset);
    case OP_EQUAL:
        return simpleInstruction("OP_EQUAL", offset);
    case OP_GREATER:
        return simpleInstruction("OP_GREATER", offset);
    case OP_LESS:
        return simpleInstruction("OP_LESS", offset);
    case OP_NIL:
        return simpleInstruction("OP_NIL", offset);
    case OP_TRUE:
        return simpleInstruction("true", offset);
    case OP_FALSE:
        return simpleInstruction("false", offset);
    case OP_PRINT:
        return simpleInstruction("print", offset);
    case OP_POP:
        return simpleInstruction("OP_POP", offset);
    case OP_DEFINE_GLOBAL:
        return constantInstruction("OP_DEFINE_GLOBAL", chunk,
                                   offset);
    case OP_GET_GLOBAL:
        return constantInstruction("OP_GET_GLOBAL", chunk,
                                   offset);
    case OP_GET_UPVALUE:
        return byteInstruction("OP_GET_UPVALUE", chunk, offset);
    case OP_SET_UPVALUE:
        return byteInstruction("OP_SET_UPVALUE", chunk, offset);
    case OP_GET_LOCAL:
        return byteInstruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:
        return byteInstruction("OP_SET_LOCAL", chunk, offset);
    case OP_JUMP:
        return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
        return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_LOOP:
        return jumpInstruction("OP_LOOP", -1, chunk, offset);
    case OP_DUP:
        return simpleInstruction("OP_Dup", offset);
    case OP_CALL:
        return byteInstruction("OP_CALL", chunk, offset);
    case OP_CLASS:
        return constantInstruction("OP_CLASS", chunk, offset);
    case OP_CLOSURE:
    {
        offset++;
        uint8_t constant = chunk->code[offset++];
        printf("%-16s %4d ", "OP_CLOSURE", constant);
        printValue(chunk->constants.value[constant]);
        printf("\n");
        ObjFunction *function = AS_FUNCTION(
            chunk->constants.value[constant]);
        for (int j = 0; j < function->upvalueCount; j++)
        {
            int isLocal = chunk->code[offset++];
            int index = chunk->code[offset++];
            printf("%04d      |                     %s %d\n",
                   offset - 2, isLocal ? "local" : "upvalue", index);
        }
        return offset;
    }
    case OP_CLOSE_UPVALUE:
    {
        return simpleInstruction("OP_CLOSE_UPVALUE", offset);
    }
    case OP_GET_PROPERTY:
        return constantInstruction("OP_GET_PROPERTY", chunk, offset);
    case OP_SET_PROPERTY:
        return constantInstruction("OP_SET_PROPERTY", chunk, offset);
    case OP_METHOD:
        return constantInstruction("OP_METHOD", chunk, offset);
    case OP_INVOKE:
        return invokeInstruction("OP_INVOKE", chunk, offset);
    case OP_FIELD:
        return constantInstruction("OP_FIELD", chunk, offset);

    default:
        printf("UnKnow opcode");
        break;
    }
    return offset;
}
