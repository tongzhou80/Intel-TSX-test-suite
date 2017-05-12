## Intel-TSX-test-suite

This project includes a suite of simple programs to test Intels' TSX extension. The suite now includes the following tests

- counting.cpp
  - increment a shared counter using pthread mutex and TSX instruction respetively

## Intel TSX

Intel TSX provides two software interfaces to specify regions of code for transactional execution

- Hardware Lock Elision (HLE)
  - legacy compatible instruction set extension (comprising the XACQUIRE and XRELEASE prefixes) to specify transactional
  regions

- Restricted Transactional Memory (RTM)
  - a new instruction set interface (comprising the XBEGIN, XEND, and XABORT instructions)
  - more flexible than HLE

Since a successful transactional execution ensures an atomic commit, the processor
executes the code region optimistically without explicit synchronization. If synchronization
was unnecessary for that specific execution, execution can commit without
any cross-thread serialization. If the processor cannot commit atomically, the optimistic
execution fails. When this happens, the processor will roll back the execution,
a process referred to as a transactional abort. On a transactional abort, the
processor will discard all updates performed in the region, restore architectural state
to appear as if the optimistic execution never occurred, and resume execution nontransactionally.
  
## Check Support

`gcc checktsx.c -o check; ./check`

The script checks all ISA extensions introduced in Haswell, only TSX support is relevant for this project.

## Build

Make sure your platform and your gcc supports `-mrtm -mhle` and c++11 std::chrono.

`make -f makefile-counting`

`./benchmark --help`

## Resources

