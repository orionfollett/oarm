#include "ostd.h"

u64 s8_hash(s8 key) {
  const u64 fnv_offset_basis = 1469598103934665603ull;
  const u64 fnv_prime = 1099511628211ull;

  u64 h = fnv_offset_basis;
  int i = 0;
  for (; i < key.len; i++) {
    h ^= (unsigned short)key.str[i];
    h *= fnv_prime;
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
  r.str = alloc((u64)r.len);
  memcpy(r.str, s, (u64)r.len);
  return r;
}

const char* s8_to_c(AllocFn alloc, s8 s) {
  char* n = alloc((u64)(1 + s.len));
  memcpy(n, s.str, (u64)s.len);
  n[s.len] = '\0';
  return (const char*)n;
}

s8 s8_clone(AllocFn alloc, s8 s) {
  s8 n;
  n.str = alloc((u64)s.len);
  n.len = s.len;
  return n;
}

s8 s8_replace_all(AllocFn alloc,
                  FreeFn free,
                  s8 dest,
                  s8 target,
                  s8 replacement) {
  /*for all occurences of target in dest, replace with replacement returns a new
   * copy and does not modify any args.*/
  if (dest.len < target.len) {
    return s8_clone(alloc, dest);
  }

  /*
  ex:
  dest: bacatcat
  target: cat
  repl:  hello
  answer bahellohello

  ex:
  dest: bacatcat
  target: cat
  repl:  c
  answer: bacc
  */

  /*find occurences, store where they are*/
  int* match_list = alloc(((u64)(dest.len / target.len) + 1) * sizeof(int));
  int match_count = 0;
  int i = 0;
  for (; i < (dest.len - target.len) + 1; i++) {
    if (memcmp(dest.str + i, target.str, (u64)target.len) == 0) {
      match_list[match_count] = i;
      match_count++;
      i = i + target.len - 1;
    }
  }

  /*calculate size needed, allocate it*/
  s8 new_str;
  new_str.len = (replacement.len - target.len) * match_count + dest.len;
  new_str.str = (char*)alloc((u64)new_str.len);

  /*write the new string in`*/
  int new_i = 0;
  int dest_i = 0;
  int match_i = 0;

  while (new_i < new_str.len) {
    /*copy replacement in*/
    if (match_i < match_count && match_list[match_i] == dest_i) {
      memcpy(new_str.str + ((u64)new_i), replacement.str, (u64)replacement.len);
      dest_i += target.len;
      new_i += replacement.len;
      match_i++;
    }
    /* write from original string */
    else {
      new_str.str[new_i] = dest.str[dest_i];
      dest_i++;
      new_i++;
    }
  }
  free(match_list);
  return new_str;
}

void s8_destroy(FreeFn free, s8 s) {
  free(s.str);
}

Map map_init(AllocFn alloc, u64 size_log_2) {
  int size = 1;
  u64 i = 0;
  for (; i < size_log_2; i++) {
    size = size * 2;
  }
  Map m;
  u64 byte_size = (u64)size * sizeof(MapNode);
  m.buckets = alloc(byte_size);
  m.count = 0;
  m.size = size;
  memset(m.buckets, 0, byte_size);
  return m;
}

MapNode* map_node_init(AllocFn alloc,
                       s8 key,
                       int val,
                       u64 hash,
                       MapNode* next) {
  s8 key_copy;
  key_copy.len = key.len;
  key_copy.str = (char*)alloc((u64)key.len);
  memcpy(key_copy.str, key.str, (u64)key.len);

  MapNode* n = (MapNode*)alloc(sizeof(MapNode));
  n->hash = hash;
  n->key = key_copy;
  n->next = next;
  n->val = val;
  return n;
}

Map map_set(AllocFn alloc, Map m, s8 key, int val) {
  if (m.count >> 1 > m.size) {
    /* TODO: rebalance if m count is greater than half the size */
  }
  u64 hash = s8_hash(key);
  int index = (int)(hash & (u64)(m.size - 1));

  MapNode* curr = m.buckets[index];
  if (curr == 0) {
    MapNode* n = map_node_init(alloc, key, val, hash, 0);
    m.buckets[index] = n;
    m.count++;
    return m;
  }

  while (true) {
    if (s8_eq(curr->key, key)) {
      curr->val = val;
      break;
    }
    if (curr->next == 0) {
      m.count++;
      MapNode* n = map_node_init(alloc, key, val, hash, 0);
      curr->next = n;
      break;
    }
    curr = curr->next;
  }

  return m;
}

ResultInt map_get(Map m, s8 key) {
  u64 hash = s8_hash(key);
  int index = (int)(hash & ((u64)m.size - 1));
  ResultInt r;
  r.ok = false;
  r.val = 0;
  MapNode* curr = m.buckets[index];
  while (curr != 0) {
    if (s8_eq(curr->key, key)) {
      r.ok = true;
      r.val = curr->val;
      return r;
    }
    curr = curr->next;
  }
  return r;
}

void map_destroy(FreeFn free, Map map) {
  /*iterate through all the buckets and free all the strings too before freeing
   * the buckets buffer.*/
  free(map.buckets);
}
