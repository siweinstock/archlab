#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#define ADD		0
#define SUB		1
#define AND		2
#define OR		3
#define XOR		4
#define MUL		5
#define SLL		6
#define SRA		7
#define SRL		8
#define BEQ		9
#define BNE		10
#define BLT		11
#define BGT		12
#define BLE		13
#define BGE		14
#define JAL		15
#define LW		16
#define SW		17
#define RETI	18
#define IN		19
#define OUT		20
#define HALT	21

#define MASK			0x3FF				// 10 rightmost bits
#define DMEM_LEN		4096				// size of simulated memory
#define MAXLEN			50					// maximum row length
#define SIGN_EX_MASK	0xFFF00000			// used for sign extension
#define LINES_IN_SECTOR 128					// number of lines in one SECTOR_LINES
#define SECTORS_IN_HD	128					// number of sectors in hard drive
#define DISK_MAX_TIME	1024				// clock cycles for disk operation
#define X_PIXELS		352					// monitor pixels on X axis
#define Y_PIXELS		288					// monitor pixels on Y axis
#define RESOLUTION		X_PIXELS * Y_PIXELS	// total pixels in monitor

#define DEBUG	0
#define FDE		1		// toggle fetch-decode-execute (for debugging)

// translate between IO registers indices and names
char* IOReg[] = { "irq0enable","irq1enable","irq2enable","irq0status","irq1status","irq2status", "irqhandler","irqreturn",
				  "clks", "leds","reserved", "timerenabled", "timercurrent", "timermax",
				  "diskcmd","disksector", "diskbuffer","diskstatus",
				  "monitorcmd", "monitorx", "monitory", "monitordata" };


long pc = 0;					// program counter
int is_irq = 0;					// boolean - is there a pending interrupt request?
int is_handling_irq = 0;		// boolean - is currently handling an interruption request?
int next_irq2;					// clock in which next IRQ2 interruption occurs

int R[16] = { 0 };				// general purpose registers
int IORegister[22] = { 0 };		// IO registers

int disk_timer;					// timer for the hard disk

int imem_img[DMEM_LEN];							// instructions memory image
int dmem_img[DMEM_LEN];							// data memory image
int HD_img[SECTORS_IN_HD * LINES_IN_SECTOR];	// hard drive image

int frame_buffer[RESOLUTION] = { 0 };			// screen pixel setting

int instuction_count = 0;		// number of instructions executed by processor since simulation began

FILE* leds;						// leds file

// struct for holding fragments of parsed operation
typedef struct Operation {
	int opcode;
	int rd;
	int rs;
	int rt;
	int op;
} oper;

// validate all opcodes and registers are valid and within boundries
// if valid returns 1 otherwise returns 0
int validate_operation(oper* operation) {
	// if invalid opcode
	if (operation->opcode > 21 || operation->opcode < 0) {
		printf("EINVAL opcode");
		return 0;
	}
	// if IN/OUT operation and invalid hardware register
	if ((operation->opcode == IN || operation->opcode == OUT) && (operation->rd + operation->rs < 0 || operation->rd + operation->rs > 21)) {
		printf("EINVAL hw register");
		return 0;
	}
	// if not IN/OUT operation and invalid register
	if ((operation->opcode != IN && operation->opcode != OUT) &&
		(operation->rt > 15 || operation->rt < 0 ||
			operation->rs > 15 || operation->rs < 0 ||
			operation->rd > 15 || operation->rd < 0)) {
		printf("EINVAL register");
		return 0;
	}
	// valid operation
	return 1;

}

// populate operation structure with current operation data
int parse_operation(int pc, oper* operation) {
	int line = imem_img[pc];

	operation->rt = line & 0xF;
	operation->rs = (line & 0xF0) >> 4;
	operation->rd = (line & 0xF00) >> 8;
	operation->opcode = (line & 0xFF000) >> 12;
	operation->op = line;

	// if invalid opcode replace with nop
	if (!validate_operation(operation)) {
		operation->rt = 0;
		operation->rs = 0;
		operation->rd = 0;
		operation->opcode = 0 & 0xFF;
		operation->op = 0 & 0xFFFFF;
		return 0;
	}
	// if this operation uses immediate
	else if (operation->rt == 1 || operation->rs == 1 || operation->rd == 1) {
		R[1] = imem_img[pc + 1];
		return 1;
	}
	else {
		return 0;
	}

}

