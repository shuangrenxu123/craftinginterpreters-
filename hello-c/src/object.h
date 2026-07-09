#ifndef Object_H
#define Object_H
#include "chunk.h"
#include "common.h"
#include "value.h"
#include "table.h"
typedef struct ObjClosure ObjClosure;

#define OBJ_TYPE(v) (AS_OBJ(v)->type)

#define IS_STRING(v) isObjType(v, OBJ_STRING)
#define IS_FUNCTION(v) isObjType(v, OBJ_FUNCTION)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)
#define IS_CLASSINSTANCE(value) isObjType(value, OBJ_INSTANCE)
#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)

#define AS_CLASSINSTANCE(value) ((ObjInstance *)AS_OBJ(value))
#define AS_BOUND_METHOD(value) ((ObjBoundMethod *)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass *)AS_OBJ(value))
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
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_BOUND_METHOD,
} ObjType;

struct Obj
{
    ObjType type;
    bool isMarked;
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
    ObjString *name;
    Table methods;
} ObjClass;

typedef struct
{
    Obj obj;
    ObjClass *class;
    Table fields;

} ObjInstance;

typedef struct
{
    Obj obj;
    value receiver;
    ObjClosure *method;
} ObjBoundMethod;

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

typedef struct ObjClosure
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
ObjClass *newClass(ObjString *name);
ObjInstance *newInstance(ObjClass *class);
ObjBoundMethod *newBoundMethod(value receiver, ObjClosure *method);
ObjString *copyString(const char *chars, int length);
ObjUpvalue *newUpvalue(value *slot);

ObjString *takeString(char *chars, int length);
static inline bool isObjType(value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
void printObject(value value);
#endif
