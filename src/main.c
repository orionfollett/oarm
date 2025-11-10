#include <math.h>
#include <stdio.h>
#include <string.h>

#define bool int
#define true 1
#define false 0

#define MAX_LINE_LEN 128
#define MAX_IDENT_LEN 32
#define CMD_LEN 3
#define ARGS_LEN (MAX_LINE_LEN - CMD_LEN - 1)
#define NUM_REGISTERS 10
#define MEM_BYTES 4096

/*Globals*/
int registers[NUM_REGISTERS] = {0};
int memory[MEM_BYTES] = {0};

/*Types*/
typedef enum { ADD, LDR, MOV, REG, RET, STR, UNKNOWN } CMD;
typedef int Register;

typedef enum { A_CONSTANT, A_REGISTER } AddressType;
typedef struct Address {
  int val;
  AddressType type;
} Address;

typedef enum { ADDRESS, CONSTANT, REGISTER } ArgType;

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

/*Declarations*/
bool tick(char line[MAX_LINE_LEN]);
CMD identify_cmd(char cmd[CMD_LEN]);

Args parse_args(char line[ARGS_LEN]);
int parse_int(const char* num, int len);

bool mov(char line[MAX_LINE_LEN]);
bool ldr(char line[MAX_LINE_LEN]);
bool sdr(char line[MAX_LINE_LEN]);
void log_registers(void);
/*Implementations*/

int main(int argc, char** argv) {
  FILE* input_stream = stdin;

  if (argc > 1) {
    if (strcmp("--help", argv[1]) == 0) {
      printf(
          "Usage: oarm [FILE]\n"
          "\n"
          "If FILE is provided, oarm will assemble and run it.\n"
          "If no FILE is given, oarm starts in interactive REPL mode.\n"
          "\n"
          "Examples:\n"
          "  oarm program.s      Assemble and run program.s\n"
          "  oarm                Start REPL\n"
          "\n"
          "Options:\n"
          "  --help              Show this help message and exit\n");
      return 0;
    } else {
      input_stream = fopen(argv[1], "r");
      if (input_stream == NULL) {
        perror("Error opening file");
        return 1;
      }
    }
  }

  printf("oarm v0.1\n____\n\n");

  bool cont = true;
  while (cont) {
    printf("> ");
    char line[MAX_LINE_LEN] = {0};
    int i = 0;
    for (; i < MAX_LINE_LEN; i++) {
      char c = (char)getc(input_stream);
      if (c == EOF || c == '\n') {
        line[i] = EOF;
        break;
      }
      line[i] = c;
    }

    cont = tick(line);
  }

  return 0;
}

/*
    parse
*/
bool tick(char line[MAX_LINE_LEN]) {
  /*Evaluate one line of assembly, update global registers and memory where
   * needed.*/
  bool cont = true;
  char cmd_str[CMD_LEN] = {0};
  memcpy(cmd_str, line, CMD_LEN * sizeof(char));
  CMD cmd = identify_cmd(cmd_str);

  /* line identified by first three chars */

  switch (cmd) {
    case ADD:
      printf("ADD command detected\n");
      break;
    case LDR:
      ldr(line);
      break;
    case MOV:
      mov(line);
      break;
    case REG:
      log_registers();
      break;
    case RET:
      break;
    case STR:
      str(line);
      break;
    case UNKNOWN:
      printf("Error could not parse statement identifier: %c%c%c\n", cmd_str[0],
             cmd_str[1], cmd_str[2]);
      break;
  }
  return cont;
}

CMD identify_cmd(char cmd[CMD_LEN]) {
  if (cmd[0] == 'm') {
    return MOV;
  } else if (cmd[0] == 'a') {
    return ADD;
  } else if (cmd[0] == 'r') {
    if (cmd[1] == 'e' && cmd[2] == 'g') {
      return REG;
    }
    return RET;
  } else if (cmd[0] = 'l') {
    return LDR;
  } else if (cmd[0] = 's') {
    return STR;
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
      printf("Non digit detected in parse int string\n");
      return 0;
    }
    result += place * (num[i] - (int)'0');
    place = place * 10;
  }

  return result;
}

