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
TEST=test
BUILD_DIR=build
SRC_DIR=src

build(){
    rm -rf $BUILD_DIR/
    mkdir -p $BUILD_DIR
    $CC $CFLAGS -c $SRC_DIR/oarm.c -o $BUILD_DIR/oarm.o
    $CC $CFLAGS $SRC_DIR/main.c $BUILD_DIR/oarm.o -o $BUILD_DIR/$APP
    $CC $CFLAGS $SRC_DIR/test.c $BUILD_DIR/oarm.o -o $BUILD_DIR/$TEST
}

run(){
    build || return
    $BUILD_DIR/$APP "$1"
}

test(){
    build || return
    $BUILD_DIR/$TEST
}

fmt() {
    clang-format --style Chromium -i $SRC_DIR/*.c 2>/dev/null || true
}