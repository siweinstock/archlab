//
// Created by eden on 10/25/22.
//


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#define MEMORY_SIZE 65536
#define REGISTER_SIZE 6

//instruction struct :
typedef struct Instruction {
    int opcode; //5 bits
    int dst; // 3 bits
    int src0; // 3 bits
    int src1;  // 3 bits
    int imm; //16 bits


} Instruction;


static int * INSTRUCTIONS_MEM = (int *)(calloc(0,MEMORY_SIZE * (sizeof(int))));
static int * MEM= (int *)(calloc(0,MEMORY_SIZE * (sizeof(int))));
static int * REG= (int *)(calloc(0,REGISTER_SIZE * (sizeof(int))));


/*
Input argument: imemin.txt file , INSTRUCTIONS_MEM- the memory array
Output arguments :None
Functionality : function read every line from the file and convert every line to a decimal number.
*/
void init_instructions_mem(FILE* instructions) {
    char line[14]; //  12 for opcode rd, rs, rt, rm, imm1, imm2 +"\0"
    int i = 0;
    while (!feof(instructions)) {
        if (fgets(line, 14, instructions) == nullptr)
            break;
        if (strcmp(line, "\n") == 0 || strcmp(line, "\0") == 0 || strcmp(line, "0") == 0) // ignore white spaces
            continue;
        INSTRUCTIONS_MEM[i] = (int)strtol(line, nullptr, 16);
        i += 1;
    }
}


/*
Input arguments:INSTRUCTIONS_MEM, curr_inst - pointer to  "Instruction" struct, current PC
Output arguments : None
Functionality: function converts every line and fills the fields of the instruction
*/
void line_to_instruction( int PC, Instruction* curr_inst) {
    curr_inst->opcode = (INSTRUCTIONS_MEM[PC] & 0x3E000000) >> 25;
    curr_inst->dst = (INSTRUCTIONS_MEM[PC] & 0x1C00000) >> 22;
    curr_inst->src0 = (INSTRUCTIONS_MEM[PC] & 0x380000) >> 19;
    curr_inst->src1 = (INSTRUCTIONS_MEM[PC] & 0x70000) >> 16;
    curr_inst->imm = (INSTRUCTIONS_MEM[PC] & 0x00FF);
}

/*
Input argument: trace.txt file, PC- the current PC, REG- current REG array, INSTRUCTIONS_MEM
Output arguments: None
functionality : function prints to trace.txt file every instruction that was executed.
*/
void PrintToTrace(FILE* trace, int PC) {
    REG[0] = 0;
    fprintf(trace, "%03X %012X %08x %08x %08x %08x %08x %08x\n", PC, INSTRUCTIONS_MEM[PC], REG[0], REG[1], REG[2], REG[3], REG[4], REG[5]);
}


//start of opcode instructions:

/*
Input argument: curr_inst- pointer to the current instruction, PC- the current PC , REG -register array
Output arguments :new_PC- the new PC according the opcode.
Functionality: function adds the arguments in register rs, rt, and rm.
*/
int ADD(Instruction* curr_inst, int PC) {
    REG[curr_inst->dst] = REG[curr_inst->src0] + REG[curr_inst->src1];
    PC += 1;
    return PC;
}

/*
Input argument: curr_inst- pointer to the current instruction, PC- the current PC , REG -register array
Output arguments :new_PC- the new PC according the opcode.
Functionality: function sub the arguments in register rs, rt, and rm.
*/
int SUB(Instruction* curr_inst, int PC) {
    REG[curr_inst->dst] = REG[curr_inst->src0] - REG[curr_inst->src1];
    PC += 1;
    return PC;
}

/*
Input argument: curr_inst- pointer to the current instruction, PC- the current PC , REG -register array
Output arguments :new_PC- the new PC according the opcode.
Functionality: function multiplier the arguments in register rs and rt and adds the arguments in register rm.
*/
int LSF(Instruction* curr_inst, int PC) {
    REG[curr_inst->dst] = REG[curr_inst->src0] << REG[curr_inst->src1];
    PC += 1;
    return PC;
}