// Write final state of all registers to regout
void write_regout(FILE* regout) {
	int i;
	for (i = 2; i < 16; i++) {
		fprintf(regout, "%08X\n", R[i]);
	}
}


// trace general purpose registers
void trace_gp_reg(FILE* trace, oper* current_oper) {
	int i;

	fprintf(trace, "%03X ", pc); // PC
	fprintf(trace, "%05X ", current_oper->op); // INST
	fprintf(trace, "%08X ", 0); // R0

	if (DEBUG) {
		fprintf(stdout, "%03X ", pc); // PC
		fprintf(stdout, "%05X ", current_oper->op); // INST
		fprintf(stdout, "%08X ", 0); // R0
	}

	if ((R[1] >> 19) & 0x1) { // checking if sign extension is needed for R1
		fprintf(trace, "%08X ", R[1] | SIGN_EX_MASK);
		if (DEBUG)
			fprintf(stdout, "%08X ", R[1] | SIGN_EX_MASK);
	}
	else {
		fprintf(trace, "%08X ", R[1]);
		if (DEBUG)
			fprintf(stdout, "%08X ", R[1]);
	}
	for (i = 2; i < 15; i++) { // R2-R14
		fprintf(trace, "%08X ", R[i]);
		if (DEBUG)
			fprintf(stdout, "%08X ", R[i]);
	}
	fprintf(trace, "%08X\n", R[15]); // R15
	if (DEBUG)
		fprintf(stdout, "%08X\n", R[15]); // R15
}


// trace hardware registers
void trace_hw_reg(FILE* hwregtrace, oper *op, int is_immediate) {
	if (op->opcode == IN) {
		if (is_immediate) {
			fprintf(hwregtrace, "%d READ %s %08X\n", IORegister[8] + 1, IOReg[R[op->rs] + R[op->rt]], R[op->rd]);
		}
		else {
			fprintf(hwregtrace, "%d READ %s %08X\n", IORegister[8], IOReg[R[op->rs] + R[op->rt]], R[op->rd]);
		}
	}
	else if (op->opcode == OUT) {
		if (is_immediate) {
			fprintf(hwregtrace, "%d WRITE %s %08X\n", IORegister[8] + 1, IOReg[R[op->rs] + R[op->rt]], R[op->rd]);
		}
		else {
			fprintf(hwregtrace, "%d WRITE %s %08X\n", IORegister[8], IOReg[R[op->rs] + R[op->rt]], R[op->rd]);
		}
	}
}

// convert hex characters to decimal value
int conv_char(char ch) {
	if (ch == 'f' || ch == 'F') {
		return 15;
	}
	else if (ch == 'e' || ch == 'E') {
		return 14;
	}
	else if (ch == 'd' || ch == 'D') {
		return 13;
	}
	else if (ch == 'c' || ch == 'C') {
		return 12;
	}
	else if (ch == 'b' || ch == 'B') {
		return 11;
	}
	else if (ch == 'a' || ch == 'A') {
		return 10;
	}
	else if (ch == '9') {
		return 9;
	}
	else if (ch == '8') {
		return 8;
	}
	else if (ch == '7') {
		return 7;
	}
	else if (ch == '6') {
		return 6;
	}
	else if (ch == '5') {
		return 5;
	}
	else if (ch == '4') {
		return 4;
	}
	else if (ch == '3') {
		return 3;
	}
	else if (ch == '2') {
		return 2;
	}
	else if (ch == '1') {
		return 1;
	}
	else if (ch == '0') {
		return 0;
	}
}


// load memory image from file to memory
// mode = number of bits in hex representation (5 or 8)
void load_memory(FILE* memfile, int img[], int mode) {
	char line[MAXLEN];
	int i = 0;
	int msb;

	while (fgets(line, MAXLEN, memfile)) {
		msb = conv_char(line[0]);

		// if negative (MSB is 1)
		if (msb > 7 && mode == 8) {
			printf("NEG %d: %s", i, line);
			img[i] = (int)(-1) * (~(strtoll(line, NULL, 16)) + 1);
		}
		else {
			img[i] = (int)strtol(line, NULL, 16);
		}

		i++;
	}
}

