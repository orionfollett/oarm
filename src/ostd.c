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

int s8_hash(s8 key) {
  int fnv_offset_basis = 2166136261;
  int fnv_prime = 16777619;

  int h = fnv_offset_basis;
  int i = 0;
  for (; i < key.len; i++) {
    h = h * fnv_prime;
    h = h ^ key.str[i];
  }
  return h;
}

bool s8_eq(s8 s1, s8 s2) {
  if (s1.len != s2.len) {
    return false;
  }

  int i = 0;
  for (; i < s1.len; i++) {
    if (s1.str[i] != s2.str[i]) {
      return false;
    }
  }
  return true;
}

s8 s8_from(AllocFn alloc, const char* s) {
  s8 r;

  int i = 0;
  while (s[i] != '\0') {
    i++;
  }
  r.len = i;
  r.str = alloc(sizeof(char) * r.len);
  memcpy(r.str, s, sizeof(char) * r.len);
  return r;
}

MapNode* map_node_init(AllocFn alloc,
                       s8 key,
                       int val,
                       int hash,
                       MapNode* next) {
  s8 key_copy;
  key_copy.len = key.len;
  key_copy.str = (char*)alloc(sizeof(char) * key.len);
  memcpy(key_copy.str, key.str, sizeof(char) * key.len);

  MapNode* n = (MapNode*)alloc(sizeof(MapNode));
  n->hash = hash;
  n->key = key_copy;
  n->next = 0;
  return n;
}

Map map_set(AllocFn alloc, Map m, s8 key, int val) {
  if (m.count >> 1 > m.size) {
    /* TODO: rebalance if m count is greater than half the size */
  }
  int hash = s8_hash(key);
  int index = hash & (m.size - 1);

  MapNode* curr = m.buckets[index];
  if (curr == 0) {
    MapNode* n = map_node_init(alloc, key, val, hash, 0);
    m.buckets[index] = n;
    return m;
  }

  while (true) {
    if (s8_eq(curr->key, key)) {
      curr->val = val;
      break;
    }
    if (curr->next == 0) {
      MapNode* n = map_node_init(alloc, key, val, hash, 0);
      curr->next = n;
      break;
    }
    curr = curr->next;
  }

  return m;
}

ResultInt map_get(Map m, s8 key) {
  int hash = s8_hash(key);
  int index = hash & (m.size - 1);
  ResultInt r;
  r.found = false;
  r.val = 0;
  MapNode* curr = m.buckets[index];
  while (curr != 0) {
    if (s8_eq(curr->key, key)) {
      r.found = true;
      r.val = curr->val;
      return r;
    }
    curr = curr->next;
  }
  return r;
}

void map_destroy(FreeFn free, Map map) {}
