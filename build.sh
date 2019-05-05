mkdir -p build
cd build

CFLAGS="-g -Wall -O0 -Wno-writable-strings -Wno-unused-variable"
LIBS="-lSDL2 -lGL"

clang $LIBS $CFLAGS ../livefx.cpp -o livefx
