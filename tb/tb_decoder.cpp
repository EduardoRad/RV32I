#include <memory>
#include <iostream>
#include <cstdint>
 
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vdecoder.h"
using VDut = Vdecoder;

#include "riscv_pkg.h"
using namespace riscv;

#if VM_TRACE
static VerilatedVcdC* g_tfp      = nullptr;
static vluint64_t     g_sim_time = 0;
#endif

static int g_tests_run    = 0;
static int g_tests_failed = 0;
 
#define CHECK(actual, expected, description)                                 \
    do {                                                                     \
        g_tests_run++;                                                      \
        auto _act = (actual);                                               \
        auto _exp = (expected);                                             \
        if ((uint64_t)_act != (uint64_t)_exp) {                             \
            g_tests_failed++;                                               \
            std::cout << "[FAIL] " << (description)                        \
                      << " | esperado=" << (uint64_t)_exp                  \
                      << " obtenido=" << (uint64_t)_act << "\n";            \
        } else {                                                            \
            std::cout << "[PASS] " << (description) << "\n";               \
        }                                                                    \
    } while (0)
 
static void decode(VDut* dut, uint32_t instr) {
    dut->instr_i = instr;
    dut->eval();
#if VM_TRACE
    if (g_tfp) {
        g_tfp->dump(g_sim_time);
        g_sim_time++;
    }
#endif
}

static uint32_t make_r_type(uint8_t opcode, uint8_t rd, uint8_t funct3, uint8_t rs1, uint8_t rs2, uint8_t funct7) {
    return (uint32_t(funct7 & 0x7F) << 25) | (uint32_t(rs2 & 0x1F) << 20) | (uint32_t(rs1 & 0x1F) << 15) |
        (uint32_t(funct3 & 0x7) << 12) | (uint32_t(rd & 0x1F) << 7) | (opcode & 0x7F);
}

static uint32_t make_i_type(uint8_t opcode, uint8_t rd, uint8_t funct3, uint8_t rs1, uint16_t imm12) {
    return (uint32_t(imm12 & 0xFFF) << 20) | (uint32_t(rs1 & 0x1F) << 15) | (uint32_t(funct3 & 0x7) << 12) |
      (uint32_t(rd & 0x1F) << 7) | (opcode & 0x7F);
}
 
int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    std::unique_ptr<VDut> dut = std::make_unique<VDut>();
 #if VM_TRACE
    Verilated::traceEverOn(true);
    std::unique_ptr<VerilatedVcdC> tfp = std::make_unique<VerilatedVcdC>();
    dut->trace(tfp.get(), 99);
    tfp->open("wave.vcd");
    g_tfp = tfp.get();
#endif
    std::cout << "\n=== Tests: decoder ===\n\n";

    // ===================================================================
    // Tipo R: ADD x5, x6, x7  (opcode=OP_REG, funct3=000, funct7=0000000)
    // rd=5, rs1=6, rs2=7
    // ===================================================================
    {
        uint32_t instr = make_r_type(OP_REG, 5, F3_ADD_SUB, 6, 7, F7_ADD_SRL);
        decode(dut.get(), instr);
        CHECK(dut->opcode_o, OP_REG,     "ADD: opcode = OP_REG");
        CHECK(dut->rd,       5,          "ADD: rd = 5");
        CHECK(dut->funct3,   F3_ADD_SUB, "ADD: funct3 = F3_ADD_SUB");
        CHECK(dut->rs1,      6,          "ADD: rs1 = 6");
        CHECK(dut->rs2,      7,          "ADD: rs2 = 7");
        CHECK(dut->funct7,   F7_ADD_SRL, "ADD: funct7 = F7_ADD_SRL");
    }
 
    // SUB x10, x11, x12 (mismo funct3 que ADD, distinto funct7)
    {
        uint32_t instr = make_r_type(OP_REG, 10, F3_ADD_SUB, 11, 12, F7_SUB_SRA);
        decode(dut.get(), instr);
        CHECK(dut->rd,     10,         "SUB: rd = 10");
        CHECK(dut->rs1,    11,         "SUB: rs1 = 11");
        CHECK(dut->rs2,    12,         "SUB: rs2 = 12");
        CHECK(dut->funct7, F7_SUB_SRA, "SUB: funct7 = F7_SUB_SRA (distingue de ADD)");
    }
 
    // ===================================================================
    // Tipo I: ADDI x1, x2, 5  (opcode=OP_IMM, funct3=000)
    // Aquí rs2/funct7 en realidad forman parte del inmediato — el decoder
    // los extrae igualmente porque no sabe (ni le importa) el formato;
    // eso lo decide luego imm_gen a partir de imm_type.
    // ===================================================================
    {
        uint32_t instr = make_i_type(OP_IMM, 1, F3_ADD_SUB, 2, 5);
        decode(dut.get(), instr);
        CHECK(dut->opcode_o, OP_IMM,     "ADDI: opcode = OP_IMM");
        CHECK(dut->rd,       1,          "ADDI: rd = 1");
        CHECK(dut->funct3,   F3_ADD_SUB, "ADDI: funct3 = F3_ADD_SUB");
        CHECK(dut->rs1,      2,          "ADDI: rs1 = 2");
    }
 
    // ===================================================================
    // rd = x0 y rs1 = x0: verifica que el decoder extrae el valor tal
    // cual (0), sin ningún tipo de lógica especial — esa lógica vive en
    // reg_file, no aquí.
    // ===================================================================
    {
        uint32_t instr = make_r_type(OP_REG, 0, F3_AND, 0, 31, F7_ADD_SRL);
        decode(dut.get(), instr);
        CHECK(dut->rd,  0,  "rd=x0: se extrae 0 sin tratamiento especial");
        CHECK(dut->rs1, 0,  "rs1=x0: se extrae 0 sin tratamiento especial");
        CHECK(dut->rs2, 31, "rs2=31 (x31): se extrae el máximo índice válido");
    }
 
    // ===================================================================
    // Todos los campos a 1 (instrucción "todo unos" salvo opcode): valida
    // que no hay solapamiento entre campos ni bits mal recortados en los
    // límites de cada rango.
    // ===================================================================
    {
        uint32_t instr = 0xFFFFFFFF & ~0x7Fu; // todo 1s, opcode a 0 para no
                                               // depender de qué opcode_e
                                               // exista en ese valor
        instr |= OP_REG; // opcode válido conocido
        decode(dut.get(), instr);
        CHECK(dut->rd,     0x1F, "Todo unos: rd = 0x1F (5 bits)");
        CHECK(dut->funct3, 0x7,  "Todo unos: funct3 = 0x7 (3 bits)");
        CHECK(dut->rs1,    0x1F, "Todo unos: rs1 = 0x1F (5 bits)");
        CHECK(dut->rs2,    0x1F, "Todo unos: rs2 = 0x1F (5 bits)");
        CHECK(dut->funct7, 0x7F, "Todo unos: funct7 = 0x7F (7 bits)");
    }

    std::cout << "\n=== Resumen: " << (g_tests_run - g_tests_failed)
              << "/" << g_tests_run << " tests OK ===\n";
 
#if VM_TRACE
        tfp->close();
#endif
    dut->final();
    return (g_tests_failed > 0) ? 1 : 0;
}
