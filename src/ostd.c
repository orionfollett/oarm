#include "ostd.h"

Map map_init(AllocFn alloc, unsigned long size_log_2) {
  int size = 1;
  int i = 0;
  for (; i < size_log_2; i++) {
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

int s8_hash(s8 key){
   int fnv_offset_basis = 2166136261;
   int fnv_prime = 16777619;

   int h = fnv_offset_basis;
   int i = 0;
   for(; i < key.len; i++){
    h = h * fnv_prime;
    h = h ^ key.str[i];
   }
   return h;
}

bool s8_eq(s8 s1, s8 s2){
    if(s1.len != s2.len){
        return false; 
    }

    int i = 0;
    for(; i < s1.len; i++){
        if(s1.str[i] != s2.str[i]){
            return false;
        }
    }
    return true; 
}

Map map_set(AllocFn alloc, Map m, s8 key, int val) {
    if(m.count >> 1 > m.size) {
        /* rebalance if m count is greater than half the size */    
    }
    int h = s8_hash(key);
    int index = h & (m.size - 1);

    MapNode n;
    n.hash = h;
    s8 key_copy;
    key_copy.len = key.len;
    key_copy.str = (char*)alloc(sizeof(char)*key.len);
    memcpy(key_copy.str, key.str, sizeof(char)*key.len);
    n.key = key_copy;

    
    return m;
}

int map_get(Map m, s8 key) {
  return 0;
}

void map_destroy(FreeFn free, Map map) {}
