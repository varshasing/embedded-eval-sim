#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define LOCALMEMORY 256
// check to stay in bounds of 8-bit signed integer
#define CHECK_8BIT(x) (((x) > 127) ? 127 : ((x) < -128) ? -128 : (x))
/* 
*  8-bit embedded processor
*  recognize MOV ADD LD ST CMP JE and JMP
*  suppoort byte-addressable 256-byte local memory
*  count total number of executed instructions
*  compute total cycle count
*       each MOV ADD CMP or JMP takes 1 clock cycle
*       each LD/ST takes 2 cycles of address in local memory
*       
*/

/* read disassembled code for the architecture

STRUCTURE OF AN INSTRUCTION
max 4 fields for r-type and ld/st instructions
line; instruction type; Register(r-type) Address (j-type) memory location (ld/st); register (r-type) immediate (i-type) 
*/

/* constants the represent recognizable instructions */
typedef enum {
    MOV,
    ADD,
    CMP,
    JE,
    JMP,
    LD,
    ST
} instruction_type;

/* flexible instruction components */
typedef struct instruction {
    int line;
    instruction_type type;
    int reg1;
    int reg2;
    int immediate;
    bool is_immediate;
} instruction;

/* dynamic array (vector) */
typedef struct vector {
    instruction *data;      // pointer to array
    size_t size;            // current size
    size_t capacity;        // allocated size
    int start_line;         // first line number (unbiasing)
} instruction_vector;

typedef struct local_memory {
    int memory[LOCALMEMORY];        // values stored in memory
    bool occupied[LOCALMEMORY];     // is the memory location occupied?
} local_memory;

/* initalize memory, all unoccupied, all complicit misses */
void init_localmem(local_memory *mem)
{
    /* glibc memset, optimized with vectorized instructions */
    memset(mem->memory, 0, sizeof(mem->memory));
    memset(mem->occupied, 0, sizeof(mem->occupied));
}

/* load a value from memory */
int load_memory(local_memory *mem, int address, int *cycle_count, int *local_hit)
{
    *cycle_count += (mem->occupied[address]) ? 2 : 45;
    *local_hit += (mem->occupied[address]) ? 1 : 0;
    mem->occupied[address] = true;
    return mem->memory[address];
}

/* store value into memory */
void store_memory(local_memory *mem, int address, int value, int *cycle_count, int *local_hit)
{
    *cycle_count += (mem->occupied[address]) ? 2 : 45;
    *local_hit += (mem->occupied[address]) ? 1 : 0;
    mem->memory[address] = value;
    mem->occupied[address] = true;
}

/* initalize vector with overestimate of size (CURRENT: 500 instructions )*/
void init_vector(instruction_vector *vec)
{
    vec->data = (instruction *)malloc(500 * sizeof(instruction));
    if(!vec->data)
    {
        perror("instruction vector allocation FAILED");
        exit(1);
    }
    vec->size = 0;
    vec->capacity = 500;
}

/* resize vector, double each time */
void resize_vector(instruction_vector *vec)
{
    vec->capacity *= 2;
    vec->data = (instruction *)realloc(vec->data, vec->capacity * sizeof(instruction));
    if(!vec->data)
    {
        perror("instruction vector reallocation FAILED");
        exit(1);
    }
}

