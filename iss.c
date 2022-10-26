#include <stdio.h>
#include <stdlib.h>
#include "iss.h"


// global variables
int R[8];           // registers content
long PC = 0;        // program counter register
int SRAM[MEMLEN];   // static memory

int inst_counter;   // count total executed instructions
char* oplist[] = {"ADD", "SUB", "LSF", "RSF", "AND", "OR", "XOR", "LHI", "LD",
                  "ST", "invalid", "invalid", "invalid", "invalid", "invalid", "invalid",
              "JLT", "JLE", "JEQ", "JNE", "JIN", "invalid", "invalid", "invalid",
              "HLT"};   // opcode to name translation


// populate command struct
void parse_op(int hex, op* cmd) {
    cmd->inst = hex;
    cmd->opcode = (hex >> 25) & 0x1f;
    cmd->dst = (hex >> 22) & 0x7;
    cmd->src0 = (hex >> 19) & 0x7;
    cmd->src1 = (hex >> 16) & 0x7;
    cmd->imm = hex & 0xffff;
}

// dump command contents
void print_cmd(op* cmd) {
    fprintf(stdout, "--- instruction %d (%04x) @ PC %d (%04x) -----------------------------------------------------------\n",
           inst_counter, inst_counter, PC, PC);
    fprintf(stdout, "pc = %04d, ", PC);
    fprintf(stdout, "inst = %08x, ", cmd->inst);
    fprintf(stdout, "opcode = %d (%s), ", cmd->opcode, oplist[cmd->opcode]);
    fprintf(stdout, "dst = %d, ", cmd->dst);
    fprintf(stdout, "src0 = %d, ", cmd->src0);
    fprintf(stdout, "src1 = %d, ", cmd->src1);
    fprintf(stdout, "immediate = %08x\n", cmd->imm);

}

// convert a command from hex form to integer
int op2int(char* op) {
    return (int) strtol(op, NULL, 16);
}

// display registers values
void regdump() {
    for (int i=0; i<REGNUM; i++) {
        fprintf(stdout, "r[%d] = %08x ", i, R[i]);
        if ((i+1) % (REGNUM/2) == 0) fprintf(stdout, " \n");
    }
    fprintf(stdout, "\n");
}

