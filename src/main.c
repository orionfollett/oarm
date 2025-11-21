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

int registers[NUM_REGISTERS] = {0};
int memory[MEM_BYTES] = {0};

/* comparison byte, -1 if lt, 0 eq, 1 gt */
int cmp = 0;

/*program counter, just references the line no in asm file*/
int pc = 0;

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
  UNKNOWN,
  BRANCH,
  BLE,
  BGE,
  BLT,
  BGT,
  BEQ,
  BNE,
  RPC
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

/*Declarations*/
bool tick(char line[MAX_LINE_LEN]);
CMD identify_cmd(char cmd[CMD_LEN]);

Args parse_args(char line[ARGS_LEN]);
int parse_int(const char* num, int len);
TokenizedProgram tokenize(char* str, int size);

void mov(char line[MAX_LINE_LEN]);
void ldr(char line[MAX_LINE_LEN]);
void str(char line[MAX_LINE_LEN]);
void add_or_sub(char line[MAX_LINE_LEN], bool is_add);
void branch(char line[MAX_LINE_LEN], CMD command);
void lsl_or_lsr(char line[MAX_LINE_LEN], bool is_left);
bool validate_args(Args args, ArgValidations validations);
void log_registers(void);
void log_mem(void);
int get_register_or_constant(Arg a);
void print_help(void);
/*Implementations*/

void print_help(void) {
  printf(
      "Usage: oarm [FILE]\n"
      "\n"
      "Examples:\n"
      "  oarm program.s      Assemble and run program.s\n"
      "\n"
      "Options:\n"
      "  --help              Show this help message and exit\n");
}

int main(int argc, char** argv) {
  printf("oarm v0.1\n____\n\n");

  FILE* input_stream = NULL;
  if (argc <= 1) {
    print_help();
  } else {
    if (strcmp("--help", argv[1]) == 0) {
      print_help();
      return 0;
    } else {
      input_stream = fopen(argv[1], "r");
      if (input_stream == NULL) {
        perror("Error opening file");
        return 1;
      }
    }
  }

  /*Copy the file into memory*/
  /*fun fact, there is a race condition between getting size of file and
   * allocating memory. Whatever though, don't run the assembler while editing
   * the file.*/
  fseek(input_stream, 0, SEEK_END);
  long fsize = ftell(input_stream);
  fseek(input_stream, 0, SEEK_SET);
  char* program = malloc((unsigned long)(fsize + 1));
  fread(program, (unsigned long)fsize, 1, input_stream);
  fclose(input_stream);
  program[fsize] = EOF;

  /*
  First pass:
  - squash white space - not needed for now, assume white space is good
  - split into lines (makes jumping easier)
  - tokenize each line (split lines by separators, eliminates whitespace, allows
  parsing individual tokens that you know are correct)
  - collect label declarations, label uses
  - replace label uses with absolute jump position
  */

  /* squash seps

  ' ' -> squash multi into single whitespace
  ' ,' -> squash into ','
  ', ' -> squash into ','
  ':', '[', ']'
  */

  int i = 0;
  int line_counter = 0;
  char line[MAX_LINE_LEN] = {0};
  for (; i < fsize; i++) {
    char c = program[i];
    if (c == EOF) {
      break;
    }
    if (line_counter > MAX_LINE_LEN) {
      printf("max line len of %i exceeded", MAX_LINE_LEN);
      break;
    }
    if (line_counter == 0) {
      printf("> ");
    }

    line[line_counter] = c;
    line_counter++;

    putchar(c);

    if (c == '\n') {
      if (!tick(line)) {
        break;
      }
      line_counter = 0;
      memset(line, 0, sizeof(char) * MAX_LINE_LEN);
    }
  }

  free(program);
  return 0;
}

TokenizedProgram tokenize(char* str, int length) {
  int program_size = 1;
  TokenizedProgram program;
  program.len = 0;
  program.lines = (Line*)malloc(program_size * sizeof(Line));

  Token t;
  t.len = 0;

  int line_counter = 0;
  int i = 0;
  for (; i < length; i++) {
    char c = str[i];

    switch (c) {
      case '\n':
        if (t.len > 0) {
          line_counter++;
        }
      case ' ':
      case ',':
      case ':':
        program.lines[line_counter].tokens[program.lines[line_counter].len] = t;
        program.lines[line_counter].len++;
        t.len = 0;
        memset(t.tok, 0, sizeof(char) * MAX_IDENT_LEN);
        break;
      default:
        t.tok[t.len] = c;
        t.len++;
    }
  }
}

