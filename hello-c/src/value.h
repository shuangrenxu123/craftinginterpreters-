#ifndef VALUE_H
#define VALUE_H

#include "common.h"
typedef double value;

typedef struct
{
    int capacity;
    int count;
    value *value;
} valueArray;

void initValueArray(valueArray *array);
int writeValueArray(valueArray *array, value value);
void freeValueArray(valueArray *array);

void printValue(value value);
#endif