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
typedef enum { MOV, ADD, RET, UNKNOWN, REG } CMD;
typedef int Register;
typedef int Address;

typedef enum { ADDRESS, REGISTER, CONSTANT } ArgType;

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
  if (cmd == MOV) {
    mov(line);
  } else if (cmd == ADD) {
    printf("ADD command detected\n");
  } else if (cmd == RET) {
    printf("RET command detected\n");
  } else if (cmd == UNKNOWN) {
    printf("Error could not parse statement identifier: %c%c%c\n", cmd_str[0],
           cmd_str[1], cmd_str[2]);
  } else if (cmd == REG){
    log_registers();
  }
  return cont;
}

CMD identify_cmd(char cmd[CMD_LEN]) {
  if (cmd[0] == 'm') {
    return MOV;
  } else if (cmd[0] == 'a') {
    return ADD;
  } else if (cmd[0] == 'r') {
    if(cmd[1] == 'e' && cmd[2] == 'g') {
        return REG;
    }
    return RET;
  }
  return UNKNOWN;
}

/*
get args -> split, trim
convert string to register (which is just an index into an array) (could take
into account register labels later)

args could be:
- memory address (wrapped in '[]')
- register name (starts with 'x')
- register label (not supported yet)
- constant (prefixed with '#')

args are always comma separated

different commands expect different number of args
*/

int parse_int(const char* num, int len) {
  /*Given an array of chars, return an int. Char array must be null
   * terminated.*/
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
      if (line[arg_start] == '[') {
        a.tag = ADDRESS;
        a.addr = 1;
        /*TODO: not implemented yet*/
      } else if (line[arg_start] == 'x') {
        a.tag = REGISTER;
        a.reg = parse_int(line + arg_start + 1, i - arg_start - 1);
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
  /* Parse and execute a mov command.
      mov x0, x1;

  */
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
    printf(
        "Expected first argument to mov to be a register or register label\n");
    return false;
  }

  if (a1.reg > NUM_REGISTERS || a1.reg < 0) {
    printf(
        "First argument's register is out of range, must be between 0 and %i\n",
        NUM_REGISTERS);
    return false;
  }

  int val = 0;
  if (a2.tag == REGISTER) {
    val = registers[a2.reg];
  } else if (a2.tag == CONSTANT) {
    val = a2.constant;
  } else {
    printf(
        "Expected second argument to mov to be a register, register label, or "
        "constant\n");
    return false;
  }

  registers[a1.reg] = val;
  return true;
}

void log_registers(void){
    int i = 0;
    printf("registers: [");
    for(; i<NUM_REGISTERS; i++){
        printf("%i, ", registers[i]);
    }
    printf("]\n");
}
