
#include "memory.h"
#include "object.h"
#include "vm.h"
#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

static void freeObject(Obj *freeObject);

void *reallocate(void *pointer, size_t oldSize, size_t newSize)
{
    vm.bytesAllocated += newSize - oldSize;
    if (newSize > oldSize)
    {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif
    }

    if (vm.bytesAllocated > vm.nextGC)
    {
        collectGarbage();
        vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
    }

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
void markObject(Obj *obj)
{
    if (obj == NULL)
        return;
    if (obj->isMarked)
        return;
#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void *)obj);
    printValue(OBJ_VAL(obj));
    printf("\n");
#endif

    obj->isMarked = true;
    if (vm.grayCapacity < vm.grayCount + 1)
    {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.grayStack = (Obj **)realloc(vm.grayStack, sizeof(Obj *) * vm.grayCapacity);
        if (vm.grayStack == NULL)
        {
            exit(1);
        }
    }
    vm.grayStack[vm.grayCount++] = obj;
}
void markValue(value value)
{
    if (IS_OBJ(value))
    {
        markObject(AS_OBJ(value));
    }
}

static void markRoots()
{
    for (value *slot = vm.stack; slot < vm.stackTop; slot++)
    {
        markValue(*slot);
    }

    for (int i = 0; i < vm.frameCount; i++)
    {
        markObject((Obj *)vm.frames[i].closure);
    }

    for (ObjUpvalue *upvalue = vm.openUpvalues; upvalue != NULL;
         upvalue = upvalue->next)
    {
        markObject((Obj *)upvalue);
    }

    markTable(&vm.globals);

    markCompilerRoots();
    markObject((Obj *)vm.initString);
}
static void markArray(valueArray *array)
{
    for (int i = 0; i < array->count; i++)
    {
        markValue(array->value[i]);
    }
}

static void blackenObject(Obj *object)
{
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void *)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif
    switch (object->type)
    {
    case OBJ_NATIVE:
    case OBJ_STRING:
        break;
    case OBJ_UPVALUE:
        markValue(((ObjUpvalue *)object)->closed);
        break;
    case OBJ_FUNCTION:
    {
        ObjFunction *function = (ObjFunction *)object;
        markObject((Obj *)function->name);
        markArray(&function->chunk.constants);
        break;
    }
    case OBJ_CLOSURE:
    {
        ObjClosure *closure = (ObjClosure *)object;
        markObject((Obj *)closure->function);
        for (int i = 0; i < closure->upvalueCount; i++)
        {
            markObject((Obj *)closure->upvalues[i]);
        }
        break;
    }
    case OBJ_BOUND_METHOD:
    {
        ObjBoundMethod *bound = (ObjBoundMethod *)object;
        markValue(bound->receiver);
        markObject((Obj *)bound->method);
        break;
    }
    case OBJ_CLASS:
    {
        ObjClass *class = (ObjClass *)object;
        markObject((Obj *)class->name);
        markTable(&class->methods);
        break;
    }
    case OBJ_INSTANCE:
    {
        ObjInstance *instance = (ObjInstance *)object;
        markObject((Obj *)instance->class);
        markTable(&instance->fields);
        break;
    }
    }
}

static void traceReferences()
{
    while (vm.grayCount > 0)
    {
        Obj *object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    }
}

static void sweep()
{
    Obj *previous = NULL;
    Obj *object = vm.objects;
    while (object != NULL)
    {
        if (object->isMarked)
        {
            object->isMarked = false;
            previous = object;
            object = object->next;
        }
        else
        {
            Obj *unreached = object;
            object = object->next;
            if (previous != NULL)
            {
                previous->next = object;
            }
            else
            {
                vm.objects = object;
            }

            freeObject(unreached);
        }
    }
}

void collectGarbage()
{
#ifdef DEBUG_LOG_GC
    printf("--GC Begin\n");
    size_t before = vm.bytesAllocated;
#endif

    markRoots();
    traceReferences();
    tableRemoveWhite(&vm.strings);
    sweep();

#ifdef DEBUG_LOG_GC
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
           before - vm.bytesAllocated, before, vm.bytesAllocated,
           vm.nextGC);
    printf("-- gc end\n");
#endif
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
        FREE(ObjFunction, freeObject);
        break;
    case OBJ_NATIVE:
        FREE(ObjNative, freeObject);
        break;
    case OBJ_CLOSURE:
    {

        ObjClosure *closure = (ObjClosure *)freeObject;
        FREE_ARRAY(ObjUpvalue *, closure->upvalues, closure->upvalueCount);
        FREE(ObjClosure, freeObject);

        break;
    }
    case OBJ_UPVALUE:
        FREE(ObjUpvalue, freeObject);
        break;
    case OBJ_BOUND_METHOD:
        FREE(ObjBoundMethod, freeObject);
        break;
    case OBJ_INSTANCE:
    {
        ObjInstance *instance = (ObjInstance *)freeObject;
        freeTable(&instance->fields);
        FREE(ObjInstance, freeObject);
        break;
    }
    case OBJ_CLASS:
    {
        ObjClass *class = (ObjClass *)freeObject;
        freeTable(&class->methods);
        FREE(ObjClass, class);
        break;
    }
    default:
        break;
    }
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void *)freeObject, freeObject->type);
#endif
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
    free(vm.grayStack);
}