bool tick(char line[MAX_LINE_LEN]) {
  /*Evaluate one line of asm.*/
  bool cont = true;
  char cmd_str[CMD_LEN] = {0};
  memcpy(cmd_str, line, CMD_LEN * sizeof(char));
  CMD cmd = identify_cmd(cmd_str);
  switch (cmd) {
    case ADD:
      add_or_sub(line, true);
      break;
    case BRANCH:
    case BEQ:
    case BNE:
    case BLE:
    case BLT:
    case BGE:
    case BGT:
      branch(line, cmd);
      break;
    case LDR:
      ldr(line);
      break;
    case LSL:
      lsl_or_lsr(line, true);
      break;
    case LSR:
      lsl_or_lsr(line, false);
      break;
    case MEM:
      log_mem();
      break;
    case MOV:
      mov(line);
      break;
    case REG:
      log_registers();
      break;
    case RET:
      cont = false;
      break;
    case NL:
      return false;
    case STR:
      str(line);
      break;
    case SUB:
      add_or_sub(line, false);
      break;
    case RPC:
      printf("pc: %i\n", pc);
      break;
    case UNKNOWN:
      printf("Error could not parse statement identifier: %c%c%c\n", cmd_str[0],
             cmd_str[1], cmd_str[2]);
      break;
  }
  pc++;
  return cont;
}

CMD identify_cmd(char cmd[CMD_LEN]) {
  if (cmd[0] == '\n' || cmd[0] == EOF) {
    return NL;
  }
  int key = (cmd[0] << 16) | (cmd[1] << 8) | cmd[2];
  switch (key) {
    case ('m' << 16) | ('e' << 8) | 'm':
      return MEM;
      break;
    case ('m' << 16) | ('o' << 8) | 'v':
      return MOV;
      break;
    case ('a' << 16) | ('d' << 8) | 'd':
      return ADD;
      break;
    case ('r' << 16) | ('e' << 8) | 'g':
      return REG;
      break;
    case ('r' << 16) | ('e' << 8) | 't':
      return RET;
      break;
    case ('l' << 16) | ('s' << 8) | 'l':
      return LSL;
      break;
    case ('l' << 16) | ('s' << 8) | 'r':
      return LSR;
      break;
    case ('s' << 16) | ('u' << 8) | 'b':
      return SUB;
      break;
    case ('s' << 16) | ('t' << 8) | 'r':
      return STR;
      break;
    case ('b' << 16) | ('e' << 8) | 'q':
      return BEQ;
      break;
    case ('b' << 16) | ('n' << 8) | 'e':
      return BNE;
      break;
    case ('b' << 16) | ('l' << 8) | 't':
      return BLT;
      break;
    case ('b' << 16) | ('l' << 8) | 'e':
      return BLE;
      break;
    case ('b' << 16) | ('g' << 8) | 't':
      return BGT;
      break;
    case ('b' << 16) | ('g' << 8) | 'e':
      return BGE;
      break;
    case ('b' << 16) | ('r' << 8) | 'a':
      return BRANCH;
      break;
    case ('r' << 16) | ('p' << 8) | 'c':
      return RPC;
      break;
  }
  return UNKNOWN;
}

int parse_int(const char* num, int len) {
  /*Given an array of chars, return an int. Char array must be null
   * terminated.*/
  if (len > 8) {
    printf(
        "Integer overflow detected in parse int. Max int is 8 digits. "
        "Truncating digits.\n");
    len = 8;
  }

  int result = 0;
  int place = 1;
  int i = len - 1;
  for (; i >= 0; i--) {
    if (num[i] < (int)'0' || num[i] > (int)'9') {
      printf("Non digit detected in parse int string: %x (%i)\n", num[i],
             num[i]);
      return 0;
    }
    result += place * (num[i] - (int)'0');
    place = place * 10;
  }

  return result;
}

