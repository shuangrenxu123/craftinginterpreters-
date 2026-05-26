#ifndef Object_H
#define Object_H

#include "common.h"
#include "value.h"

#define OBJ_TYPE(v) (AS_OBJ(v)->type)

#define IS_STRING(v) isObjType(v, OBJ_STRING)

#define AS_STRING(v) ((ObjString *)AS_OBJ(v))
#define AS_CSTRING(v) (((ObjString *)AS_OBJ(v))->chars)

typedef enum
{
    OBJ_STRING,

} ObjType;

struct Obj
{
    ObjType type;

    struct Obj *next;
};

struct ObjString
{
    Obj obj;
    int length;
    char *chars;
};

ObjString *copyString(const char *chars, int length);

static inline bool isObjType(value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
void printObject(value value);
#endif
