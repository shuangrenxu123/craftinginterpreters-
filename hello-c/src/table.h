#ifndef TABLE_H
#define TABLE_H

#include "common.h"
#include "value.h"

typedef struct
{
    ObjString *key;
    value value;
} Entry;

typedef struct
{
    int count;
    int capacity;
    Entry *entries;
} Table;

void initTable(Table *table);
void freeTable(Table *table);
bool tableGet(Table *table, ObjString *key, value *value);
bool tableSet(Table *table, ObjString *string, value value);
void tableAddAll(Table *form, Table *to);
void markTable(Table *table);

ObjString *tableFindString(Table *table, const char *chars,
                           int length, uint32_t hash);

void tableRemoveWhite(Table *table);
bool tableDelete(Table *table, ObjString *key);
#endif