/*TODO: clean this up,

do a few passes, one to cleanup all whitespace,
then split into tokens
then identify tokens
then do the final parse
*/
Args parse_args(char line[ARGS_LEN]) {
  Args args;
  args.count = 0;
  args.is_valid = true;

  int i = 0;
  int arg_start = 0;
  for (; i < ARGS_LEN; i++) {
    bool is_line_end = line[i] == EOF || line[i] == '\n';
    bool is_arg_end = line[i] == ',' || is_line_end;
    if (is_arg_end) {
      Arg a;

      /* ADDRESS argument*/
      if (line[arg_start] == '[') {
        a.tag = ADDRESS;
        if (line[arg_start + 1] == 'x') {
          a.addr.type = A_REGISTER;
        } else if (line[arg_start + 1] == '#') {
          a.addr.type = A_CONSTANT;
        } else {
          printf(
              "Argument %i, unsupported address type, must be register or "
              "constant.",
              args.count + 1);
          args.is_valid = false;
          return args;
        }
        int start = arg_start + 2;
        int len = i - arg_start - 3;
        if (len < 1) {
          printf(
              "Argument %i [col %i:%i], expected numerical value for memory "
              "address argument, got empty string.\n",
              args.count + 1, start, len);
          args.is_valid = false;
          return args;
        }
        a.addr.val = parse_int(line + start, len);

        if (a.addr.type == A_CONSTANT) {
          if (a.addr.val > MEM_BYTES || a.addr.val < 0) {
            printf(
                "Argument %i memory address is out of range, must be between 0 "
                "and %i\n",
                args.count + 1, MEM_BYTES);
            args.is_valid = false;
            return args;
          }
        }

        /*Register argument*/
      } else if (line[arg_start] == 'x') {
        a.tag = REGISTER;
        int start = arg_start + 1;
        int len = i - arg_start - 1;
        a.reg = parse_int(line + start, len);
        if (a.reg > NUM_REGISTERS || a.reg < 0) {
          printf(
              "Argument %i register is out of range, must be between 0 and "
              "%i\n",
              args.count + 1, NUM_REGISTERS);
          args.is_valid = false;
          return args;
        }

        /*constant argument*/
      } else if (line[arg_start] == '#') {
        a.tag = CONSTANT;
        int start = arg_start + 1;
        int len = i - arg_start - 1;
        a.constant = parse_int(line + start, len);
      } else {
        args.is_valid = false;
        printf(
            "Error parsing args, couldn't recognize arg type. last char "
            "parsed: %i (%c) (col: %i)\n",
            line[i], line[i], i);
        return args;
      }

      args.args[args.count] = a;
      args.count++;
      arg_start = i + 1;
      continue;
    }
    if (is_line_end) {
      return args;
    }
    if (args.count == 3) {
      return args;
    }
    if (line[i] == ' ' || line[i] == ',') {
      arg_start = i + 1;
    }
  }
  return args;
}

void mov(char line[MAX_LINE_LEN]) {
  Args args = parse_args(line + CMD_LEN + 1);
  if (!args.is_valid) {
    return;
  }

  ArgValidations v;
  v.cmd_pretty_str = "mov";
  v.expected_arg_count = 2;
  ArgValidation first_arg;
  first_arg.expected_arg_type = REGISTER;
  ArgValidation second_arg;
  second_arg.expected_arg_type = REGISTER_OR_CONSTANT;
  v.validations[0] = first_arg;
  v.validations[1] = second_arg;
  if (!validate_args(args, v)) {
    return;
  }

  Arg a1 = args.args[0];
  Arg a2 = args.args[1];

  int val = get_register_or_constant(a2);

  registers[a1.reg] = val;
  return;
}

void ldr(char line[MAX_LINE_LEN]) {
  Args args = parse_args(line + CMD_LEN + 1);

  if (!args.is_valid) {
    return;
  }

  ArgValidations v;
  v.cmd_pretty_str = "ldr";
  v.expected_arg_count = 2;
  ArgValidation first_arg;
  first_arg.expected_arg_type = REGISTER;
  ArgValidation second_arg;
  second_arg.expected_arg_type = ADDRESS;
  v.validations[0] = first_arg;
  v.validations[1] = second_arg;
  if (!validate_args(args, v)) {
    return;
  }

  Arg a1 = args.args[0];
  Arg a2 = args.args[1];

  if (a2.addr.type == A_REGISTER) {
    int addr = registers[a2.addr.val];
    if (addr < 0 || addr >= MEM_BYTES) {
      printf("ldr: out of bounds memory access at address %i\n", addr);
      return;
    }
    registers[a1.reg] = memory[addr];
  } else if (a2.addr.type == A_CONSTANT) {
    registers[a1.reg] = memory[a2.addr.val];
  }

  return;
}

void str(char line[MAX_LINE_LEN]) {
  Args args = parse_args(line + CMD_LEN + 1);
  if (!args.is_valid) {
    return;
  }

  ArgValidations v;
  v.cmd_pretty_str = "str";
  v.expected_arg_count = 2;
  ArgValidation first_arg;
  first_arg.expected_arg_type = REGISTER;
  ArgValidation second_arg;
  second_arg.expected_arg_type = ADDRESS;
  v.validations[0] = first_arg;
  v.validations[1] = second_arg;
  if (!validate_args(args, v)) {
    return;
  }

  Arg a1 = args.args[0];
  Arg a2 = args.args[1];

  if (a2.addr.type == A_REGISTER) {
    int addr = registers[a2.addr.val];
    if (addr < 0 || addr >= MEM_BYTES) {
      printf("str: out of bounds memory access at address %i\n", addr);
      return;
    }
    memory[addr] = registers[a1.reg];
  } else if (a2.addr.type == A_CONSTANT) {
    if (a2.addr.val < 0 || a2.addr.val >= MEM_BYTES) {
      printf("str: out of bounds memory access at address %i\n", a2.addr.val);
      return;
    }
    memory[a2.addr.val] = registers[a1.reg];
  }
  return;
}