Args parse_args(char line[ARGS_LEN]) {
  Args args;
  args.count = 0;
  args.is_valid = true;

  int i = 0;
  int arg_start = 0;
  for (; i < ARGS_LEN; i++) {
    bool line_end = line[i] == EOF || line[i] == '\n';
    bool arg_end = line[i] == ',' || line_end;
    if (arg_end) {
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
              args.count);
          args.is_valid = false;
          return args;
        }
        int start = arg_start + 2;
        int end = i - arg_start - 2;
        if (end - start < 1) {
          printf(
              "Argument %i [col %i], expected numerical value for memory "
              "address argument, got empty string.\n",
              args.count, i);
          args.is_valid = false;
          return args;
        }
        a.addr.val = parse_int(line + start, end);

        if (a.addr.type == A_CONSTANT) {
          if (a.addr.val > MEM_BYTES || a.addr.val < 0) {
            printf(
                "Argument %i memory address is out of range, must be between 0 "
                "and %i\n",
                args.count, MEM_BYTES);
            args.is_valid = false;
            return args;
          }
        }

        /*Register argument*/
      } else if (line[arg_start] == 'x') {
        a.tag = REGISTER;
        a.reg = parse_int(line + arg_start + 1, i - arg_start - 1);
        if (a.reg > NUM_REGISTERS || a.reg < 0) {
          printf(
              "Argument %i register is out of range, must be between 0 and "
              "%i\n",
              args.count, NUM_REGISTERS);
          args.is_valid = false;
          return args;
        }

        /*constant argument*/
      } else if (line[arg_start] == '#') {
        a.tag = CONSTANT;
        a.constant = parse_int(line + arg_start + 1, i - arg_start - 1);
      } else {
        args.is_valid = false;
        printf(
            "Error parsing args, couldn't recognize arg type. last char "
            "parsed: %i\n",
            line[i]);
        return args;
      }

      args.args[args.count] = a;
      args.count++;
      arg_start = i + 1;
      continue;
    }
    if (line_end) {
      break;
    }
    if (line[i] == ' ') {
      arg_start++;
    }
  }
  return args;
}

bool mov(char line[MAX_LINE_LEN]) {
  Args args = parse_args(line + CMD_LEN + 1);
  if (!args.is_valid) {
    return false;
  }

  if (args.count != 2) {
    printf("Expected exactly 2 args for mov\n");
    return false;
  }

  Arg a1 = args.args[0];
  Arg a2 = args.args[1];

  if (a1.tag != REGISTER) {
    printf("mov: expected first argument to be a register or register label\n");
    return false;
  }

  int val = 0;
  if (a2.tag == REGISTER) {
    val = registers[a2.reg];
  } else if (a2.tag == CONSTANT) {
    val = a2.constant;
  } else {
    printf(
        "mov: expected second argument to be a register, register label, "
        "constant\n");
    return false;
  }

  registers[a1.reg] = val;
  return true;
}

bool ldr(char line[MAX_LINE_LEN]) {
  Args args = parse_args(line + CMD_LEN + 1);

  if (!args.is_valid) {
    return false;
  }

  if (args.count != 2) {
    printf("ldr: expected 2 arguments got %i\n", args.count);
  }

  Arg a1 = args.args[0];
  Arg a2 = args.args[1];

  if (a1.tag != REGISTER) {
    printf("ldr: expected first argument to be a register.\n");
  }
  if (a2.tag != ADDRESS) {
    printf("ldr: expected second argument to be an address.\n");
  }

  if (a2.addr.type == A_REGISTER) {
    int addr = registers[a2.addr.val];
    if (addr < 0 || addr >= MEM_BYTES) {
      printf("ldr: out of bounds memory access at address %i\n", addr);
      return false;
    }
    registers[a1.reg] = memory[addr];
  } else if (a2.addr.type == A_CONSTANT) {
    registers[a1.reg] = memory[a2.addr.val];
  }

  return true;
}

bool str(char line[MAX_LINE_LEN]) {
  Args args = parse_args(line + CMD_LEN + 1);

  if (!args.is_valid) {
    return false;
  }

  if (args.count != 2) {
    printf("ldr: expected 2 arguments got %i\n", args.count);
  }

  Arg a1 = args.args[0];
  Arg a2 = args.args[1];

  if (a1.tag != REGISTER) {
    printf("ldr: expected first argument to be a register.\n");
  }
  if (a2.tag != ADDRESS) {
    printf("ldr: expected second argument to be an address.\n");
  }

  if (a2.addr.type == A_REGISTER) {
    int addr = registers[a2.addr.val];
    if (addr < 0 || addr >= MEM_BYTES) {
      printf("ldr: out of bounds memory access at address %i\n", addr);
      return false;
    }
    registers[a1.reg] = memory[addr];
  } else if (a2.addr.type == A_CONSTANT) {
    registers[a1.reg] = memory[a2.addr.val];
  }

  return true;
}

void log_registers(void) {
  int i = 0;
  printf("registers: [");
  for (; i < NUM_REGISTERS; i++) {
    printf("%i, ", registers[i]);
  }
  printf("]\n");
}
