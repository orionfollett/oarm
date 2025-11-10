#include <math.h>
#include <stdio.h>
#include <string.h>

#define bool int
#define true 1
#define false 0

#define MAX_LINE_LEN 128
#define MAX_IDENT_LEN 32
#define CMD_LEN 3
#define ARG_LEN (MAX_IDENT_LEN - CMD_LEN - 1)
#define NUM_REGISTERS 10
#define MEM_BYTES 4096

/*Globals*/
int registers[NUM_REGISTERS] = {0};
int memory[MEM_BYTES] = {0};

/*Types*/
typedef enum { MOV, ADD, RET, UNKNOWN } CMD;
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
Args parse_args(char line[MAX_LINE_LEN]);
int parse_int(char num[MAX_IDENT_LEN - CMD_LEN - 1]);
bool mov(char line[MAX_LINE_LEN]);

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
    printf("MOV command detected\n");
    cont = mov(line);
  } else if (cmd == ADD) {
    printf("ADD command detected\n");
  } else if (cmd == RET) {
    printf("RET command detected\n");
  } else if (cmd == UNKNOWN) {
    printf("Error could not parse statement identifier: %c%c%c\n", cmd_str[0],
           cmd_str[1], cmd_str[2]);
    cont = false;
  }
  return cont;
}

CMD identify_cmd(char cmd[CMD_LEN]) {
  if (cmd[0] == 'm') {
    return MOV;
  } else if (cmd[0] == 'a') {
    return ADD;
  } else if (cmd[0] == 'r') {
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

int parse_int(char num[MAX_IDENT_LEN]) {
  /*Given an array of chars, return an int. Char array must be null
   * terminated.*/

  /*subtract 48 to convert ASCII to int for single digit*/

  /*go backwards until EOF is found*/
  int result = 0;
  int place = 0;
  int i = MAX_IDENT_LEN - 1;
  for (; i >= 0; i--) {
    if (num[i] == EOF || num[i] == 0) {
      place = 1;
      continue;
    }
    if (place == 0) {
      continue;
    }
    int digit = (int)num[i] - 48;
    result = result + place * digit;
    place = place * 10;
  }
  return result;
}

Args parse_args(char line[ARG_LEN]) {
  Args args;
  args.count = 0;
  args.is_valid = true;
  int i = 0;
  char arg_str[MAX_IDENT_LEN] = {0};

  for (; i < ARG_LEN; i++) {
    if (line[i] == ',' || line[i] == EOF) {
      Arg a;
      if (arg_str[0] == '[') {
        a.tag = ADDRESS;
        a.addr = 1;
        /*TODO: not implemented yet*/
      } else if (arg_str[0] == 'x') {
        a.tag = REGISTER;
        char num_str[MAX_IDENT_LEN];
        bool end_hit = false;
        int j = 1;
        for (; j < MAX_IDENT_LEN - 1; j++) {
          if (arg_str[j] >= (int)'0' && arg_str[j] <= (int)'9') {
            num_str[j] = arg_str[j];
          } else {
            num_str[j] = 0;
            end_hit = true;
            break;
          }
        }
        /*Clean this up its disgusting.*/
        if (!end_hit) {
          args.is_valid = false;
          num_str[MAX_IDENT_LEN - 1] = 0;
          printf("Invalid int passed: %s", num_str);
          return args;
        }
        a.reg = parse_int(num_str);
      } else if (arg_str[0] == '#') {
        a.tag = CONSTANT;
        a.constant = 1;
      } else {
        args.is_valid = false;
        return args;
      }

      args.args[args.count] = a;
      args.count++;
      memset(arg_str, 0, sizeof(arg_str));
    }
    if (line[i] == EOF) {
      break;
    }
    if (line[i] != ' ') {
      arg_str[i] = line[i];
    }
  }
  return args;
}

bool mov(char line[MAX_LINE_LEN]) {
  /* Parse and execute a mov command.
      mov x0, x1;

  */
  Args a = parse_args(line);
  if (!a.is_valid) {
    return false;
  }
  printf("Args parsed.\n");
  printf("Count: %i", a.count);
  if(a.args[0].tag == REGISTER){
    printf("REGISTER %i\n", a.args[0].reg);
  }
  if(a.args[0].tag == CONSTANT){
    printf("CONSTANT %i\n", a.args[0].constant);
  }

  return true;
}