void add_or_sub(char line[MAX_LINE_LEN], bool is_add) {
  Args args = parse_args(line);
  if (!args.is_valid) {
    return;
  }

  ArgValidations v;
  if (is_add) {
    v.cmd_pretty_str = "add";
  } else {
    v.cmd_pretty_str = "sub";
  }
  v.expected_arg_count = 3;
  ArgValidation first_arg;
  first_arg.expected_arg_type = REGISTER;
  ArgValidation second_arg;
  second_arg.expected_arg_type = REGISTER_OR_CONSTANT;
  ArgValidation third_arg;
  third_arg.expected_arg_type = REGISTER_OR_CONSTANT;
  v.validations[0] = first_arg;
  v.validations[1] = second_arg;
  v.validations[2] = third_arg;
  if (!validate_args(args, v)) {
    return;
  }

  Arg a1 = args.args[0];
  Arg a2 = args.args[1];
  Arg a3 = args.args[2];

  int val1 = get_register_or_constant(a2);
  int val2 = get_register_or_constant(a3);

  if (!is_add) {
    val2 = val2 * -1;
  }
  registers[a1.reg] = val1 + val2;
  return;
}

void lsl_or_lsr(char line[MAX_LINE_LEN], bool is_left) {
  Args args = parse_args(line);
  if (!args.is_valid) {
    return;
  }

  ArgValidations v;
  if (is_left) {
    v.cmd_pretty_str = "lsl";
  } else {
    v.cmd_pretty_str = "lsr";
  }
  v.expected_arg_count = 3;
  ArgValidation first_arg;
  first_arg.expected_arg_type = REGISTER;
  ArgValidation second_arg;
  second_arg.expected_arg_type = REGISTER_OR_CONSTANT;
  ArgValidation third_arg;
  third_arg.expected_arg_type = REGISTER_OR_CONSTANT;
  v.validations[0] = first_arg;
  v.validations[1] = second_arg;
  v.validations[2] = third_arg;
  if (!validate_args(args, v)) {
    return;
  }

  Arg a1 = args.args[0];
  Arg a2 = args.args[1];
  Arg a3 = args.args[2];

  int val1 = get_register_or_constant(a2);
  int val2 = get_register_or_constant(a3);

  if (is_left) {
    registers[a1.reg] = val2 << val1;
  } else {
    registers[a1.reg] = val2 >> val1;
  }
  return;
}

void branch(char line[MAX_LINE_LEN], CMD command) {
  if (command == BNE) {
    printf("%s", line);
  }
  return;
}

void log_registers(void) {
  int i = 0;
  printf("registers: [");
  for (; i < NUM_REGISTERS; i++) {
    printf("%i, ", registers[i]);
  }
  printf("]\n");
}

void log_mem(void) {
  int i = 0;
  printf("mem: [");
  for (; i < MEM_BYTES; i++) {
    if (i % 48 == 0) {
      printf("\n");
    }
    printf("%i, ", memory[i]);
  }
  printf("]\n");
}

bool validate_args(Args args, ArgValidations validations) {
  if (args.count != validations.expected_arg_count) {
    printf("%s: expected %i arguments, got %i\n", validations.cmd_pretty_str,
           validations.expected_arg_count, args.count);
    return false;
  }
  int i = 0;
  for (; i < args.count; i++) {
    ArgType cmd_type = args.args[i].tag;
    ArgType expected = validations.validations[i].expected_arg_type;

    if (expected == REGISTER_OR_CONSTANT && !(cmd_type == REGISTER) &&
        !(cmd_type == CONSTANT)) {
      printf("%s: expected arg %i to be a register or a constant.\n",
             validations.cmd_pretty_str, i + 1);
      return false;
    }
    if (expected != REGISTER_OR_CONSTANT && expected != cmd_type) {
      const char* err_msg = "";
      if (expected == REGISTER) {
        err_msg = "register";
      } else if (expected == CONSTANT) {
        err_msg = "constant";
      }
      if (expected == ADDRESS) {
        err_msg = "address";
      }
      printf("%s: expected arg %i to be a %s.\n", validations.cmd_pretty_str,
             i + 1, err_msg);
      return false;
    }
  }
  return true;
}

int get_register_or_constant(Arg a) {
  int val = 0;
  if (a.tag == REGISTER) {
    val = registers[a.reg];
  } else if (a.tag == CONSTANT) {
    val = a.constant;
  }
  return val;
}
