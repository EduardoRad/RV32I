#include <memory>
#include <iostream>
#include <cstdint>
 
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Valu.h"
using VDut = Valu;

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
 
static void run_alu(VDut* dut, uint32_t a, uint32_t b, uint8_t op) {
    dut->operand_a_i = a;
    dut->operand_b_i = b;
    dut->alu_op      = op;
    dut->eval();
#if VM_TRACE
    if (g_tfp) {
        g_tfp->dump(g_sim_time);
        g_sim_time++;
   }
#endif
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
    std::cout << "\n=== Tests: alu ===\n\n";

    run_alu(dut.get(), 10, 5, ALU_ADD);
    CHECK(dut->result_o, 15, "ADD: 10 + 5 = 15");

    run_alu(dut.get(), 10, 5, ALU_SUB);
    CHECK(dut->result_o, 5, "SUB: 10 - 5 = 5");

    run_alu(dut.get(), 5, 10, ALU_SUB);
    CHECK(dut->result_o, 0xFFFFFFFBu, "SUB: 5 - 10 = -5 (0xFFFFFFFB)");
 
    run_alu(dut.get(), 0xF0F0F0F0u, 0x0F0F0F0Fu, ALU_AND);
    CHECK(dut->result_o, 0x00000000u, "AND: 0xF0F0F0F0 & 0x0F0F0F0F = 0");

    run_alu(dut.get(), 0xF0F0F0F0u, 0x0F0F0F0Fu, ALU_OR);
    CHECK(dut->result_o, 0xFFFFFFFFu, "OR: 0xF0F0F0F0 | 0x0F0F0F0F = 0xFFFFFFFF");
 
    run_alu(dut.get(), 0xFFFFFFFFu, 0x0F0F0F0Fu, ALU_XOR);
    CHECK(dut->result_o, 0xF0F0F0F0u, "XOR: 0xFFFFFFFF ^ 0x0F0F0F0F = 0xF0F0F0F0");

    run_alu(dut.get(), 1, 4, ALU_SLL);
    CHECK(dut->result_o, 16, "SLL: 1 << 4 = 16");
 
    run_alu(dut.get(), 0x80000000u, 4, ALU_SRL);
    CHECK(dut->result_o, 0x08000000u, "SRL: 0x80000000 >> 4 = 0x08000000 (rellena con 0)");
 
    run_alu(dut.get(), 0x80000000u, 4, ALU_SRA);
    CHECK(dut->result_o, 0xF8000000u, "SRA: 0x80000000 >>> 4 = 0xF8000000 (rellena con signo)");
 
    run_alu(dut.get(), 1, 31, ALU_SLL);
    CHECK(dut->result_o, 0x80000000u, "SLL: shift máximo (31) no se desborda");

    run_alu(dut.get(), 1, 0xFFFFFFE4u /* ...100100 -> shamt=4 */, ALU_SLL);
    CHECK(dut->result_o, 16, "SLL: solo los 5 bits bajos de operand_b cuentan como shamt");

    run_alu(dut.get(), (uint32_t)-5, 3, ALU_SLT);
    CHECK(dut->result_o, 1, "SLT: -5 < 3 (con signo) = 1");
 
    run_alu(dut.get(), (uint32_t)-5, 3, ALU_SLTU);
    CHECK(dut->result_o, 0, "SLTU: 0xFFFFFFFB < 3 (sin signo) = 0 (es un número enorme)");
 
    run_alu(dut.get(), 3, 10, ALU_SLT);
    CHECK(dut->result_o, 1, "SLT: 3 < 10 = 1");
 
    run_alu(dut.get(), 10, 3, ALU_SLT);
    CHECK(dut->result_o, 0, "SLT: 10 < 3 = 0");

    run_alu(dut.get(), 42, 42, ALU_ADD); // alu_op irrelevante para el flag
    CHECK(dut->zero, 1, "zero: a == b -> zero = 1 (con alu_op=ADD, no SUB)");
 
    run_alu(dut.get(), 42, 43, ALU_ADD);
    CHECK(dut->zero, 0, "zero: a != b -> zero = 0");
 
    run_alu(dut.get(), (uint32_t)-1, 0, ALU_ADD); // -1 < 0 con signo
    CHECK(dut->less, 1, "less: -1 < 0 (con signo) = 1");
 
    run_alu(dut.get(), (uint32_t)-1, 0, ALU_ADD); // 0xFFFFFFFF > 0 sin signo
    CHECK(dut->less_u, 0, "less_u: 0xFFFFFFFF < 0 (sin signo) = 0");
 
    run_alu(dut.get(), 5, 10, ALU_XOR); // alu_op irrelevante, flags en paralelo
    CHECK(dut->less,   1, "less: 5 < 10, independiente de alu_op=XOR");
    CHECK(dut->less_u, 1, "less_u: 5 < 10, independiente de alu_op=XOR");

    std::cout << "\n=== Resumen: " << (g_tests_run - g_tests_failed) << "/" << g_tests_run << " tests OK ===\n";
 
#if VM_TRACE
        tfp->close();
#endif
    dut->final();
    return (g_tests_failed > 0) ? 1 : 0;
}
