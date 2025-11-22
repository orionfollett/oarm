#ifndef OARM_H
#define OARM_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define bool int
#define true 1
#define false 0

#define MAX_LINE_LEN 128
#define MAX_IDENT_LEN 32
#define MAX_TOKENS_PER_LINE 4
#define CMD_LEN 3
#define ARGS_LEN (MAX_LINE_LEN - CMD_LEN - 1)
#define NUM_REGISTERS 10
#define MEM_BYTES 256

typedef struct State {
  int registers[NUM_REGISTERS];
  int memory[MEM_BYTES];

  /* comparison byte, -1 if lt, 0 eq, 1 gt */
  int cmp;

  /*program counter, just references the line no in asm file*/
  int pc;
  bool cont;
} State;

typedef struct Token {
  char tok[MAX_IDENT_LEN];
  int len;
} Token;

typedef struct Line {
  Token tokens[MAX_TOKENS_PER_LINE];
  int len;
} Line;

typedef struct TokenizedProgram {
  Line* lines;
  int len;
} TokenizedProgram;

typedef enum {
  ADD,
  LDR,
  LSL,
  LSR,
  MEM,
  MOV,
  REG,
  RET,
  STR,
  SUB,
  NL,
  LABEL,
  BRANCH,
  BLE,
  BGE,
  BLT,
  BGT,
  BEQ,
  BNE,
  RPC,
  UNKNOWN
} CMD;
typedef int Register;

typedef enum { A_CONSTANT, A_REGISTER } AddressType;
typedef struct Address {
  int val;
  AddressType type;
} Address;

typedef enum { ADDRESS, CONSTANT, REGISTER, REGISTER_OR_CONSTANT } ArgType;

typedef struct Arg {
  ArgType tag;
  union {
    Register reg;
    Address addr;
    int constant;
  };
} Arg;

typedef struct Args {
  int count;
  Arg args[3];
  bool is_valid;
} Args;

typedef struct ArgValidation {
  ArgType expected_arg_type;
} ArgValidation;

typedef struct ArgValidations {
  int expected_arg_count;
  const char* cmd_pretty_str;
  ArgValidation validations[3];
} ArgValidations;

State tick(State s, Line line);
CMD identify_cmd(Token t);

Args parse_args(Line line);
int parse_int(const char* num, int len);
TokenizedProgram tokenize(char* str, int size);

State mov(State s, Line line);
State ldr(State s, Line line);
State str(State s, Line line);
State add_or_sub(State s, Line line, bool is_add);
State branch(State s, Line line, CMD command);
State lsl_or_lsr(State s, Line line, bool is_left);

bool validate_args(Args args, ArgValidations validations);
void log_registers(State s);
void log_mem(State s);
int get_register_or_constant(State s, Arg a);
void print_help(void);
void log_tokenized_program(TokenizedProgram p);
void log_line(Line line);
int entry(int argc, char** argv);

#endif