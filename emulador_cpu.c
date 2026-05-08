/*
 * cpu16.c — Emulador de CPU de 16 bits
 *
 * Formato de instrucción (16 bits):
 *   [15..11] OPCODE  (5 bits)
 *   [10.. 7] RD      (4 bits) — registro destino
 *   [ 6.. 3] RA      (4 bits) — registro fuente A
 *   [ 2.. 0] MISC    (3 bits) — RB (registro fuente B) o flags de modo
 *
 * Para instrucciones con inmediato de 16 bits (LDI, JMP, JCC, CALL):
 *   la siguiente palabra en memoria contiene el inmediato.
 *
 * Registros especiales:
 *   R14 = LR  (link register, guarda dirección de retorno en CALL)
 *   R15 = SP  (stack pointer — reservado, no modificar a mano)
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/* ── Tamaños ─────────────────────────────────────────────────────────── */
#define MEM_SIZE   0x10000u   /* 65536 palabras de 16 bits = 128 KB      */
#define NUM_REGS   16

/* ── Registros especiales ────────────────────────────────────────────── */
#define LR  14
#define SP  15

/* ── Tabla de opcodes ─────────────────────────────────────────────────
 *
 *  ALU (RD = RA op RB)
 *   0x00  NOP
 *   0x01  ADD   RD, RA, RB
 *   0x02  SUB   RD, RA, RB
 *   0x03  AND   RD, RA, RB
 *   0x04  OR    RD, RA, RB
 *   0x05  XOR   RD, RA, RB
 *   0x06  NOT   RD, RA          (RB ignorado)
 *   0x07  SHL   RD, RA, RB      (shift izq RB posiciones)
 *   0x08  SHR   RD, RA, RB      (shift der lógico)
 *   0x09  CMP   RA, RB          (RD ignorado, sólo actualiza flags)
 *   0x0A  MOV   RD, RA          (RD = RA)
 *
 *  Inmediato
 *   0x0B  ADDI  RD, RA, IMM3   (IMM de 3 bits, sign-extended)
 *   0x0C  LDI   RD, IMM16      (siguiente palabra = inmediato 16 bits)
 *
 *  Memoria
 *   0x0D  LOAD  RD, [RA]       (RD = mem[RA])
 *   0x0E  STORE [RD], RA       (mem[RD] = RA)
 *
 *  Saltos (la siguiente palabra contiene la dirección absoluta de 16 bits)
 *   0x10  JMP   addr
 *   0x11  JEQ   addr           (salta si Z=1)
 *   0x12  JNE   addr           (salta si Z=0)
 *   0x13  JLT   addr           (salta si N≠V)
 *   0x14  JGE   addr           (salta si N=V)
 *   0x15  JGT   addr           (salta si Z=0 y N=V)
 *   0x16  JLE   addr           (salta si Z=1 o N≠V)
 *
 *  Subrutinas / stack
 *   0x17  CALL  addr           (LR=PC+1, luego JMP addr)
 *   0x18  RET                  (PC = LR)
 *   0x19  PUSH  RA             (mem[--SP] = RA)
 *   0x1A  POP   RD             (RD = mem[SP++])
 *
 *  Sistema
 *   0x1F  HALT
 */

typedef enum {
    OP_NOP  = 0x00,
    OP_ADD  = 0x01, OP_SUB  = 0x02,
    OP_AND  = 0x03, OP_OR   = 0x04, OP_XOR  = 0x05,
    OP_NOT  = 0x06,
    OP_SHL  = 0x07, OP_SHR  = 0x08,
    OP_CMP  = 0x09, OP_MOV  = 0x0A,
    OP_ADDI = 0x0B, OP_LDI  = 0x0C,
    OP_LOAD = 0x0D, OP_STORE= 0x0E,
    OP_JMP  = 0x10,
    OP_JEQ  = 0x11, OP_JNE  = 0x12,
    OP_JLT  = 0x13, OP_JGE  = 0x14,
    OP_JGT  = 0x15, OP_JLE  = 0x16,
    OP_CALL = 0x17, OP_RET  = 0x18,
    OP_PUSH = 0x19, OP_POP  = 0x1A,
    OP_HALT = 0x1F
} Opcode;

/* ── Estructura del CPU ──────────────────────────────────────────────── */
typedef struct {
    uint16_t reg[NUM_REGS];
    uint16_t pc;
    uint16_t mem[MEM_SIZE];
    bool flag_z, flag_n, flag_c, flag_v;
    bool halted;
    uint64_t cycles;
} CPU;

