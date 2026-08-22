#include <memory>
#include <iostream>
#include <cstdint>
 
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vimm_gen.h"
using VDut = Vimm_gen;

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
 
static void gen(VDut* dut, uint32_t instr, uint8_t imm_type) {
    dut->instr_i = instr;
    dut->imm_type_i = imm_type;
    dut->eval();
#if VM_TRACE
    if (g_tfp) {
        g_tfp->dump(g_sim_time);
        g_sim_time++;
   }
#endif
}

static uint32_t make_i_type(uint16_t imm12) {
    return (uint32_t(imm12 & 0xFFF) << 20);
}

static uint32_t make_s_type(uint16_t imm12) {
    uint32_t hi = (imm12 >> 5) & 0x7F;
    uint32_t lo = imm12 & 0x1F;
    return (hi << 25) | (lo << 7);
}

static uint32_t make_b_type(int32_t imm13) {
    uint32_t u = (uint32_t)imm13;
    uint32_t b12    = (u >> 12) & 0x1;
    uint32_t b11    = (u >> 11) & 0x1;
    uint32_t b10_5  = (u >> 5)  & 0x3F;
    uint32_t b4_1   = (u >> 1)  & 0xF;
    return (b12 << 31) | (b10_5 << 25) | (b4_1 << 8) | (b11 << 7);
}

static uint32_t make_u_type(uint32_t imm20_shifted) {
    return imm20_shifted & 0xFFFFF000u;
}

static uint32_t make_j_type(int32_t imm21) {
    uint32_t u = (uint32_t)imm21;
    uint32_t b20    = (u >> 20) & 0x1;
    uint32_t b19_12 = (u >> 12) & 0xFF;
    uint32_t b11    = (u >> 11) & 0x1;
    uint32_t b10_1  = (u >> 1)  & 0x3FF;
    return (b20 << 31) | (b19_12 << 12) | (b11 << 20) | (b10_1 << 21);
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
    std::cout << "\n=== Tests: imm_gen ===\n\n";

    gen(dut.get(), make_i_type(5), IMM_I);
    CHECK(dut->imm_o, 5, "IMM_I: inmediato positivo pequeño (5)");
 
    gen(dut.get(), make_i_type(0xFFF /* -1 en 12 bits */), IMM_I);
    CHECK(dut->imm_o, 0xFFFFFFFFu, "IMM_I: -1 sign-extended a 0xFFFFFFFF");
 
    gen(dut.get(), make_i_type(0x800 /* -2048, el más negativo en 12 bits */), IMM_I);
    CHECK(dut->imm_o, 0xFFFFF800u, "IMM_I: valor mínimo (-2048) sign-extended");

    gen(dut.get(), make_s_type(10), IMM_S);
    CHECK(dut->imm_o, 10, "IMM_S: inmediato positivo (10)");
 
    gen(dut.get(), make_s_type((uint16_t)(-1 & 0xFFF)), IMM_S);
    CHECK(dut->imm_o, 0xFFFFFFFFu, "IMM_S: -1 sign-extended");

    gen(dut.get(), make_b_type(16), IMM_B);
    CHECK(dut->imm_o, 16, "IMM_B: offset positivo (+16)");
 
    gen(dut.get(), make_b_type(-16), IMM_B);
    CHECK(dut->imm_o, 0xFFFFFFF0u, "IMM_B: offset negativo (-16) sign-extended");
 
    gen(dut.get(), make_b_type(0), IMM_B);
    CHECK(dut->imm_o, 0, "IMM_B: offset cero");

    gen(dut.get(), make_u_type(0x12345000u), IMM_U);
    CHECK(dut->imm_o, 0x12345000u, "IMM_U: imm[31:12] tal cual, bits bajos a 0");
 
    gen(dut.get(), make_u_type(0xFFFFF000u), IMM_U);
    CHECK(dut->imm_o, 0xFFFFF000u, "IMM_U: valor con bit 31 activo (no se sign-extiende más)");

    gen(dut.get(), make_j_type(100), IMM_J);
    CHECK(dut->imm_o, 100, "IMM_J: offset positivo (+100)");
 
    gen(dut.get(), make_j_type(-100), IMM_J);
    CHECK(dut->imm_o, 0xFFFFFF9Cu, "IMM_J: offset negativo (-100) sign-extended");
 
    gen(dut.get(), make_j_type(0), IMM_J);
    CHECK(dut->imm_o, 0, "IMM_J: offset cero");

    std::cout << "\n=== Resumen: " << (g_tests_run - g_tests_failed) << "/" << g_tests_run << " tests OK ===\n";
 
#if VM_TRACE
        tfp->close();
#endif
    dut->final();
    return (g_tests_failed > 0) ? 1 : 0;
}
