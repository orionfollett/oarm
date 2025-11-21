#include "oarm.h"

bool assert(bool cond) {
  if (cond) {
    printf(".");
  }
  return cond;
}
void test_parse_int(void);

int main(int argc, char** argv) {
  printf("oarm test run\n");
  test_parse_int();
  printf("\nend tests.\n");
}

void test_parse_int(void) {
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
}