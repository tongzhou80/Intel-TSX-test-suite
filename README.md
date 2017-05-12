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

## Transactional Abort

A primary abort cause is due to conflicting accesses between the transactionally executing logical
processor and another logical processor. Memory addresses read from within a transactional
region constitute the **read-set** of the transactional region and addresses
written to within the transactional region constitute the **write-set** of the transactional
region. Intel TSX maintains the read- and write-sets **at the granularity of a
cache line**. A conflicting access occurs if another logical processor

- either reads a location that is part of the transactional region's write-set
- writes a location that is a part of either the read-set or write-set of the transactional region.

A conflicting access typically means serialization is indeed required for this code region.
  
## Check Support

`gcc checktsx.c -o check; ./check`

The script checks all ISA extensions introduced in Haswell, only TSX support is relevant for this project.

## Build

Make sure your platform and your gcc supports `-mrtm -mhle` and c++11 std::chrono.

`make -f makefile-counting`

`./benchmark --help`

## Resources