// load memory image from file to memory
// mode = number of bits in hex representation (5 or 8)
void load_instruction_memory(FILE* memfile, int img[], int mode) {
	char line[MAXLEN];
	int i = 0;
	int n;

	while (fgets(line, MAXLEN, memfile)) {
		n = strtol(line, NULL, 16);
		n = (n << (32 - mode * 4)) >> (32 - mode * 4);	// perform sign extension
		img[i] = n;
		i++;
	}
}



// dump memory image contents to file
void dump_memory(FILE* dumpfile, int img[], int mode) {
	int i;

	for (i = 0; i < DMEM_LEN; i++) {
		if (mode == 5) {
			fprintf(dumpfile, "%05X\n", img[i]);
		}
		else if (mode == 8) {
			fprintf(dumpfile, "%08X\n", img[i]);
		}
	}
}


// advance pc and clock registers according to comand type
void update_counters(int pc_update, int is_immediate) {
	IORegister[8]++;	// advance clock
	instuction_count++;

	// update timer if enabled
	if (IORegister[11]) {
		IORegister[12]++;
	}

	if (pc_update && is_immediate) {
		pc += 2;
		IORegister[8]++;
		// update timer if enabled
		if (IORegister[11]) {
			IORegister[12]++;
		}
	}
	else if (!pc_update && is_immediate) {
		IORegister[8]++;
		// update timer if enabled
		if (IORegister[11]) {
			IORegister[12]++;
		}
	}
	else if (pc_update && !is_immediate) {
		pc++;
	}
	else {
		//printf("none of the above\n");
	}
}


// get next line from irq2in.txt file
int get_next_irq2(FILE* irq2in) {
	char next[MAXLEN];
	fgets(next, MAXLEN, irq2in);
	return strtol(next, NULL, 10);
}


// handle interrupt requests 
void handle_irq() {
	is_handling_irq = 1;
	IORegister[7] = pc;
	pc = IORegister[6];
	return;
}

// check for interrupts
int check_irq(FILE* irq2in, int is_imm) {

	//IRQ0
	if (IORegister[11] && (IORegister[12] >= IORegister[13])) {
		IORegister[3] = 1;
		IORegister[12] = 0;
	}

	// IRQ2
	// if command takes only one clock cycle - then next IRQ2 must be now
	// if commads takes two clock cycles - the next IRQ2 can be either now or on next clock
	if ((!is_imm && IORegister[8] == next_irq2) || (IORegister[8] == next_irq2 || IORegister[8] == next_irq2 - 1)) {
		IORegister[5] = 1;
		next_irq2 = get_next_irq2(irq2in);
	}

	return ((IORegister[0] && IORegister[3]) || (IORegister[1] && IORegister[4]) || (IORegister[2] && IORegister[5]));
}

// ============================ HARD DISK ======================================

// performs disk read or write
void disk_op(void) {

	if (IORegister[17])// if disk is busy return
		return;

	// translate sector number to line number in HD_img array
	int sectorStart = IORegister[15] * 128;

	// if we need to read from the hard drive
	if (IORegister[14] == 1) {
		for (int i = 0; i < 128; i++) {
			dmem_img[i + IORegister[16]] = HD_img[i + sectorStart];
		}
	}
	// if we need to write to the hard drive
	else {
		for (int i = 0; i < 128; i++) {
			HD_img[i + sectorStart] = dmem_img[i + IORegister[16]];
		}
	}
	IORegister[17] = 1; // set disk to busy
	disk_timer = 0;		// set the timer of the hard disk to zero
}

// counts time for disk operation and checks if finnished
void disk_update_and_check() {
	disk_timer++;
	if (disk_timer == DISK_MAX_TIME) {	// if disk finnished its operation
		IORegister[4] = 1;				// set irq1status to 1
		IORegister[14] = 0;				// set diskcmd to 0
		IORegister[17] = 0;				// set diskstatus to 0
	}
}

// ========================= MONOCHROMATIC SCREEN ==============================

// set pixel (x,y) to the be val
void update_pixel(int x, int y, int val) {
	frame_buffer[y * X_PIXELS + x] = val;
	IORegister[18] = 0;	// reset monitorcmd
}

