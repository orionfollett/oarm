#include <stdio.h>
#include <string.h>

#define MAX_LINE_LEN 1024

int main(int argc, char** argv) {
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
        printf("Interpreting from file...");
        printf("%s\n", argv[1]);
        /* Change get char to read from file instead of stdin?*/
    }    
  }

  printf("oarm v0.1\n____\n\n");
  
  while(1){
    printf("> ");
    char line[MAX_LINE_LEN] = {0};
    int i = 0;
    for(; i < MAX_LINE_LEN; i++) {
        char c = (char)getc(stdin);
        line[i] = c;
        if(c == EOF || c == '\n'){
            break;
        }
    } 
    
    printf("%s\n", line);
  }
  
  
  
  return 0;
}