#include "scanner.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

typedef struct
{
    const char *start;
    const char *current;
    int line;
} scanner;

scanner scanner;
void initScanner(const char *source)
{
    scanner.current = source;
    scanner.start = source;
    scanner.line = 1;
}