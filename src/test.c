#include "oarm.h"
#include "ostd.h"

bool assert(bool cond);
void test_parse_int(void);
void test_tokenize(void);
void test_resolve_labels(void);
void test_ostd_map(void);
void test_e2e_add_sub(void);
void test_e2e_ldr_str(void);
void test_e2e_lsl_lsr(void);
void test_all_branches(void);

int main(void) {
  printf("oarm test run\n");
  test_parse_int();
  test_tokenize();
  test_ostd_map();
  test_e2e_add_sub();
  test_e2e_ldr_str();
  test_e2e_lsl_lsr();
  test_all_branches();
  printf("\nend tests.\n");
}

void test_parse_int(void) {
  printf("\ntest_parse_int\n");
  int n = parse_int(s8_from(malloc, "123")).val;
  if (!assert(123 == n)) {
    printf("test_part_int: expected 123 got %i", n);
  }

  n = parse_int(s8_from(malloc, "0")).val;
  if (!assert(0 == n)) {
    printf("test_part_int: expected 0 got %i", n);
  }

  n = parse_int(s8_from(malloc, "2")).val;
  if (!assert(2 == n)) {
    printf("test_part_int: expected 2 got %i", n);
  }

  n = parse_int(s8_from(malloc, "1234")).val;
  if (!assert(1234 == n)) {
    printf("test_part_int: expected 1234 got %i", n);
  }

  n = parse_int(s8_from(malloc, "-3")).val;
  if (!assert(-3 == n)) {
    printf("test_part_int: expected -3 got %i", n);
  }

  n = parse_int(s8_from(malloc, "-765")).val;
  if (!assert(-765 == n)) {
    printf("test_part_int: expected -765 got %i", n);
  }
}

void test_tokenize(void) {
  printf("\ntest_tokenize\n");

  TokenizedProgram p = tokenize(
      s8_from(malloc, " mov  x0, #1 \n rpc\n add  x1 , x0, #2\nreg\n"));

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
  if (!assert(r1.ok)) {
    printf("expected to find str hello in map");
  }
  if (!assert(r1.val == 1)) {
    printf("expected to val for hello to be 1 got %i", r1.val);
  }

  ResultInt r2 = map_get(m, s8_from(malloc, "notok"));
  if (!assert(!r2.ok)) {
    printf("Expected not to find string notok");
  }
  ResultInt r3 = map_get(m, s8_from(malloc, "orion"));
  if (!assert(r3.ok)) {
    printf("expected to find str orion in map");
  }
  if (!assert(r3.val == -1)) {
    printf("expected val for orion to be -1 got %i", r3.val);
  }

  if (!assert(m.count == 7)) {
    printf("expected m count to be 7 got %i", m.count);
  }
}

void test_e2e_add_sub(void) {
  printf("\ntest_e2e_add_sub\n");

  char* argv[2];
  argv[1] = "asm/e2e/add_sub.s";
  ResultState rs = entry(2, (char**)&argv);

  if (!assert(rs.return_val == 0)) {
    printf("expected add_sub.s to return successful, got %i\n", rs.return_val);
  }
  if (!assert(rs.state.registers[0] == 3)) {
    printf("expected add_sub.s to have 3 in its first register, got %i\n",
           rs.state.registers[0]);
  }
}

void test_e2e_ldr_str(void) {
  printf("\ntest_e2e_ldr_str\n");

  char* argv[2];
  argv[1] = "asm/e2e/ldr_str.s";
  ResultState rs = entry(2, (char**)&argv);

  if (!assert(rs.return_val == 0)) {
    printf("expected ldr_str.s to return successful, got %i\n", rs.return_val);
  }
  if (!assert(rs.state.registers[0] == 99)) {
    printf("expected ldr_str.s to have 99 in its first register, got %i\n",
           rs.state.registers[0]);
  }
}

void test_e2e_lsl_lsr(void) {
  printf("\test_e2e_lsl_lsr\n");

  char* argv[2];
  argv[1] = "asm/e2e/lsl_lsr.s";
  ResultState rs = entry(2, (char**)&argv);

  if (!assert(rs.return_val == 0)) {
    printf("expected lsl_lsr.s to return successful, got %i\n", rs.return_val);
  }
  if (!assert(rs.state.registers[0] == 2)) {
    printf("expected lsl_lsr.s to have 2 in its first register, got %i\n",
           rs.state.registers[0]);
  }
}
void test_all_branches(void) {
  printf("\ntest_all_branches\n");

  int num_branches = 7;
  char* file_names[num_branches];
  file_names[0] = (char*)"asm/e2e/b.s";
  file_names[1] = (char*)"asm/e2e/beq.s";
  file_names[2] = (char*)"asm/e2e/bne.s";
  file_names[3] = (char*)"asm/e2e/bgt.s";
  file_names[4] = (char*)"asm/e2e/bge.s";
  file_names[5] = (char*)"asm/e2e/ble.s";
  file_names[6] = (char*)"asm/e2e/blt.s";

  int i = 0;
  for (; i < num_branches; i++) {
    char* fn = file_names[i];
    char* argv[2];
    argv[1] = fn;

    ResultState rs = entry(2, (char**)&argv);
    if (!assert(rs.return_val == 0)) {
      printf("expected %s to return successful, got %i\n", fn, rs.return_val);
    }
    if (!assert(rs.state.registers[0] == 1)) {
      printf("expected %s to have 1 in its first register, got %i\n", fn,
             rs.state.registers[0]);
    }
  }
}

bool assert(bool cond) {
  if (cond) {
    putchar('.');
  } else {
    putchar('!');
  }
  return cond;
}