/* ── Macros para decodificar campos ─────────────────────────────────── */
#define OPCODE(i)   (((i) >> 11) & 0x1F)
#define FIELD_RD(i) (((i) >>  7) & 0x0F)
#define FIELD_RA(i) (((i) >>  3) & 0x0F)
#define FIELD_RB(i) (((i) >>  0) & 0x07)
#define FIELD_I3(i) ((int16_t)(((i) & 0x07) << 13) >> 13)

/* ── Helpers ─────────────────────────────────────────────────────────── */
static void cpu_reset(CPU *cpu) {
    memset(cpu, 0, sizeof(*cpu));
    cpu->reg[SP] = MEM_SIZE - 1;
}

static void update_flags(CPU *cpu, uint32_t result, uint16_t a, uint16_t b, bool is_sub) {
    cpu->flag_z = ((result & 0xFFFF) == 0);
    cpu->flag_n = ((result >> 15) & 1);
    cpu->flag_c = (result > 0xFFFF);
    if (is_sub)
        cpu->flag_v = (((a ^ b) & 0x8000) && ((a ^ (uint16_t)result) & 0x8000));
    else
        cpu->flag_v = (!((a ^ b) & 0x8000) && ((a ^ (uint16_t)result) & 0x8000));
}

static void update_flags_logical(CPU *cpu, uint16_t result) {
    cpu->flag_z = (result == 0);
    cpu->flag_n = ((result >> 15) & 1);
    cpu->flag_c = false;
    cpu->flag_v = false;
}

static uint16_t fetch_word(CPU *cpu) {
    return cpu->mem[cpu->pc++];
}

/* ── Carga del programa ──────────────────────────────────────────────── */
static void cpu_load_program(CPU *cpu, const uint16_t *prog, size_t len) {
    for (size_t i = 0; i < len && i < MEM_SIZE; i++)
        cpu->mem[i] = prog[i];
}

/* ── Dump de estado ──────────────────────────────────────────────────── */
static void cpu_dump(const CPU *cpu) {
    printf("\n+------------------------------------------+\n");
    printf("|  CPU dump  PC=0x%04X  ciclos=%-10llu |\n",
           cpu->pc, (unsigned long long)cpu->cycles);
    printf("+------------------------------------------+\n");
    for (int i = 0; i < NUM_REGS; i++) {
        printf("|  R%-2d = 0x%04X (%6d)%s",
               i, cpu->reg[i], (int16_t)cpu->reg[i],
               (i % 2 == 1) ? "  |\n" : "   ");
    }
    printf("+------------------------------------------+\n");
    printf("|  FLAGS:  Z=%d  N=%d  C=%d  V=%d                  |\n",
           cpu->flag_z, cpu->flag_n, cpu->flag_c, cpu->flag_v);
    printf("|  SP=0x%04X  LR=0x%04X                    |\n",
           cpu->reg[SP], cpu->reg[LR]);
    printf("+------------------------------------------+\n\n");
}

