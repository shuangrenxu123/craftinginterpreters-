#ifndef Object_H
#define Object_H

#include "common.h"
#include "value.h"

#define OBJ_TYPE(v) (AS_OBJ(v).type)
#define IS_STRING(v) isOBjType(v, OBJ_STRING);

static inline bool isObjType(Value value,ObjType type){
    return 
}

typedef enum
{
    OBJ_STRING,

} ObjType;

struct Obj
{
    ObjType type;
};

struct ObjString
{
    Obj obj;
    int length;
    char *chars;
};

#endif