// refresh monitor with updated pixel values
void update_screen(FILE* monitor, FILE* monitor_b) {
	int i;
	for (i = 0; i < RESOLUTION; i++) {
		fprintf(monitor, "%02X\n", frame_buffer[i]);
		fwrite(&frame_buffer[i], sizeof(char), 1, monitor_b);
	}
}

// ============================= LEDS ========================================
// turn on leds
void leds_op(int rd, int with_imm) {
	int old_led = IORegister[9];
	int new_led = R[rd];

	if (old_led != new_led) {
		if (with_imm) {
			fprintf(leds, "%d ", IORegister[8] + 1);
		}
		else {
			fprintf(leds, "%d ", IORegister[8]);
		}

		fprintf(leds, "%08X\n", R[rd]);
	}
}
// ============================================================================

// execute a command
int exec(oper* operation, int is_immediate) {
	int rd = operation->rd, rs = operation->rs, rt = operation->rt;
	int pc_update = 1;

	switch (operation->opcode) {

	case ADD:
		R[rd] = R[rs] + R[rt];
		break;

	case SUB:
		R[rd] = R[rs] - R[rt];
		break;

	case AND:
		R[rd] = R[rs] & R[rt];
		break;

	case OR:
		R[rd] = R[rs] | R[rt];
		break;

	case XOR:
		R[rd] = R[rs] ^ R[rt];
		break;

	case MUL:
		R[rd] = R[rs] * R[rt];
		break;

	case SLL:
		R[rd] = R[rs] << R[rt];
		break;

	case SRA:
		R[rd] = R[rs] >> R[rt];
		break;

	case SRL:
		R[rd] = (int)((unsigned int)R[rs] >> R[rt]);
		break;

	case BEQ:
		if (R[rs] == R[rt]) {
			pc = R[rd] & MASK;
			pc_update = 0;
		}
		break;

	case BNE:
		if (R[rs] != R[rt]) {
			pc = R[rd] & MASK;
			pc_update = 0;
		}
		break;

	case BLT:
		if (R[rs] < R[rt]) {
			pc = R[rd] & MASK;
			pc_update = 0;
		}
		break;

	case BGT:
		if (R[rs] > R[rt]) {
			pc = R[rd] & MASK;
			pc_update = 0;
		}

		break;

	case BLE:
		if (R[rs] <= R[rt]) {
			pc = R[rd] & MASK;
			pc_update = 0;
		}
		break;

	case BGE:
		if (R[rs] >= R[rt]) {
			pc = R[rd] & MASK;
			pc_update = 0;
		}
		break;

	case JAL:
		if (is_immediate) {
			R[15] = pc + 2;
		}
		else {
			R[15] = pc + 1;
		}

		pc = R[rd] & MASK;
		pc_update = 0;
		break;

	case LW:
		if (R[rs] + R[rt] >= 0 && R[rs] + R[rt] < DMEM_LEN) {
			R[rd] = dmem_img[R[rs] + R[rt]];
		}
		break;

	case SW:
		if (R[rs] + R[rt] >= 0 && R[rs] + R[rt] < DMEM_LEN) {
			dmem_img[R[rs] + R[rt]] = R[rd];
		}
		break;

	case RETI:
		is_handling_irq = 0;
		pc = IORegister[7];
		pc_update = 0;
		break;

	case IN:
		R[rd] = IORegister[R[rs] + R[rt]];

		// always read 0 from monitorcmd 
		if (R[rs] + R[rt] == 18) {
			IORegister[R[rd]] = 0;
		}

		break;

	case OUT:
		if (R[rs] + R[rt] == 9) {
			leds_op(rd, is_immediate);
		}

		IORegister[R[rs] + R[rt]] = R[rd];

		if (R[rs] + R[rt] == 14 && (R[rd] == 1 || R[rd] == 2)) {
			disk_op();
		}
		else if (R[rs] + R[rt] == 18 && IORegister[R[rs] + R[rt]]) {
			update_pixel(IORegister[19], IORegister[20], IORegister[21]);
		}

		break;

	case HALT:
		update_counters(0, 0);
		return 1;

	}

	// prevent zero register modification
	if ((operation->opcode < 9 || operation->opcode == LW || operation->opcode == IN) && rd == 0) {
		printf("ZERO %d\n", operation->opcode);
		R[0] = 0;
	}

	update_counters(pc_update, is_immediate);

	return 0;
}




