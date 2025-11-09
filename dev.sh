CC=gcc
CFLAGS=(-std=c89)
run(){
    mkdir -p build
    $CC $CFLAGS main.c -o build/app
    ./build/app
}