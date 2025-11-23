#ifndef OSTD_H
#define OSTD_H

#include <string.h>
#include "ostd.h"

#define bool int
#define true 1
#define false 0

/*Note this is just true on modern hardware with gcc/clang but isn't actually
 * guaranteed.*/
typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned char u8;

typedef long i64;
typedef int i32;
typedef char i8;

typedef void* (*AllocFn)(u64);
typedef void (*FreeFn)(void*);

typedef struct s8 {
  char* str;
  int len;
} s8;

typedef struct ResultInt {
  bool found;
  int val;
} ResultInt;

typedef struct MapNode {
  u64 hash;
  int val;
  s8 key;
  struct MapNode* next;
} MapNode;

typedef struct Map {
  MapNode** buckets;
  int size;
  int count;
} Map;

i64 s8_hash(s8 key);
s8 s8_from(AllocFn alloc, const char* s);
bool s8_eq(s8 s1, s8 s2);

Map map_init(AllocFn alloc, u64 size);
Map map_set(AllocFn alloc, Map m, s8 key, int val);
ResultInt map_get(Map m, s8 key);
void map_destroy(FreeFn free, Map map);

MapNode* map_node_init(AllocFn alloc, s8 key, int val, u64 hash, MapNode* next);
#endif