#include "chunk.h"
#include "memory.h"
#include "value.h"

void initChunk(chunk *chunk)
{
    chunk->count = 0;
    chunk->code = NULL;
    chunk->capacity = 0;

    chunk->lineInfos = NULL;
    chunk->lineCapacity = 0;
    chunk->lineCount = 0;

    initValueArray(&(chunk->constants));
}

void writeChunk(chunk *chunk, uint8_t byte, int line)
{
    if (chunk->capacity < chunk->count + 1)
    {
        int oldCapacity = chunk->capacity;

        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    if (chunk->lineCapacity < chunk->lineCount + 1)
    {
        int oldLineCapacity = chunk->lineCapacity;

        chunk->lineCapacity = GROW_CAPACITY(oldLineCapacity);
        chunk->lineInfos = GROW_ARRAY(lineInfo, chunk->lineInfos, oldLineCapacity, chunk->lineCapacity);
    }

    // 拿到最后一个的line值
    if (chunk->lineInfos->count != 0 && chunk->lineInfos[chunk->lineCount - 1].line == line)
    {
        chunk->lineInfos[chunk->lineCount - 1].count += 1;
        chunk->lineInfos[chunk->lineCount - 1].endIndex = chunk->count + 1;
    }
    else
    {
        lineInfo info;
        info.startIndex = chunk->count;
        info.endIndex = info.startIndex;
        info.count = 1;
        info.line = line;
        chunk->lineInfos[chunk->lineCount] = info;
    }

    chunk->code[chunk->count] = byte;
    chunk->count += 1;
}

/// @brief 添加一个常量
/// @param chunk
/// @param value
/// @return 常量对应数组的索引值
static int addConstant(chunk *chunk, value value)
{
    writeValueArray(&(chunk->constants), value);
    return chunk->constants.count - 1;
}

/// @brief 用于写入N位的常量数值
/// @param chunk
/// @param value
/// @param byteCount
/// @param line
static void writeOperandN(chunk *chunk, int value, int byteCount, int line)
{
    for (int i = byteCount - 1; i >= 0; --i)
    {
        uint8_t b = (uint8_t)((value >> (i * 8)) & 0xFF);
        writeChunk(chunk, b, line);
    }
}

void writeConstant(chunk *chunk, value value, int line)
{
    int index = addConstant(chunk, value);
    if (chunk->constants.count > 255)
    {
        writeChunk(chunk, OP_CONSTANT_LONG, line);
        writeOperandN(chunk, index, 3, line);
    }
    else
    {
        writeChunk(chunk, OP_CONSTANT, line);
        writeOperandN(chunk, index, 1, line);
    }
}

void freeChunk(chunk *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(lineInfo, chunk->lineInfos, chunk->lineCapacity);

    freeValueArray(&(chunk->constants));
    initChunk(chunk);
}

int getLine(chunk *chunk, int index)
{
    for (int i = 0; i < chunk->lineCount; i++)
    {
        lineInfo *lineInfo = &chunk->lineInfos[i];
        if (lineInfo->startIndex >= index && lineInfo->endIndex >= index)
        {
            return lineInfo->line;
        }
    }
    return -1;
}