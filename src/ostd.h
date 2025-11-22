#ifndef OSTD_H
#define OSTD_H

#include <string.h>
#include "ostd.h"

typedef void* (*AllocFn)(unsigned long);
typedef void (*FreeFn)(void*);

typedef struct s8 {
  char* str;
  int len;
} s8;

typedef struct MapNode {
  int hash;
  int val;
  s8 key;
  struct MapNode* next;
} MapNode;

typedef struct Map {
  MapNode** buckets;
  int size;
  int count;
} Map;

Map map_init(AllocFn alloc, unsigned long size);
Map map_set(AllocFn alloc, Map m, s8 key, int val);
int map_get(Map m, s8 key);
void map_destroy(FreeFn free, Map map);

#endif