/* ── Nucleo de ejecucion ─────────────────────────────────────────────── */
static void cpu_step(CPU *cpu) {
    if (cpu->halted) return;

    uint16_t instr = fetch_word(cpu);
    uint8_t  op    = OPCODE(instr);
    uint8_t  rd    = FIELD_RD(instr);
    uint8_t  ra    = FIELD_RA(instr);
    uint8_t  rb    = FIELD_RB(instr);
    int16_t  imm3  = FIELD_I3(instr);

    uint32_t result;
    uint16_t addr;
    bool     jump;

    cpu->cycles++;

    switch ((Opcode)op) {
    case OP_NOP: break;

    case OP_ADD:
        result = (uint32_t)cpu->reg[ra] + cpu->reg[rb];
        update_flags(cpu, result, cpu->reg[ra], cpu->reg[rb], false);
        cpu->reg[rd] = (uint16_t)result;
        break;

    case OP_SUB:
        result = (uint32_t)cpu->reg[ra] - cpu->reg[rb];
        update_flags(cpu, result, cpu->reg[ra], cpu->reg[rb], true);
        cpu->reg[rd] = (uint16_t)result;
        break;

    case OP_AND:
        cpu->reg[rd] = cpu->reg[ra] & cpu->reg[rb];
        update_flags_logical(cpu, cpu->reg[rd]);
        break;

    case OP_OR:
        cpu->reg[rd] = cpu->reg[ra] | cpu->reg[rb];
        update_flags_logical(cpu, cpu->reg[rd]);
        break;

    case OP_XOR:
        cpu->reg[rd] = cpu->reg[ra] ^ cpu->reg[rb];
        update_flags_logical(cpu, cpu->reg[rd]);
        break;

    case OP_NOT:
        cpu->reg[rd] = ~cpu->reg[ra];
        update_flags_logical(cpu, cpu->reg[rd]);
        break;

    case OP_SHL:
        cpu->reg[rd] = cpu->reg[ra] << (cpu->reg[rb] & 0xF);
        update_flags_logical(cpu, cpu->reg[rd]);
        break;

    case OP_SHR:
        cpu->reg[rd] = cpu->reg[ra] >> (cpu->reg[rb] & 0xF);
        update_flags_logical(cpu, cpu->reg[rd]);
        break;

    case OP_CMP:
        result = (uint32_t)cpu->reg[ra] - cpu->reg[rb];
        update_flags(cpu, result, cpu->reg[ra], cpu->reg[rb], true);
        break;

    case OP_MOV:
        cpu->reg[rd] = cpu->reg[ra];
        break;

    case OP_ADDI:
        result = (uint32_t)cpu->reg[ra] + (uint16_t)imm3;
        update_flags(cpu, result, cpu->reg[ra], (uint16_t)imm3, false);
        cpu->reg[rd] = (uint16_t)result;
        break;

    case OP_LDI:
        cpu->reg[rd] = fetch_word(cpu);
        break;

    case OP_LOAD:
        cpu->reg[rd] = cpu->mem[cpu->reg[ra]];
        break;

    case OP_STORE:
        cpu->mem[cpu->reg[rd]] = cpu->reg[ra];
        break;

    case OP_JMP:
        cpu->pc = fetch_word(cpu);
        break;

    case OP_JEQ:
        addr = fetch_word(cpu);
        if (cpu->flag_z) cpu->pc = addr;
        break;

    case OP_JNE:
        addr = fetch_word(cpu);
        if (!cpu->flag_z) cpu->pc = addr;
        break;

    case OP_JLT:
        addr = fetch_word(cpu);
        if (cpu->flag_n != cpu->flag_v) cpu->pc = addr;
        break;

    case OP_JGE:
        addr = fetch_word(cpu);
        if (cpu->flag_n == cpu->flag_v) cpu->pc = addr;
        break;

    case OP_JGT:
        addr = fetch_word(cpu);
        jump = !cpu->flag_z && (cpu->flag_n == cpu->flag_v);
        if (jump) cpu->pc = addr;
        break;

    case OP_JLE:
        addr = fetch_word(cpu);
        jump = cpu->flag_z || (cpu->flag_n != cpu->flag_v);
        if (jump) cpu->pc = addr;
        break;

    case OP_CALL:
        addr = fetch_word(cpu);
        cpu->reg[LR] = cpu->pc;
        cpu->pc = addr;
        break;

    case OP_RET:
        cpu->pc = cpu->reg[LR];
        break;

    case OP_PUSH:
        cpu->reg[SP]--;
        cpu->mem[cpu->reg[SP]] = cpu->reg[ra];
        break;

    case OP_POP:
        cpu->reg[rd] = cpu->mem[cpu->reg[SP]];
        cpu->reg[SP]++;
        break;

    case OP_HALT:
        cpu->halted = true;
        printf("[CPU] HALT en PC=0x%04X  ciclos totales: %llu\n",
               cpu->pc - 1, (unsigned long long)cpu->cycles);
        break;

    default:
        fprintf(stderr, "[CPU] Opcode desconocido 0x%02X en PC=0x%04X\n",
                op, cpu->pc - 1);
        cpu->halted = true;
        break;
    }

    /* Suprimir warnings de rb no usado en algunas rutas */
    (void)rb;
}

static void cpu_run(CPU *cpu, uint64_t maxcycles) {
    while (!cpu->halted && cpu->cycles < maxcycles)
        cpu_step(cpu);
    if (!cpu->halted)
        printf("[CPU] Limite de %llu ciclos alcanzado.\n",
               (unsigned long long)maxcycles);
}

/* ── Macros de ensamblado ─────────────────────────────────────────────
 *
 * INSTR genera una sola palabra de 16 bits.
 * Las instrucciones marcadas con "2 palabras" emiten el opcode MAS
 * el inmediato/dirección como segunda entrada en el array.
 */
