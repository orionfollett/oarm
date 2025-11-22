#include "oarm.h"

bool assert(bool cond);
void test_parse_int(void);
void test_tokenize(void);

int main(void) {
  printf("oarm test run\n");
  test_parse_int();
  test_tokenize();
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

  TokenizedProgram p =
      tokenize(" mov  x0, #1 \n rpc\n add  x1 , x0, #2\nreg\n", 41);
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

bool assert(bool cond) {
  if (cond) {
    printf(".");
  }
  return cond;
}