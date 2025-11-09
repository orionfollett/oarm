#include <stdio.h>

int main(int argc, char** argv){
    printf("OARM Interpreter\n");

    if(argc > 1){
        printf("%s", argv[1]);
    }
    
    
    return 0;
}