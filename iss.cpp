#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "iss.hh"
using namespace std;


// global variables
int R[8];           // registers content
long PC = 0;        // program counter register
int SRAM[MEMLEN];   // static memory

int inst_counter;   // count total executed instructions
string oplist[] = {"ADD", "SUB", "LSF", "RSF", "AND", "OR", "XOR", "LHI", "LD",
                  "ST", "invalid", "invalid", "invalid", "invalid", "invalid", "invalid",
                  "JLT", "JLE", "JEQ", "JNE", "JIN", "invalid", "invalid", "invalid",
                  "HLT"};   // opcode to name translation

FILE* stream;  // output stream (file or stdout)

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
    fprintf(stream, "--- instruction %d (%04x) @ PC %ld (%04lx) -----------------------------------------------------------\n",
            inst_counter, inst_counter, PC, PC);
    fprintf(stream, "pc = %04ld, ", PC);
    fprintf(stream, "inst = %08x, ", cmd->inst);
    fprintf(stream, "opcode = %d (%s), ", cmd->opcode, oplist[cmd->opcode].c_str());
    fprintf(stream, "dst = %d, ", cmd->dst);
    fprintf(stream, "src0 = %d, ", cmd->src0);
    fprintf(stream, "src1 = %d, ", cmd->src1);
    fprintf(stream, "immediate = %08x\n", cmd->imm);
}

// display registers values
void regdump() {
    for (int i=0; i<REGNUM; i++) {
        fprintf(stream, "r[%d] = %08x ", i, R[i]);
        if ((i+1) % (REGNUM/2) == 0) fprintf(stream, "\n");
    }
    fprintf(stream, "\n");
}

// TODO: verify correctness of commands not in example
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
                fprintf(stream, ">>>> EXEC: R[%d] = %d ADD %d <<<<\n\n", dst, src0, src1);
            break;
        case SUB:
            R[dst] = src0 - src1;
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: R[%d] = %d SUB %d <<<<\n\n", dst, src0, src1);

            break;
        case LSF:
            R[dst] = src0 << src1;
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: R[%d] = %d LSF %d <<<<\n\n", dst, src0, src1);

            break;
        case RSF:
            R[dst] = (int) ((unsigned int)src0 >> src1); // sign extended
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: R[%d] = %d RSF %d <<<<\n\n", dst, src0, src1);

            break;
        case AND:
            R[dst] = src0 & src1;
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: R[%d] = %d AND %d <<<<\n\n", dst, src0, src1);

            break;
        case OR:
            R[dst] = src0 | src1;
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: R[%d] = %d OR %d <<<<\n\n", dst, src0, src1);

            break;
        case XOR:
            R[dst] = src0 ^ src1;
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: R[%d] = %d XOR %d <<<<\n\n", dst, src0, src1);

            break;
        case LHI:
            R[dst] ^= ((R[dst] << 16) >> 16);   // set top 16 bits to zero
            R[dst] |= (R[1] << 16); // set top 16 bits to immediate value
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: R[%d][31:16] = immediate[15:0] <<<<\n\n", dst);
            break;
        case LD:
            R[dst] = SRAM[src1];
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: R[%d] = MEM[%d] = %08x <<<<\n\n", dst, src1, SRAM[src1]);
            break;
        case ST:
            SRAM[src1] = src0;
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: MEM[%d] = R[%d] = %08x <<<<\n\n", src1, cmd->src0, SRAM[src1]);
            break;
        case JLT:
            if (src0 < src1) {
                update_pc = 0;
                R[7] = PC;
                PC = cmd->imm;
            }
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: JLT %d, %d, NEXT PC: %ld <<<<\n\n", src0, src1, update_pc ? PC+1 : PC);

            break;
        case JLE:
            if (src0 <= src1) {
                update_pc = 0;
                R[7] = PC;
                PC = cmd->imm;
            }
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: JLE %d, %d, NEXT PC: %ld <<<<\n\n", src0, src1, update_pc ? PC+1 : PC);
            break;
        case JEQ:
            if (src0 == src1) {
                update_pc = 0;
                R[7] = PC;
                PC = cmd->imm;
            }
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: JEQ %d, %d, NEXT PC: %ld <<<<\n\n", src0, src1, update_pc ? PC+1 : PC);
            break;
        case JNE:
            if (src0 != src1) {
                update_pc = 0;
                R[7] = PC;
                PC = cmd->imm;
            }
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: JNE %d, %d, NEXT PC: %ld <<<<\n\n", src0, src1, update_pc ? PC+1 : PC);
            break;
        case JIN:
            update_pc = 0;
            R[7] = PC;
            PC = src0;
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: JIN NEXT PC: %ld <<<<\n\n", PC);
            break;
        case HLT:
            update_pc = 0;
            if (DEBUG)
                fprintf(stream, ">>>> EXEC: HALT at PC %04lx<<<<\n", PC);
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

// load commands
int load_commands(FILE* memfile, int img[]) {
    char line[OPLEN];
    int i = 0;

    while (fgets(line, OPLEN, memfile)) {
        img[i++] = (int) strtol(line, NULL, 16);    // convert a command from hex form to integer
    }
    return i;
}

// export SRAM contents to file
void dump_mem(FILE* memfile) {
    for (int i=0; i<MEMLEN; i++) {
        fprintf(memfile, "%08x\n", SRAM[i]);
    }
}


int main(int argc, char* argv[]) {

    op* cmd = (op*) malloc(sizeof(op));
    FILE *f_inst, *f_trace, *f_sram;
    int lines;

    if (argc != 2) {
        fprintf(stdout, "usage: iss <mem_state.bin>\n");
        exit(argc);
    }

    if ((f_inst = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Failed to open %s\n", argv[1]);
        exit(-1);
    }
    lines = load_commands(f_inst, SRAM);  // load SRAM
    char *ptr= strtok(argv[1], ".");
    if ((f_trace= fopen((std::string(ptr) + "_trace.txt").c_str(), "w"))==NULL){
        fprintf(stderr, "Failed to open trace file.\n");
        exit(-1);
    }

    stream = SCREEN ? stdout : f_trace;

    if ((f_sram = fopen((std::string(ptr) + "_sram_out.txt").c_str(), "w")) == NULL) {
        fprintf(stderr, "Failed to open sram file.\n");
        exit(-1);
    }

    fprintf(stream, "program %s loaded, %d lines\n\n", argv[1], lines);
    // FDE emulation
    while (cmd->opcode != HLT) {
        parse_op(SRAM[PC], cmd);
        print_cmd(cmd);
        execute(cmd);

        if (GETCH)
            getchar();
    }

    dump_mem(f_sram);
    fprintf(stream, "sim finished at pc %ld, %d instructions", PC, inst_counter);

    free(cmd);
    fclose(f_inst);
    fclose(f_trace);
    fclose(f_sram);

    return 0;
}
