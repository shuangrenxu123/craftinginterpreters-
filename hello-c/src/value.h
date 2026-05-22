#ifndef VALUE_H
#define VALUE_H

#include "common.h"

typedef struct Obj obj;
typedef static ObjString ObjString;

typedef enum
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ
} valueType;

typedef struct
{
    valueType type;
    union
    {
        bool boolean;
        double number;
        Obj *obj;
    } as;

} value;

typedef struct
{
    int capacity;
    int count;
    value *value;
} valueArray;

#define BOOL_VAL(v) ((value){VAL_BOOL, {.boolean = (v)}})
#define NIL_VAL ((value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(v) ((value){VAL_NUMBER, {.number = (v)}})
#define OBJ_VAL(v) ((value){VAL_OBJ, {.obj = (*Obj)(v)}})

#define IS_OBJ(v) ((v).type == VAL_OBJ)
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

#define AS_OBJ(value) ((value).as.obj)
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

bool valuesEqual(value a, value b);

void initValueArray(valueArray *array);
int writeValueArray(valueArray *array, value value);
void freeValueArray(valueArray *array);

void printValue(value value);
#endif