/* free memory from vector */
void free_vector(instruction_vector *vec)
{
    free(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

/* tokenize the instruction */
instruction instruction_decoder(char* line)
{
    instruction ins;
    char opcode[10], op1[10], op2[10] = "";
    int parsed;

    /* initialize instruction */
    ins.line = 0;
    ins.type = -1;
    ins.reg1 = 0;
    ins.reg2 = 0;
    ins.immediate = 0;
    ins.is_immediate = false;

    /* separate instruction into components */
    parsed = sscanf(line, "%d %s %s %s", &ins.line, opcode, op1, op2);
    if(parsed < 2)
    {
        printf("Unknown instruction: %s\n", line);
        exit(1);
    }

    /* assign instruction values based on instruction */
    if(strcmp(opcode, "MOV") == 0)
    {
        ins.type = MOV;
    }
    else if(strcmp(opcode, "ADD") == 0)
    {
        ins.type = ADD;
    }
    else if(strcmp(opcode, "ST") == 0)
    {
        ins.type = ST;
    }
    else if(strcmp(opcode, "CMP") == 0)
    {
        ins.type = CMP;
    }
    else if(strcmp(opcode, "JE") == 0)
    {
        ins.type = JE;
    }
    else if(strcmp(opcode, "JMP") == 0)
    {
        ins.type = JMP;
    }
    else if(strcmp(opcode, "LD") == 0)
    {
        ins.type = LD;
    }
    else
    {
        printf("Unknown instruction: %s\n", line);
        exit(1);
    }

    /* Different instruction formats */
    /* MOV and ADD; accounts for R-Type and I-Type ADD */
    if(ins.type == MOV || ins.type == ADD)
    {
        sscanf(op1, "R%d", &ins.reg1);
        /* R-Type ADD */
        if(op2[0] == 'R')
        {
            sscanf(op2, "R%d", &ins.reg2);
        }
        /* I-Type */
        else
        {
            ins.is_immediate = true;
            ins.immediate = atoi(op2);
        }
    }
    /* ST store instructions ST [Rm], Rn*/
    else if(ins.type == ST)
    {
        sscanf(op1, "[R%d]", &ins.reg2);
        sscanf(op2, "R%d", &ins.reg1);
    }
    /* J-Type instructions */
    else if(ins.type == JMP || ins.type == JE)
    {
        sscanf(op1, "%d", &ins.reg1);
    }
    /* R-Type Compare */
    else if(ins.type == CMP)
    {
        sscanf(op1, "R%d", &ins.reg1);
        sscanf(op2, "R%d", &ins.reg2);
    }
    /* LD load instructions LD Rn, [Rm]*/
    else if(ins.type == LD)
    {
        sscanf(op1, "R%d", &ins.reg1);
        sscanf(op2, "[R%d]", &ins.reg2);
    }

    return ins;
}

/* add instruction to instruction vector */
void append_instruction(instruction_vector *vec, instruction ins)
{
    if(vec->size == vec->capacity)
    {
        resize_vector(vec);
    }
    vec->data[vec->size++] = ins;
}

/* read instructions from file, parse, and store in vector */
void load_instruction(FILE* filename, instruction_vector *vec)
{
    if(!filename)
    {
        perror("ERROR IN FILE");
        exit(1);
    }

    char line[100];         // buffer for line
    while(fgets(line, sizeof(line), filename)) {
        instruction ins = instruction_decoder(line);
        append_instruction(vec, ins);
    }

    fclose(filename);
}

/* processor execution logic */
void execute_assembly(instruction_vector *vec)
{
    /* 6 general purpose registers, 256 different local memory addresses */
    int registers[7] = {0};
    local_memory localmemory;                   // will likely have to change to a hash map :(
    int instruction_count = 0;
    int cycle_count = 0;
    int local_hit = 0;
    int LD_ST = 0;
    size_t ip = 0;     // instruction pointer for vec->data
    vec->start_line = vec->data[0].line;        // offset for indexing for J-type
    int bias = vec->start_line;
    bool cmp_flag = false;
    init_localmem(&localmemory);

    while(ip < vec->size)
    {
        instruction ins = vec->data[ip];
        instruction_count++;

        /* cases for current instruction NEED TO CHECK THAT IT IS IN SIGNED 8 RANGE */
        switch (ins.type)
        {
            /* move the immediate to the register, one cycle */
            case MOV:
                registers[ins.reg1] = ins.immediate;
                cycle_count++;
                break;
            /* either add r2 value to r1, or add immediate, one cycle*/
            case ADD:
                registers[ins.reg1] += ins.is_immediate ? ins.immediate : registers[ins.reg2];
                cycle_count++;
                break;
            /* compare rn and rm, set cmp_flag value, one cycle */
            case CMP:
                cmp_flag = (registers[ins.reg1] == registers[ins.reg2]) ? true : false;
                cycle_count++;
                break;
            /* jump if cmp_flag is on, one cycle */
            case JE:
                if(cmp_flag)
                {
                    ip = ins.reg1 - bias;
                    cycle_count++;
                    continue;
                }
                cycle_count++;
                break;
            /* unconditional jump, one cycle */
            case JMP:
                ip = ins.reg1 - bias;
                cycle_count++;
                continue;
            /* load, must check if address is in local memory */
            case LD:
                registers[ins.reg1] = load_memory(&localmemory, registers[ins.reg2], &cycle_count, &local_hit);
                LD_ST++;
                break;
            /* store, must check if address is in local memory */
            case ST:
                store_memory(&localmemory, registers[ins.reg2], registers[ins.reg1], &cycle_count, &local_hit);
                LD_ST++;
                break;
            default:
                printf("Unknown instruction type: %d\n", ins.type);
                exit(1);
        }
        ip++;
    }

    printf("Total number of executed instructions: %d\n", instruction_count);
    printf("Total number of clock cycles: %d\n", cycle_count);
    printf("Number of hits to local memory: %d\n", local_hit);
    printf("Total number of executed LD/ST instructions: %d\n", LD_ST);
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    FILE *file = fopen(argv[1], "r");
    if(!file)
    {
        perror("ERROR IN FILE");
        exit(1);
    }

    instruction_vector vec;
    init_vector(&vec);
    load_instruction(file, &vec);
    execute_assembly(&vec);

    free_vector(&vec);
    return 0;
}
