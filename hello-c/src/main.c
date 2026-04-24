#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

/// @brief 用于读取输入-》处理结果 read - Eval -print -loop
static void repl()
{
    char line[1024];
    while (1)
    {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin))
        {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

// 读取一个文件中的所有字符
static char *runfile(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        fprintf("Cant Load File path : %s", path);
        exit(74);
    }
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(fileSize + 1);
    if (buffer == NULL)
    {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    size_t byteRead = fread(buffer, sizeof(char), fileSize, file);
    // 新增部分开始
    if (bytesRead < fileSize)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[byteRead] = '\0';
    fclose(file);
    return buffer;
}

int main(int argc, const char *argv[])
{
    initVM();
    if (argc == 1)
    {
        repl();
    }
    else if (argc == 2)
    {
        runFile(argv[1]);
    }
    else
    {
        fprintf(stderr, "Usage [path] \n");
    }

    freeVM();
    scanf("1");
    return 0;
}