int RSF(Instruction* curr_inst, int PC) {
    REG[curr_inst->dst] = REG[curr_inst->src0] >> REG[curr_inst->src1];
    PC += 1;
    return PC;
}

/*
Input argument: curr_inst- pointer to the current instruction, PC- the current PC , REG -register array
Output arguments :new_PC- the new PC according the opcode.
Functionality: the function do "and" of the arguments in register rs, rt and  rm.
*/
int AND(Instruction* curr_inst, int PC) {
    REG[curr_inst->dst] = REG[curr_inst->src0] & REG[curr_inst->src1];
    PC += 1;
    return PC;
}

/*
Input argument: curr_inst- pointer to the current instruction, PC- the current PC , REG -register array
Output arguments :new_PC- the new PC according the opcode.
Functionality: the function do "or" of the arguments in register rs, rt and  rm.
*/
int OR(Instruction* curr_inst, int PC) {
    REG[curr_inst->dst] = REG[curr_inst->src0] | REG[curr_inst->src1];
    PC += 1;
    return PC;
}

/*
Input argument: curr_inst- pointer to the current instruction, PC- the current PC , REG -register array
Output arguments :new_PC- the new PC according the opcode.
Functionality: the function do "xor" of the arguments in register rs, rt and  rm.
*/
int XOR(Instruction* curr_inst, int PC) {
    REG[curr_inst->dst] = REG[curr_inst->src0] ^ REG[curr_inst->src1];
    PC += 1;
    return PC;
}

/*
Input argument: curr_inst- pointer to the current instruction, PC- the current PC , REG -register array
Output arguments :new_PC- the new PC according the opcode.
Functionality: the function do shift logical left of the arguments in register rs.
*/
int LHI(Instruction* curr_inst, int PC) {
    REG[curr_inst->dst] = ((curr_inst->imm)<<16)+REG[curr_inst->dst]&16;
    PC += 1;
    return PC;
}

int JLT(Instruction* curr_inst, int PC) {
    if (REG[curr_inst->src0] <  REG[curr_inst->src1]) {
        PC = curr_inst->imm;
        return PC;
    }
    PC += 1;
    return PC;
}
int JLE(Instruction* curr_inst, int PC) {
    if (REG[curr_inst->src0] <=  REG[curr_inst->src1]) {
        PC = curr_inst->imm;
        return PC;
    }
    PC += 1;
    return PC;
}
int JEQ(Instruction* curr_inst, int PC) {
    if (REG[curr_inst->src0] == REG[curr_inst->src1]) {
        PC = curr_inst->imm;
        return PC;
    }
    PC += 1;
    return PC;
}
int JNE(Instruction* curr_inst, int PC) {
    if (REG[curr_inst->src0] != REG[curr_inst->src1]) {
        PC = curr_inst->imm;
        return PC;
    }
    PC += 1;
    return PC;
}
int JIN(Instruction* curr_inst, int PC) {
    PC = REG[curr_inst->src0];
    return PC;
}


/*
Input argument: curr_inst- pointer to the current instruction, PC- the current PC , REG -register array, MEM- current memory array
Output arguments :new_PC- the new PC according the opcode.
Functionality: function loads a value from an address (rs+rt) in the memory  and adds rm.
*/
int LD(Instruction* curr_inst, int PC) {
   // REG[curr_inst->rd] = MEM[REG[curr_inst->rs] + REG[curr_inst->rt]] + REG[curr_inst->rm];
    PC += 1;
    return PC;
}
/*
Input argument: curr_inst- pointer to the current instruction, PC- the current PC , REG -register array, MEM- current memory array
Output arguments :new_PC- the new PC according the opcode.
Functionality: function stores a value from  rm + rd  into an address in the memory(rs+rt) .
*/
int ST(Instruction* curr_inst, int PC) {
    MEM[REG[curr_inst->src1]]= REG[curr_inst->src0];
    PC += 1;
    return PC;
}

/*
Input arguments: dmemout.txt file, Mem - memory array
Output arguments : None
Functionality:function prints to "dmemout" file
*/
void PrintToDmemout(FILE* sram) {
    int num_of_lines = MEMORY_SIZE;
    for (int i = 0; i < num_of_lines; i++) {
        fprintf(sram, "%08X\n", MEM[i]);
    }
}


