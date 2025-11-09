CC=gcc
CFLAGS=(
    -Wall
    -Wextra
    -Werror
    -Wshadow
    -Wconversion
    -Wcast-align
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wfloat-equal
    -Wundef
    -fsanitize=address
    -fsanitize=undefined
    -g
    -std=c89
    -O2
)

build(){
    mkdir -p build
    $CC $CFLAGS src/main.c -o build/app
}

run(){
    build    
    ./build/app "$1"
}