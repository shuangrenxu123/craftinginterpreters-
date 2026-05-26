#include <stdio.h>
#include <string.h>

#include "vm.h"
#include "object.h"
#include "memory.h"
#include "value.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type *)allocateObject(sizeof(type), objectType)

static Obj *allocateObject(size_t size, ObjType type)
{
    Obj *object = (Obj *)reallocate(NULL, 0, size);
    object->type = type;

    object->next = vm.objects;
    vm.objects = object;

    return object;
}

static ObjString *allocateString(char *heapChars, int length)
{
    ObjString *str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    str->length = length;
    str->chars = heapChars;
    return str;
}
void printObject(value value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    default:
        break;
    }
}

ObjString *copyString(const char *chars, int length)
{
    char *heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    return allocateString(heapChars, length);
}
