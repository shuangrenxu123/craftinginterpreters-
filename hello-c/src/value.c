#include "value.h"
#include "memory.h"
#include <stdio.h>
void initValueArray(valueArray *array)
{
    array->capacity = 0;
    array->count = 0;
    array->value = 0;
}
int writeValueArray(valueArray *array, value val)
{
    if (array->capacity < array->count + 1)
    {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->value = GROW_ARRAY(value, array->value, oldCapacity, array->capacity);
    }

    array->value[array->count] = val;
    array->count += 1;
    return array->count - 1;
}
void freeValueArray(valueArray *array)
{
    FREE_ARRAY(value, array->value, array->capacity);
    initValueArray(array);
}

void printValue(value value)
{
    printf("%g", value);
}