#include "oarm.h"
#include "ostd.h"

bool assert(bool cond);
void test_parse_int(void);
void test_tokenize(void);
void test_resolve_labels(void);
void test_ostd_map(void);

int main(void) {
  printf("oarm test run\n");
  test_parse_int();
  test_tokenize();
  test_resolve_labels();
  test_ostd_map();
  printf("\nend tests.\n");
}

void test_parse_int(void) {
  printf("\ntest_parse_int\n");
  int n = parse_int("123", 3);
  if (!assert(123 == n)) {
    printf("test_part_int: expected 123 got %i", n);
  }

  n = parse_int("0", 1);
  if (!assert(0 == n)) {
    printf("test_part_int: expected 0 got %i", n);
  }

  n = parse_int("2", 1);
  if (!assert(2 == n)) {
    printf("test_part_int: expected 2 got %i", n);
  }

  char c[4] = {'1', '2', '3', '4'};
  n = parse_int((const char*)c, 4);
  if (!assert(1234 == n)) {
    printf("test_part_int: expected 1234 got %i", n);
  }

  n = parse_int("-3", 2);
  if (!assert(-3 == n)) {
    printf("test_part_int: expected -3 got %i", n);
  }

  n = parse_int("-765", 4);
  if (!assert(-765 == n)) {
    printf("test_part_int: expected -765 got %i", n);
  }
}

void test_tokenize(void) {
  printf("\ntest_tokenize\n");

  TokenizedProgram p = tokenize(
      s8_from(malloc, " mov  x0, #1 \n rpc\n add  x1 , x0, #2\nreg\n"));
  /*log_tokenized_program(p);*/

  if (!assert(4 == p.len)) {
    printf("expected program len of 4 got %i", p.len);
  }
  Line line1 = p.lines[0];
  if (!assert(3 == line1.len)) {
    printf("expected 3 tokens on line 1 got %i", line1.len);
  }
  Line line2 = p.lines[1];
  if (!assert(1 == line2.len)) {
    printf("expected 1 tokens on line 2 got %i", line2.len);
  }
}

void test_resolve_labels(void) {
  printf("\ntest_resolve_labels\n");
  TokenizedProgram p = tokenize(
      s8_from(malloc, "loop:\nmov x0, #0\nadd x0, x0, #1\nb loop\nexit:"));
  resolve_labels(p);
}

void test_ostd_map(void) {
  printf("\ntest_ostd_map\n");
  Map m = map_init(malloc, 1);
  if (!assert(m.size = 2)) {
    printf("expected map size 2 got %i", m.size);
  }

  m = map_set(malloc, m, s8_from(malloc, "hello"), 1);
  m = map_set(malloc, m, s8_from(malloc, "goodbye"), 2);
  m = map_set(malloc, m, s8_from(malloc, "orion"), 3);
  m = map_set(malloc, m, s8_from(malloc, "orion2"), 3);
  m = map_set(malloc, m, s8_from(malloc, "orion3"), 3);
  m = map_set(malloc, m, s8_from(malloc, "orion4"), 3);
  m = map_set(malloc, m, s8_from(malloc, "orion5"), 3);
  m = map_set(malloc, m, s8_from(malloc, "orion"), -1);

  ResultInt r1 = map_get(m, s8_from(malloc, "hello"));
  if (!assert(r1.found)) {
    printf("expected to find str hello in map");
  }
  if (!assert(r1.val == 1)) {
    printf("expected to val for hello to be 1 got %i", r1.val);
  }

  ResultInt r2 = map_get(m, s8_from(malloc, "notfound"));
  if (assert(r2.found)) {
    printf("Expected not to find string notfound");
  }
  ResultInt r3 = map_get(m, s8_from(malloc, "orion"));
  if (!assert(r3.found)) {
    printf("expected to find str orion in map");
  }
  if (!assert(r3.val == -1)) {
    printf("expected val for orion to be -1 got %i", r3.val);
  }

  if (!assert(m.count == 7)) {
    printf("expected m count to be 7 got %i", m.count);
  }
}

bool assert(bool cond) {
  if (cond) {
    printf(".");
  }
  return cond;
}