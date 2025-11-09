#include <stdio.h>
#include <string.h>

#define bool int
#define true 1
#define false 0

#define MAX_LINE_LEN 1024
#define NUM_REGISTERS 10
#define MEM_BYTES 4096

/*Globals*/
int registers[NUM_REGISTERS] = {0};
int memory[MEM_BYTES] = {0};

/*Types*/
typedef enum {MOV, ADD, RET, UNKNOWN} CMD;

/*Declarations*/
bool tick(char line[MAX_LINE_LEN]);
CMD identify_cmd(char cmd[3]);

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
      line[i] = c;
      if (c == EOF || c == '\n') {
        break;
      }
    }

    cont = tick(line);
  }

  return 0;
}

/*
    parse
*/
bool tick(char line[MAX_LINE_LEN]) {
    /*Evaluate one line of assembly, update global registers and memory where needed.*/

    char cmd_str[3] = {0};
    memcpy(cmd_str, line, 3*sizeof(char));
    CMD cmd = identify_cmd(cmd_str);

    /* line identified by first three chars */
    if(cmd==MOV) {
        printf("MOV command detected\n");
    }
    else if (cmd==ADD){
        printf("ADD command detected\n");
    }
    else if (cmd==RET){
        printf("RET command detected\n");
    }
    else if(cmd==UNKNOWN){
        printf("Error could not parse statement identifier: %c%c%c\n", cmd_str[0], cmd_str[1], cmd_str[2]);
        return false;
    }
    return true;
}

CMD identify_cmd(char cmd[3]) {
    if(cmd[0] == 'm'){
        return MOV;
    }
    else if(cmd[0] == 'a'){
        return ADD;
    }
    else if(cmd[0] == 'r'){
        return RET;
    }
    return UNKNOWN;
}