int main(int argc, char* argv[]) {
	int with_imm, halt = 0;
	oper *current_oper = (oper*)malloc(sizeof(oper));

	FILE *imemin, *dmemin, *diskin, *irq2in, *dmemout, *regout, *trace, *hwregtrace, *cycles, *monitor, *monitor_b, *diskout;

	if (argc != 14) {
		fprintf(stderr, "wrong number of arguments (%d)\n", argc);
		exit(EXIT_FAILURE);
	}
	if ((imemin = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "Failed to open %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	if ((dmemin = fopen(argv[2], "r")) == NULL) {
		fprintf(stderr, "Failed to open %s\n", argv[2]);
		exit(EXIT_FAILURE);
	}
	if ((diskin = fopen(argv[3], "r")) == NULL) {
		fprintf(stderr, "Failed to open %s\n", argv[3]);
		exit(EXIT_FAILURE);
	}
	if ((irq2in = fopen(argv[4], "r")) == NULL) {
		fprintf(stderr, "Failed to open %s\n", argv[4]);
		exit(EXIT_FAILURE);
	}
	if ((dmemout = fopen(argv[5], "w")) == NULL) {
		fprintf(stderr, "Failed to open %s\n", argv[5]);
		exit(EXIT_FAILURE);
	}
	if ((regout = fopen(argv[6], "w")) == NULL) {
		fprintf(stderr, "Failed to open %s\n", argv[6]);
		exit(EXIT_FAILURE);
	}
	if ((trace = fopen(argv[7], "w")) == NULL) {
		fprintf(stderr, "Failed to open %s\n", argv[7]);
		exit(EXIT_FAILURE);
	}
	if ((hwregtrace = fopen(argv[8], "w")) == NULL) {
		fprintf(stderr, "Failed to open %s\n", argv[8]);
		exit(EXIT_FAILURE);
	}
	if ((cycles = fopen(argv[9], "w")) == NULL) {
		fprintf(stderr, "Failed to open %s\n", argv[9]);
		exit(EXIT_FAILURE);
	}
	if ((leds = fopen(argv[10], "w")) == NULL) {
		fprintf(stderr, "Failed to open %s\n", argv[10]);
		exit(EXIT_FAILURE);
	}
	if ((monitor = fopen(argv[11], "w")) == NULL) {
		fprintf(stderr, "Failed to open %s\n", argv[11]);
		exit(EXIT_FAILURE);
	}
	if ((monitor_b = fopen(argv[12], "wb")) == NULL) {
		fprintf(stderr, "Failed to open %s\n", argv[12]);
		exit(EXIT_FAILURE);
	}
	if ((diskout = fopen(argv[13], "w")) == NULL) {
		fprintf(stderr, "Failed to open %s\n", argv[13]);
		exit(EXIT_FAILURE);
	}

	// load memory into matcging image
	load_instruction_memory(imemin, imem_img, 5);
	load_memory(dmemin, dmem_img, 8);
	load_memory(diskin, HD_img, 8);

	next_irq2 = get_next_irq2(irq2in);

	fclose(imemin);
	fclose(dmemin);
	fclose(diskin);

	// fetch-decode-execute loop
	while (FDE && !halt) {
		with_imm = parse_operation(pc, current_oper);
		is_irq = check_irq(irq2in, with_imm);
		trace_gp_reg(trace, current_oper);
		trace_hw_reg(hwregtrace, current_oper, with_imm);
		halt = exec(current_oper, with_imm); // if HALT was executed we exit the loop

		if (IORegister[17]) { // if disk is busy
			disk_update_and_check();
		}

		if (is_irq & !is_handling_irq) {
			handle_irq();
		}

		//_getch();
	}

	fprintf(cycles, "%d\n%d\n", IORegister[8], instuction_count); // export data to cycles.txt file
	update_screen(monitor, monitor_b);

	//_getch();
	free(current_oper);

	// export images to output files
	write_regout(regout);
	dump_memory(dmemout, dmem_img, 8);
	dump_memory(diskout, HD_img, 8);

	fclose(regout);
	fclose(hwregtrace);
	fclose(trace);
	fclose(cycles);
	fclose(dmemout);
	fclose(monitor);
	fclose(monitor_b);
	fclose(diskout);
	fclose(leds);

}
