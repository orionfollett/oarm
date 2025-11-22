#include "ostd.h"

Map map_init(AllocFn alloc, unsigned long size_power) {
  int size = 1;
  int i = 0;
  for (; i < size_power; i++) {
    size = size * 2;
  }
  Map m;
  int byte_size = (unsigned long)size * sizeof(MapNode);
  m.buckets = alloc(byte_size);
  m.count = 0;
  m.size = size;
  memset(m.buckets, 0, byte_size);
  return m;
}

Map map_set(AllocFn alloc, Map m, s8 key, int val) {
  return m;
}

int map_get(Map m, s8 key) {
  return 0;
}

void map_destroy(FreeFn free, Map map) {}
