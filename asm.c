/*
 * SP ASM: Simple Processor assembler
 *
 * usage: asm
 */
#include <stdio.h>
#include <stdlib.h>

#define ADD 0
#define SUB 1
#define LSF 2
#define RSF 3
#define AND 4
#define OR  5
#define XOR 6
#define LHI 7
#define LD 8
#define ST 9
#define CPY 10
#define POL 11
#define JLT 16
#define JLE 17
#define JEQ 18
#define JNE 19
#define JIN 20
#define HLT 24

#define MEM_SIZE_BITS	(16)
#define MEM_SIZE	(65536)
#define MEM_MASK	(MEM_SIZE - 1)
unsigned int mem[MEM_SIZE];

int pc = 0;

static void asm_cmd(int opcode, int dst, int src0, int src1, int immediate)
{
    int inst;

    inst = ((opcode & 0x1f) << 25) | ((dst & 7) << 22) | ((src0 & 7) << 19) | ((src1 & 7) << 16) | (immediate & 0xffff);
    mem[pc++] = inst;
}

static void assemble_program(char *program_name)
{
    FILE *fp;
    int addr, i, last_addr;

    for (addr = 0; addr < MEM_SIZE; addr++)
        mem[addr] = 0;

    pc = 0;

    /*
     * Program starts here
     */
asm_cmd(ADD, 2,1,0, 16);
asm_cmd(ST, 0,2,1,200);
asm_cmd(ST, 0,2,1,201);
asm_cmd(ST, 0,2,1,202);
asm_cmd(ST, 0,2,1,203);
asm_cmd(ST, 0,2,1,204);
asm_cmd(ADD,3,1,0,250); //R[3]=250
asm_cmd(ADD,0,0,0,0);
asm_cmd(ADD,5,1,0,200);//R[5]=200
asm_cmd(ADD, 6,1,0,10); //R[6]=10
asm_cmd(CPY, 3,5,6,0); //dst=250, src=200, len=10
asm_cmd(POL,4,0,0,0);
asm_cmd(JNE,0,0,4,11);
asm_cmd(HLT ,0 ,0 ,0 ,0);

for (i = 0; i < 8; i++)
mem[14+i] = i;

last_addr = 22;


    fp = fopen(program_name, "w");
    if (fp == NULL) {
        printf("couldn't open file %s\n", program_name);
        exit(1);
    }
    addr = 0;
    while (addr < last_addr) {
        fprintf(fp, "%08x\n", mem[addr]);
        addr++;
    }
}


int main(int argc, char *argv[])
{

    if (argc != 2){
        printf("usage: asm program_name\n");
        return -1;
    }else{
        assemble_program(argv[1]);
        printf("SP assembler generated machine code and saved it as %s\n", argv[1]);
        return 0;
    }

}


//
//asm_cmd(ADD ,2, 0, 1, 0x87);
//asm_cmd(ADD ,3 ,0, 1, 5);
//asm_cmd(ST ,0, 2, 1, 1000);
//asm_cmd(ST ,0, 3, 1, 1001);
//asm_cmd(ADD ,4, 0, 2, 0);
//asm_cmd(ADD ,5 ,0 ,3 ,0);
//asm_cmd(AND, 2,2,1,0x80);
//asm_cmd(AND, 3,3,1,0x80);
//asm_cmd(SUB ,4, 4, 2, 0 );
//asm_cmd(SUB ,5 ,5 ,3 ,0 );
//asm_cmd(JEQ ,0, 2, 3, 18 );
//asm_cmd(JLT ,0 ,4 ,5 ,15  );
//asm_cmd(SUB ,6, 4, 5, 0 );
//asm_cmd(OR ,6 ,6 ,2 ,0 );
//asm_cmd(JIN ,0 ,1, 0 ,20  );
//asm_cmd(SUB ,6 ,5 ,4 ,0  );
//asm_cmd(OR ,6 ,6 ,3 ,0  );
//asm_cmd(JIN ,0 ,1 ,0 ,20 );
//asm_cmd(ADD ,6, 4, 5, 0  );
//asm_cmd(OR ,6 ,6 ,2 ,0 );
//asm_cmd(ST ,0 ,6 ,1 ,1002  );
//asm_cmd(HLT ,0 ,0 ,0 ,0);

//for (i = 0; i < 8; i++)
//mem[22+i] = i;
//
//last_addr = 30;


//asm_cmd(ADD ,6, 0, 0, 0);
//asm_cmd(ADD ,4 ,0, 1, 21000);
//asm_cmd(ST ,0 ,4 ,1 ,1000);
//asm_cmd(ADD ,5, 0, 1, 1 );
//asm_cmd(LSF ,5 ,5 ,1 ,30);
//asm_cmd(JLT ,0, 5, 4, 8 );
//asm_cmd(RSF ,5 ,5 ,1 ,2);
//asm_cmd(JIN ,0, 1, 0, 5 );
//asm_cmd(JEQ ,0 ,0 ,5 ,18);
//asm_cmd(ADD ,3, 6, 5, 0);
//asm_cmd(JLT ,0 ,3 ,4 ,13);
//asm_cmd(RSF ,6, 6, 1, 1 );
//asm_cmd(JIN ,0 ,1 ,0 ,16);
//asm_cmd(SUB ,4, 4, 3, 0);
//asm_cmd(RSF ,6 ,6 ,1 ,1 );
//asm_cmd(ADD ,6 ,6 ,5, 0);
//asm_cmd(RSF ,5 ,5 ,1 ,2 );
//asm_cmd(JIN ,0 ,1 ,0 ,8 );
//asm_cmd(ST ,0 ,6 ,1 ,1001 );
//asm_cmd(HLT,0,0,0,0);
//
// * Constants are planted into the memory somewhere after the program code:
// */
//for (i = 0; i < 8; i++)
//mem[20+i] = i;
//
//last_addr = 28;
//
//asm_cmd(ADD ,2, 0, 1, 0x87);
//asm_cmd(ADD ,3 ,0, 1, 5);
//asm_cmd(ST ,0, 2, 1, 1000);
//asm_cmd(ST ,0, 3, 1, 1001);
//asm_cmd(ADD ,4, 0, 2, 0);
//asm_cmd(ADD ,5 ,0 ,3 ,0);
//asm_cmd(RSF,2,2,1,31);
//asm_cmd(AND, 2,2,1,1);
//asm_cmd(LSF, 2,2,1,31);
//asm_cmd(RSF,3,3,1,31);
//asm_cmd(AND, 3,3,1,1);
//asm_cmd(LSF,3,3,1,31);
//asm_cmd(SUB ,4, 4, 2, 0 );
//asm_cmd(SUB ,5 ,5 ,3 ,0 );
//asm_cmd(JEQ ,0, 2, 3, 18 );
//asm_cmd(JLT ,0 ,4 ,5 ,15  );
//asm_cmd(SUB ,6, 4, 5, 0 );
//asm_cmd(OR ,6 ,6 ,2 ,0 );
//asm_cmd(JIN ,0 ,1, 0 ,24  );
//asm_cmd(SUB ,6 ,5 ,4 ,0  );
//asm_cmd(OR ,6 ,6 ,3 ,0  );
//asm_cmd(JIN ,0 ,1 ,0 ,24 );
//asm_cmd(ADD ,6, 4, 5, 0  );
//asm_cmd(OR ,6 ,6 ,2 ,0 );
//asm_cmd(ST ,0 ,6 ,1 ,1002  );
//asm_cmd(HLT ,0 ,0 ,0 ,0);