#define INSTR(op,rd,ra,misc) \
    ((uint16_t)(((op)&0x1F)<<11|((rd)&0xF)<<7|((ra)&0xF)<<3|((misc)&0x7)))

/* 2 palabras: emiten INSTR + IMM como dos elementos del array literal */
#define I_LDI(rd,imm)   INSTR(OP_LDI, rd,0,0),(uint16_t)(imm)
#define I_JMP(addr)     INSTR(OP_JMP,  0,0,0),(uint16_t)(addr)
#define I_JEQ(addr)     INSTR(OP_JEQ,  0,0,0),(uint16_t)(addr)
#define I_JNE(addr)     INSTR(OP_JNE,  0,0,0),(uint16_t)(addr)
#define I_JLT(addr)     INSTR(OP_JLT,  0,0,0),(uint16_t)(addr)
#define I_JGE(addr)     INSTR(OP_JGE,  0,0,0),(uint16_t)(addr)
#define I_JGT(addr)     INSTR(OP_JGT,  0,0,0),(uint16_t)(addr)
#define I_JLE(addr)     INSTR(OP_JLE,  0,0,0),(uint16_t)(addr)
#define I_CALL(addr)    INSTR(OP_CALL,  0,0,0),(uint16_t)(addr)

/* 1 palabra */
#define I_NOP()         INSTR(OP_NOP,  0, 0,0)
#define I_HALT()        INSTR(OP_HALT, 0, 0,0)
#define I_ADD(d,a,b)    INSTR(OP_ADD,  d, a,b)
#define I_SUB(d,a,b)    INSTR(OP_SUB,  d, a,b)
#define I_AND(d,a,b)    INSTR(OP_AND,  d, a,b)
#define I_OR(d,a,b)     INSTR(OP_OR,   d, a,b)
#define I_XOR(d,a,b)    INSTR(OP_XOR,  d, a,b)
#define I_NOT(d,a)      INSTR(OP_NOT,  d, a,0)
#define I_SHL(d,a,b)    INSTR(OP_SHL,  d, a,b)
#define I_SHR(d,a,b)    INSTR(OP_SHR,  d, a,b)
#define I_CMP(a,b)      INSTR(OP_CMP,  0, a,b)
#define I_MOV(d,a)      INSTR(OP_MOV,  d, a,0)
#define I_ADDI(d,a,i)   INSTR(OP_ADDI, d, a,(i)&0x7)
#define I_LOAD(d,a)     INSTR(OP_LOAD, d, a,0)
#define I_STORE(d,a)    INSTR(OP_STORE,d, a,0)
#define I_RET()         INSTR(OP_RET,  0, 0,0)
#define I_PUSH(a)       INSTR(OP_PUSH, 0, a,0)
#define I_POP(d)        INSTR(OP_POP,  d, 0,0)

/* ══════════════════════════════════════════════════════════════════════
 * PROGRAMAS DE PRUEBA
 *
 * Regla de offsets: cada instrucción de 1 palabra ocupa 1 posición,
 * cada instrucción de 2 palabras (LDI, JMP, JEQ, etc.) ocupa 2 posiciones.
 * Los comentarios con @ indican el índice en el array (= dirección en mem).
 * ══════════════════════════════════════════════════════════════════════ */

/*
 * Programa 1: Fibonacci iterativo (N=10)
 * Guarda los primeros 10 términos en mem[0x100..0x109]
 *
 *  R0 = N=10   R1=fib_a=0  R2=fib_b=1
 *  R3 = tmp    R4=ptr      R5=contador
 *
 * Mapa de palabras:
 *  @0-1  : LDI R0,10
 *  @2-3  : LDI R1,0
 *  @4-5  : LDI R2,1
 *  @6-7  : LDI R4,0x100
 *  @8-9  : LDI R5,0
 *  @10   : CMP R5,R0          ← loop
 *  @11-12: JEQ 22             → end
 *  @13   : STORE [R4],R1
 *  @14   : ADDI R4,R4,1
 *  @15   : ADD R3,R1,R2
 *  @16   : MOV R1,R2
 *  @17   : MOV R2,R3
 *  @18   : ADDI R5,R5,1
 *  @19-20: JMP 10             → loop
 *  @21   : HALT               ← end (JEQ apunta aquí)
 *
 * Verificación: 21 es el índice del HALT pero JEQ va a 21.
 * Reconteo: @19 es JMP (2 palabras: @19 opcode, @20 dirección).
 * Siguiente libre es @21 → HALT en @21 → JEQ debe apuntar a 21. OK.
 */
