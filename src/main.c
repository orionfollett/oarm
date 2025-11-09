#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
  printf("OARM Interpreter\n");

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
    } else {
    }

    printf("%s\n", argv[1]);
  }

  return 0;
}