#include <stdio.h>
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"
int main(int argc, const char *argv[])
{
    initVM();

    chunk chunk;
    initChunk(&chunk);
    // Test CODE

    // writeChunk(&chunk, OP_RETURN);

    // int constant = addConstant(&chunk, 1.2f);

    // writeChunk(&chunk, OP_CONSTANT, 1);
    writeConstant(&chunk, 100, 1);
    writeChunk(&chunk, OP_NEGATE, 1);
    writeConstant(&chunk, 101, 1);
    writeChunk(&chunk, OP_ADD, 1);
    writeConstant(&chunk, 102, 1);
    writeChunk(&chunk, OP_SUBTRACT, 1);

    writeChunk(&chunk, OP_RETURN, 1);

    // disassembleChunk(&chunk, "Test");
    interpret(&chunk);

    freeVM();
    freeChunk(&chunk);
    scanf("1");
    return 0;
}
