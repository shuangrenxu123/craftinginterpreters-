#include "native.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static bool checkArgCount(const char *name, int actual, int expected)
{
    if (actual == expected)
    {
        return true;
    }

    runtimeError("%s() expects %d argument but got %d.", name, expected, actual);
    return false;
}

static bool isFalsey(value val)
{
    return IS_NIL(val) || (IS_BOOL(val) && !AS_BOOL(val));
}

static value makeStringValue(const char *chars)
{
    return OBJ_VAL(copyString(chars, (int)strlen(chars)));
}

static bool clockNative(int argCount, value *args, value *result)
{
    if (!checkArgCount("clock", argCount, 0))
    {
        return false;
    }

    *result = NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
    return true;
}

static bool assertNative(int argCount, value *args, value *result)
{
    if (!checkArgCount("assert", argCount, 1))
    {
        return false;
    }

    if (isFalsey(args[0]))
    {
        runtimeError("Assertion failed.");
        return false;
    }

    *result = NIL_VAL;
    return true;
}

static bool typeNative(int argCount, value *args, value *result)
{
    if (!checkArgCount("type", argCount, 1))
    {
        return false;
    }

    switch (args[0].type)
    {
    case VAL_BOOL:
        *result = makeStringValue("bool");
        return true;
    case VAL_NIL:
        *result = makeStringValue("nil");
        return true;
    case VAL_NUMBER:
        *result = makeStringValue("number");
        return true;
    case VAL_OBJ:
        switch (OBJ_TYPE(args[0]))
        {
        case OBJ_STRING:
            *result = makeStringValue("string");
            return true;
        case OBJ_FUNCTION:
        case OBJ_CLOSURE:
            *result = makeStringValue("function");
            return true;
        case OBJ_NATIVE:
            *result = makeStringValue("native");
            return true;
        }
    }

    *result = makeStringValue("unknown");
    return true;
}

static bool strNative(int argCount, value *args, value *result)
{
    if (!checkArgCount("str", argCount, 1))
    {
        return false;
    }

    char buffer[256];
    switch (args[0].type)
    {
    case VAL_BOOL:
        *result = makeStringValue(AS_BOOL(args[0]) ? "true" : "false");
        return true;
    case VAL_NIL:
        *result = makeStringValue("nil");
        return true;
    case VAL_NUMBER:
        snprintf(buffer, sizeof(buffer), "%g", AS_NUMBER(args[0]));
        *result = makeStringValue(buffer);
        return true;
    case VAL_OBJ:
        switch (OBJ_TYPE(args[0]))
        {
        case OBJ_STRING:
            *result = args[0];
            return true;
        case OBJ_FUNCTION:
        case OBJ_CLOSURE:
        {
            ObjFunction *function = IS_CLOSURE(args[0])
                                        ? AS_CLOSURE(args[0])->function
                                        : AS_FUNCTION(args[0]);
            if (function->name == NULL)
            {
                *result = makeStringValue("<script>");
                return true;
            }

            snprintf(buffer, sizeof(buffer), "<fn %s>", function->name->chars);
            *result = makeStringValue(buffer);
            return true;
        }
        case OBJ_NATIVE:
            *result = makeStringValue("<native fn>");
            return true;
        }
    }

    *result = makeStringValue("<unknown>");
    return true;
}

void defineNativeFunctions(void)
{
    defineNative("assert", assertNative);
    defineNative("clock", clockNative);
    defineNative("str", strNative);
    defineNative("type", typeNative);
}
