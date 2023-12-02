# Memory Hierarchy Simulator: UCLA CS M151B Fall 2023 Computer Assignment 2
This repository contains a C++ solution to Computer Assignment 2 for UCLA CS M151B Fall 2023, a simulator of a memory controller and cache hierarchy for a simple RISC-V processor that implements a direct-mapped L1 cache, fully-associative victim cache, and 8-way set associative L2 cache.

## Running
Navigate to the directory containing the extracted implementation, then simply run `make` with the included Makefile.
```
cd Memory_Hierarchy_Simulator_CS_M151B_F23_CA2/src
make
```

The exact commands run by `make` are as follows:

```
g++ -Wall -O2 -pipe -fno-plt -fPIC *.cpp -o memory_driver
```

The program takes one argument, which is a .txt file containing instructions. The format is as follows: 
```
Read(LW instruction)?, Write(SW instruction)?, address, data
```
Only one of Read? or Write? can have a value of 1, and data should be 0 for a read (LW) instruction. Example input files are provided in the ./trace folder.

The program outputs a tuple where the first item is the miss rate of the L1 cache, the second item is the miss rate of the L2 cache, and the last item is the average access time of cache and memory. Per project spec, only LW instructions are factored into the miss rate.

Example run:

```
./memory_driver ../trace/L2-test.txt
(-8,23)
```

## Cleaning up
Navigate to the directory containing the extracted implementation, then simply run `make clean` with the included Makefile.

```
cd CPU_Simulator_CS_M151B_F23_CA2/src
make clean
```

The exact commands run by make clean are as follows:

```
rm -f memory_driver
```