static const uint16_t prog_fib[] = {
    /* @0  */ I_LDI(0, 10),
    /* @2  */ I_LDI(1, 0),
    /* @4  */ I_LDI(2, 1),
    /* @6  */ I_LDI(4, 0x100),
    /* @8  */ I_LDI(5, 0),
    /* @10 */ I_CMP(5, 0),
    /* @11 */ I_JEQ(21),
    /* @13 */ I_STORE(4, 1),
    /* @14 */ I_ADDI(4, 4, 1),
    /* @15 */ I_ADD(3, 1, 2),
    /* @16 */ I_MOV(1, 2),
    /* @17 */ I_MOV(2, 3),
    /* @18 */ I_ADDI(5, 5, 1),
    /* @19 */ I_JMP(10),
    /* @21 */ I_HALT(),
};

/*
 * Programa 2: CALL/RET — subrutina suma
 * Calcula 25 + 17, resultado en R0
 *
 *  @0-1 : LDI R0, 25
 *  @2-3 : LDI R1, 17
 *  @4-5 : CALL 8          → add_sub @ 8
 *  @6   : HALT
 *  --- add_sub @ 7 ---
 *  @7   : ADD R0,R0,R1
 *  @8   : RET
 *
 * Reconteo: CALL es 2 palabras @4-5. HALT en @6. add_sub en @7.
 * CALL debe apuntar a 7.
 */
static const uint16_t prog_call[] = {
    /* @0 */ I_LDI(0, 25),
    /* @2 */ I_LDI(1, 17),
    /* @4 */ I_CALL(7),
    /* @6 */ I_HALT(),
    /* add_sub @ 7 */
    /* @7 */ I_ADD(0, 0, 1),
    /* @8 */ I_RET(),
};

/*
 * Programa 3: PUSH/POP — swap de dos valores via stack
 * R0=10, R1=20 → después del swap: R0=20, R1=10
 *
 *  @0-1 : LDI R0,10
 *  @2-3 : LDI R1,20
 *  @4   : PUSH R0
 *  @5   : PUSH R1
 *  @6   : POP  R0
 *  @7   : POP  R1
 *  @8   : HALT
 */
static const uint16_t prog_stack[] = {
    /* @0 */ I_LDI(0, 10),
    /* @2 */ I_LDI(1, 20),
    /* @4 */ I_PUSH(0),
    /* @5 */ I_PUSH(1),
    /* @6 */ I_POP(0),
    /* @7 */ I_POP(1),
    /* @8 */ I_HALT(),
};

/* ── Main ─────────────────────────────────────────────────────────────── */
int main(void) {
    CPU cpu;

    /* ── Test 1: Fibonacci ── */
    printf("========================================\n");
    printf("  Test 1: Fibonacci iterativo (N=10)\n");
    printf("========================================\n");
    cpu_reset(&cpu);
    cpu_load_program(&cpu, prog_fib, sizeof(prog_fib)/sizeof(prog_fib[0]));
    cpu_run(&cpu, 1000000);
    cpu_dump(&cpu);
    printf("Secuencia en mem[0x100..0x109]:\n  ");
    for (int i = 0; i < 10; i++)
        printf("%u ", cpu.mem[0x100 + i]);
    printf("\n  (esperado: 0 1 1 2 3 5 8 13 21 34)\n\n");

    /* ── Test 2: CALL/RET ── */
    printf("========================================\n");
    printf("  Test 2: CALL/RET  (25 + 17 = 42?)\n");
    printf("========================================\n");
    cpu_reset(&cpu);
    cpu_load_program(&cpu, prog_call, sizeof(prog_call)/sizeof(prog_call[0]));
    cpu_run(&cpu, 1000000);
    cpu_dump(&cpu);
    printf("R0 = %u  (esperado: 42)\n\n", cpu.reg[0]);

    /* ── Test 3: PUSH/POP swap ── */
    printf("========================================\n");
    printf("  Test 3: PUSH/POP swap (10 <-> 20)\n");
    printf("========================================\n");
    cpu_reset(&cpu);
    cpu_load_program(&cpu, prog_stack, sizeof(prog_stack)/sizeof(prog_stack[0]));
    cpu_run(&cpu, 1000000);
    cpu_dump(&cpu);
    printf("R0 = %u  (esperado: 20)\n", cpu.reg[0]);
    printf("R1 = %u  (esperado: 10)\n\n", cpu.reg[1]);

    return 0;
}
