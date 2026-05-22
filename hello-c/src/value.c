#include "value.h"
#include "memory.h"
#include <stdio.h>

bool valuesEqual(value a, value b)
{
    if (a.type != b.type)
    {
        return false;
    }
    switch (a.type)
    {
    case VAL_BOOL:
        return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NUMBER:
        return AS_NUMBER(a) == AS_BOOL(b);
    case VAL_NIL:
        return true;

    default:
        break;
    }
}
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
    switch (value.type)
    {
    case VAL_BOOL:
        printf(AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL:
        printf("nil");
        break;
    case VAL_NUMBER:
        printf("%g", AS_NUMBER(value));
        break;
    }
}