/*
Input arguments:pointers to all the files we need to print in , DISK - pointer to DISK array,
Mem- pointer to memory array ,REG-registers array, IOREG -IO registers array
Output arguments : None
Functionality : function calls to all the printing functions
*/
void PrintToFile(FILE* fdmemout) {
    PrintToDmemout(fdmemout);
}

/*
Input arguments: all files
Output arguments : None
Functionality :function closes all the files.
*/
void close_all_files(FILE* instructions, FILE* sram, FILE* trace) {
    fclose(instructions);
    fclose(sram);
    fclose(trace);
}

/*
Input argument: curr_inst- pointer to the current instruction in the memory,
PC- the current PC, DISK- current disk array, MONITOR- current monitor array,
INSTRUCTIONS_MEM - current memory array, REG- current registers array, pointers to files
Output argument: new_PC- the new PC according the opcode.
Functionality: identifys the opcode, excute the intruction and change the PC according the opcode.
*/
int OpcodeInstruction(Instruction* curr_inst, int PC, FILE* sram) {
    int new_PC;
    switch (curr_inst->opcode) {
        case(0):
            new_PC = ADD(curr_inst, PC);
            break;
        case(1):
            new_PC = SUB(curr_inst, PC);
            break;
        case(2):
            new_PC = LSF(curr_inst, PC);
            break;
        case(3):
            new_PC = RSF(curr_inst, PC);
            break;
        case(4):
            new_PC = AND(curr_inst, PC);
            break;
        case(5):
            new_PC = OR(curr_inst, PC);
            break;
        case(6):
            new_PC = XOR(curr_inst, PC);
            break;
        case(7):
            new_PC = LHI(curr_inst, PC);
            break;
        case(8):
            new_PC = LD(curr_inst, PC);
            break;
        case(9):
            new_PC = ST(curr_inst, PC);
            break;
        case(16):
            new_PC = JLT(curr_inst, PC);
            break;
        case(17):
            new_PC = JLE(curr_inst, PC);
            break;
        case(18):
            new_PC = JEQ(curr_inst, PC);
            break;
        case(19):
            new_PC = JNE(curr_inst, PC);
            break;
        case(20):
            new_PC = JIN(curr_inst, PC);
            break;
        case(24):  //halt
            new_PC = -1;
            PrintToFile(sram);
            break;
        default:
            new_PC = PC + 1;
    }
    return new_PC;
}

/*
Input arguments: INSTRUCTIONS_MEM array, DISK array , REG array ,IOREG array ,irq2 array + pointers to all the files
Output arguments: None
functionality : function executes the "fetch and decode " loop- simulator functionality
*/
void Fetch_And_Decode(FILE* sram, FILE* trace) {
    int PC = 0;
    Instruction* curr_inst;
    Instruction inst;
    curr_inst = &inst;

    while (PC > -1) { // PC will be -1 only when "halt" is executed
        line_to_instruction(PC, curr_inst); // convert line to instruction
        PrintToTrace(trace, PC); //print each instruction executed by the simulator into trace file
        PC = OpcodeInstruction(curr_inst, PC,sram );
    }
}


int main(int argc, char* argv[]) {
    FILE* instructions;
    FILE* trace;
    FILE* sram;
    if (argc != 2) {
        printf("Not enough input files. Exit program.\n");
        exit(1);
    }
    //open all files
    instructions = fopen(argv[1], "r");
    if (instructions == nullptr) {
        exit(1);
    }
    char *ptr= strtok(argv[1], ".");
    trace= fopen((std::string(ptr) + "_trace.txt").c_str(), "w");
    sram= fopen((std::string(ptr) + "_sram_out.txt").c_str(), "w");
    if (sram == nullptr || trace == nullptr) {
        exit(1);
    }
    //initialize INSTRUCTIONS_MEM:
    init_instructions_mem(instructions);

    Fetch_And_Decode(sram, trace);

    //close all files
    close_all_files(instructions, sram, trace);

    return 0;
}




