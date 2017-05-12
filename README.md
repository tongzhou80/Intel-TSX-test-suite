## Intel-TSX-test-suite

This project includes a suite of simple programs to test Intels' TSX extension.

## Check Support

`gcc checktsx.c -o check; ./check`

The script checks all ISA extensions introduced in Haswell, only TSX support is relevant for this project.

## Build

Make sure your platform and your gcc supports `-mrtm -mhle` and c++11 std::chrono.

`make -f makefile-counting`
`./benchmark --help`