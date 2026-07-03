#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"
#include "value.h"

typedef struct Obj Obj;

void *reallocate(void *pointer, size_t oldSize, size_t newSize);
void collectGarbage();

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity * 2))

#define GROW_ARRAY(type, pointer, oldCount, newCount)      \
    (type *)reallocate(pointer, sizeof(type) * (oldCount), \
                       sizeof(type) * (newCount))
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * oldCount, 0);

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

void freeObjects();
void markValue(value value);
void markObject(Obj *object);
#define ALLOCATE(type, count) (type *)reallocate(NULL, 0, sizeof(type) * (count))

#endif
