
#include "memory.h"
#include "object.h"
#include "vm.h"

#include <stdlib.h>
#include <stdio.h>
void *reallocate(void *pointer, size_t oldSize, size_t newSize)
{
    if (newSize == 0)
    {
        free(pointer);
        return NULL;
    }

    void *result = realloc(pointer, newSize);
    if (result == NULL)
    {
        exit(5);
    }

    return result;
}

static void freeObject(Obj *freeObject)
{
    switch (freeObject->type)
    {
    case OBJ_STRING:
        ObjString *objStr = (ObjString *)freeObject;
        FREE_ARRAY(char, objStr->chars, objStr->length + 1);
        FREE(ObjString, freeObject);
        break;
    case OBJ_FUNCTION:
        ObjFunction *objFunction = (ObjFunction *)freeObject;
        freeChunk(&objFunction->chunk);
        FREE(objFunction, freeObject);
        break;
    case OBJ_NATIVE:
        FREE(OBJ_NATIVE, freeObject);
    default:
        break;
    }
}
void freeObjects()
{
    Obj *object = vm.objects;
    while (object != NULL)
    {
        Obj *next = object->next;
        freeObject(object);
        object = next;
    }
}