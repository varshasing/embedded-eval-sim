Varsha Singh

Designing a simple simulator to evaluate an embeddded system.

### Instruction set simulator
Designing a simple instruction set simulator (ISS) for an 8-big embedded processor.

### Capabilities of Simulator
- Recognizing the MOV, ADD, LD, ST, CMP, JE, and JMP instructions (described below in more detail)
- Supporting 6 general purpose integer registers (each register has 8 bits), named R1, R2, ..., R6
- Supporting a byte-addressable 256-Byte local memory
- Counting the total number of executed instructions
- Computing the total cycle count based on the following assumptions:
	- Each move, add, compare, or jump instruction takes 1 clock cycle
	- Each load/store instruction takes 2 cycles when the requrested address is already in the local memory and 45 cycles if the data must be brought from external memory

The simulator is able to directly read disassembled code for this architecture (does not need to read any executable files, nothing that most real-life simulators would be reading directly from an executable generated by the compiler).

### ISA Instructions:
- MOV RN <num> % puts an 8-bit (positive/negative) integer <num> into RN (range of num: [-128, 127])
- ADD Rn, Rm % performs RN + Rm, and writes the result to RN
- ADD Rn, <num> % performs RN + num and writes the result in Rn (range of num: [-128, 127])
- CMP Rn, Rm % compared Rn and Rm *typically used together with the JE instruction)
- JE <Address> % jumps to instruction at <Address> if the last comparison resulted in equality
- JMP <Adderss> % unconditionally jumps to the instruction at <Address>
- LD Rn, [Rm] % loads from the address stored in Rm into Rn
- ST [Rm], Rn % stores the contents of Rn into the memory address that is in Rm

### Local Memory
Local memory in this embedded system is an overly-simplistic cache. When an application code starts running on the simulator, the local memory does not have any relevant data. First time each memory address in requested, assume there is a "miss" in local memory, then the access delay is only 2 cycles.

- Does not consider code memory in this simulator. Code is given in text format and an assumption is that code address spacec does not conflict with data addresses.

## How to Use:

Download `Makefile`, `simpleISS.c`, and `my.assembly` <br>
`make simpleISS` <br>
`> ./simpleISS sample.assembly` <br>
Total number of executed instructions: 91 <br>
Total number of clock cycles: 536 <br>
Number of hits to local memory: 5 <br>
Total number of executed LD/ST instructions: 15
