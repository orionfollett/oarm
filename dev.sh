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
SRC_DIR=src

build(){
    mkdir -p $BUILD_DIR
    $CC $CFLAGS src/main.c -o $BUILD_DIR/$APP
}

runf(){
    build    
    $BUILD_DIR/$APP "$1"
}

run(){
    build    
    $BUILD_DIR/$APP
}

fmt() {
    clang-format --style Chromium -i $SRC_DIR/*.c 2>/dev/null || true
}