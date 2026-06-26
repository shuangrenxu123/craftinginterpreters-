#ifndef Object_H
#define Object_H
#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(v) (AS_OBJ(v)->type)

#define IS_STRING(v) isObjType(v, OBJ_STRING)
#define IS_FUNCTION(v) isObjType(v, OBJ_FUNCTION)
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)

#define AS_NATIVE(v) (((ObjNative *)AS_OBJ(v))->function)
#define AS_STRING(v) ((ObjString *)AS_OBJ(v))
#define AS_CSTRING(v) (((ObjString *)AS_OBJ(v))->chars)

typedef enum
{
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,

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
    uint32_t hash;
};
typedef struct
{
    Obj obj;
    int arity; // 参数数量
    chunk chunk;
    ObjString *name;
} ObjFunction;

typedef value (*NativeFn)(int argCount, value *args);
typedef struct
{
    Obj obj;
    NativeFn function;
} ObjNative;

ObjFunction *newFunction();
ObjNative *newNative(NativeFn function);

ObjString *copyString(const char *chars, int length);
ObjString *takeString(char *chars, int length);
static inline bool isObjType(value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
void printObject(value value);
#endif
