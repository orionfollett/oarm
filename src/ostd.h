#ifndef OSTD_H
#define OSTD_H

typedef struct s8 {
    char* str;
    int len;
} s8;

typedef struct Map {
    MapNode** buckets;
    int size;
    int count;
} Map;

typedef struct MapNode {
    int hash;
    int val;
    s8 key;
} MapNode;

Map map_init(unsigned long size, alloc);
Map map_set(Map m, s8 key, int val);
Map map_get(Map m, s8 key);
void map_destroy(Map map, free);

#endif