// TODO: figure out what to print for other commands
void execute(op* cmd) {
    // effective operands
    int src0 = (cmd->src0 == 1) ? cmd->imm : R[cmd->src0];
    int src1 = (cmd->src1 == 1) ? cmd->imm : R[cmd->src1];

    R[1] = cmd->imm;
    int update_pc = 1;  // increment PC flag
    int dst = cmd->dst; //, src0 = cmd->src0, src1 = cmd->src1;

    // their regdump happens before the command execution
    if (DEBUG)
        regdump();

    switch (cmd->opcode) {
        case ADD:
            R[dst] = src0 + src1;
            if (DEBUG)
                fprintf(stdout, ">>>> EXEC: R[%d] = %d ADD %d <<<<\n", dst, src0, src1);
            break;
        case SUB:
            R[dst] = src0 - src1;
            if (DEBUG)
                fprintf(stdout, ">>>> EXEC: R[%d] = %d SUB %d <<<<\n", dst, src0, src1);

            break;
        case LSF:
            R[dst] = src0 << src1;
            if (DEBUG)
                fprintf(stdout, ">>>> EXEC: R[%d] = %d LSF %d <<<<\n", dst, src0, src1);

            break;
        case RSF:
            R[dst] = (int) ((unsigned int)src0 >> src1); // sign extended
            if (DEBUG)
                fprintf(stdout, ">>>> EXEC: R[%d] = %d RSF %d <<<<\n", dst, src0, src1);

            break;
        case AND:
            R[dst] = src0 & src1;
            if (DEBUG)
                fprintf(stdout, ">>>> EXEC: R[%d] = %d AND %d <<<<\n", dst, src0, src1);

            break;
        case OR:
            R[dst] = src0 | src1;
            if (DEBUG)
                fprintf(stdout, ">>>> EXEC: R[%d] = %d OR %d <<<<\n", dst, src0, src1);

            break;
        case XOR:
            R[dst] = src0 ^ src1;
            if (DEBUG)
                fprintf(stdout, ">>>> EXEC: R[%d] = %d XOR %d <<<<\n", dst, src0, src1);

            break;
        case LHI:
            R[dst] ^= ((R[dst] << 16) >> 16);   // set top 16 bits to zero
            R[dst] |= (R[1] << 16); // set top 16 bits to immediate value
            break;
        case LD:
            R[dst] = SRAM[src1];
            if (DEBUG)
                fprintf(stdout, ">>>> EXEC: R[%d] = MEM[%d] = %08x <<<<\n", dst, src1, SRAM[src1]);
            break;
        case ST:
            SRAM[src1] = src0;
            if (DEBUG)
                fprintf(stdout, ">>>> EXEC: MEM[%d] = R[%d] = %08x <<<<\n", src1, cmd->src0, SRAM[src1]);
            break;
        case JLT:
            if (src0 < src1) {
                update_pc = 0;
                R[7] = PC;
                PC = cmd->imm;
            }
            break;
        case JLE:
            if (src0 <= src1) {
                update_pc = 0;
                R[7] = PC;
                PC = cmd->imm;
            }
            break;
        case JEQ:
            if (src0 == src1) {
                update_pc = 0;
                R[7] = PC;
                PC = cmd->imm;
            }
            if (DEBUG)
                fprintf(stdout, ">>>> EXEC: JEQ %d, %d, %d <<<<\n", src0, src1, update_pc ? PC+1 : PC);

            break;
        case JNE:
            if (src0 != src1) {
                update_pc = 0;
                R[7] = PC;
                PC = cmd->imm;
            }
            break;
        case JIN:
            update_pc = 0;
            R[7] = PC;
            PC = src0;
            break;
        case HLT:
            update_pc = 0;
            if (DEBUG)
                fprintf(stdout, ">>>> EXEC: HALT at PC %04x <<<<\n", PC);
            break;
        // illegal operation, NOP
        default:
            fprintf(stderr, "ERROR: INVALID OPERATION\n");
            inst_counter--;
            break;
    }

    if (update_pc) PC = (PC + 1) % 0x10000; // increment PC but don't allow overflow beyond 0xffff
    inst_counter++;

    R[0] = 0;   // fix R[0] with zero value

}

// load memory image from file to memory
// mode = number of bits in hex representation (5 or 8)
int load_mem(FILE* memfile, int img[], int mode) {
    char line[OPLEN];
    int i = 0;

    while (fgets(line, OPLEN+1, memfile)) {
        img[i++] = op2int(line);
//        fprintf(stdout, "%08X\n", img[i-1]);
    }
    return i;
}


// TODO: identify and handle edge cases
// TODO: test in Linux environment
int main(int argc, char* argv[]) {

    op* cmd = (op*) malloc(sizeof(op));
    FILE *f_inst;   // TODO: add print to file support
    int lines;

    if (argc != 2) {
        fprintf(stdout, "usage: iss <mem_state.bin>\n");
        exit(argc);
    }

    if ((f_inst = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Failed to open %s\n", argv[1]);
    }
    lines = load_mem(f_inst, SRAM, 5);  // load SRAM

    fprintf(stdout, "program %s loaded, %d lines\n\n", argv[1], lines);

    // FDE emulation
    while (cmd->opcode != HLT) {
        parse_op(SRAM[PC], cmd);
        print_cmd(cmd);
        execute(cmd);

        if (GETCH)
            getchar();
    }

    fprintf(stdout, "sim finished at pc %d, %d instructions\n", PC, inst_counter);

    free(cmd);
    fclose(f_inst);

    return 0;
}
