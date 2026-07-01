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
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)

#define AS_CLOSURE(v) ((ObjClosure *)AS_OBJ(v))
#define AS_NATIVE(v) (((ObjNative *)AS_OBJ(v))->function)
#define AS_STRING(v) ((ObjString *)AS_OBJ(v))
#define AS_CSTRING(v) (((ObjString *)AS_OBJ(v))->chars)
typedef enum
{
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_CLOSURE,
    OBJ_UPVALUE,

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
    int upvalueCount;
    chunk chunk;
    ObjString *name;
} ObjFunction;

typedef struct ObjUpvalue
{
    Obj obj;
    value *location;
    struct ObjUpvalue *next;
    value closed;
} ObjUpvalue;

typedef struct
{
    Obj obj;
    ObjFunction *function;
    ObjUpvalue **upvalues;
    int upvalueCount;
} ObjClosure;

typedef bool (*NativeFn)(int argCount, value *args, value *result);
typedef struct
{
    Obj obj;
    NativeFn function;
} ObjNative;

ObjFunction *newFunction();
ObjNative *newNative(NativeFn function);
ObjClosure *newClosure(ObjFunction *function);

ObjString *copyString(const char *chars, int length);
ObjUpvalue *newUpvalue(value *slot);

ObjString *takeString(char *chars, int length);
static inline bool isObjType(value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
void printObject(value value);
#endif
