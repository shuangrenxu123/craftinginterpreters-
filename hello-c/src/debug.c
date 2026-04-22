#include <stdio.h>
#include "debug.h"

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
    if (instruction == OP_CONSTANT)
    {
        constant = chunk->code[offset + 1];
        constantOffset = 1;
    }
    else
    {
        constant = chunk->code[offset + 1] << 16 | chunk->code[offset + 2] << 8 | chunk->code[offset + 3];
        constantOffset = 3;
    }

    printf("name is %s | index is : %d |", name, constant);
    printf("value is :%g", chunk->constants.value[constant]);

    return offset + 1 + constantOffset;
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
    default:
        printf("UnKnow opcode");
        break;
    }
    return offset;
}
