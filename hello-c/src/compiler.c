#include "compiler.h"
#include <stdio.h>

void scanToken()
{
}
void compile(const char *source)
{
    initScanner(source);

    int line = 1;
    while (1)
    {

        token token = scanToken();
        if (token.line != line)
        {
            printf("%4d", token.line);
            line = token.line;
        }
        else
        {
            printf("|");
        }

        printf("%2d '%.*s'\n", token.type, token.length, token.start);

        if (token == TOKEN_EOF)
        {
            break;
        }
    }
}