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
APP=oarm
BUILD_DIR=build

build(){
    mkdir -p $BUILD_DIR
    $CC $CFLAGS src/main.c -o $BUILD_DIR/$APP
}

run(){
    build    
    $BUILD_DIR/$APP "$1"
}