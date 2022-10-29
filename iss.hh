//
// Created by shiw1 on 10/25/2022.
//

#ifndef ARCHLAB1_ISS_H
#define ARCHLAB1_ISS_H

#define DEBUG 1
#define SHOWMEM 0
#define SCREEN 0
#define GETCH 0

#define OPLEN 14
#define MEMLEN 65536
#define REGNUM 8

#define ADD 0
#define SUB 1
#define LSF 2
#define RSF 3
#define AND 4
#define OR 5
#define XOR 6
#define LHI 7
#define LD 8
#define ST 9
#define JLT 16
#define JLE 17
#define JEQ 18
#define JNE 19
#define JIN 20
#define HLT 24

typedef struct OP {
    int inst;
    int opcode;
    int dst;
    int src0;
    int src1;
    int imm;
} op;


#endif //ARCHLAB1_ISS_H
