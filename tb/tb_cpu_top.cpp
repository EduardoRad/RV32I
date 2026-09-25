#include <memory>
#include <iostream>
#include <cstdint>
 
#include <verilated.h>
#include "Vcpu_top.h"
#include "Vcpu_top__Syms.h"
#include "riscv_pkg.h"
#if VM_TRACE
#include <verilated_vcd_c.h>
#endif
 
using VDut = Vcpu_top;
using namespace riscv;
 
#if VM_TRACE
static VerilatedVcdC* g_tfp = nullptr;
#endif
static vluint64_t g_sim_time = 0;
 
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
                      << " | esperado=0x" << std::hex << (uint64_t)_exp     \
                      << " obtenido=0x" << std::hex << (uint64_t)_act       \
                      << std::dec << "\n";                                  \
        } else {                                                            \
            std::cout << "[PASS] " << (description) << "\n";               \
        }                                                                    \
    } while (0)

static void tick_half_cycle(VDut* dut) {
    dut->clk = !dut->clk;
    dut->eval();
#if VM_TRACE
    if (g_tfp) g_tfp->dump(g_sim_time);
#endif
    g_sim_time++;
}
static void tick_cycle(VDut* dut) {
    tick_half_cycle(dut);
    tick_half_cycle(dut);
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
 
    dut->clk   = 0;
    dut->rst_n = 0;

    for (int i = 0; i < 4; i++) tick_cycle(dut.get());
    dut->rst_n = 1;
 
    std::cout << "\n=== Tests: cpu_top ===\n\n";

    const int N_CYCLES = 12;
    for (int i = 0; i < N_CYCLES; i++) tick_cycle(dut.get());

    auto& root = *dut->rootp;
 
    CHECK(root.cpu_top__DOT__u_reg_file__DOT__regs[1], 5,
          "x1 = 5 (addi x1, x0, 5)");
    CHECK(root.cpu_top__DOT__u_reg_file__DOT__regs[2], 10,
          "x2 = 10 (addi x2, x0, 10)");
    CHECK(root.cpu_top__DOT__u_reg_file__DOT__regs[3], 15,
          "x3 = 15 (add x3, x1, x2)");
    CHECK(root.cpu_top__DOT__u_reg_file__DOT__regs[4], 15,
          "x4 = 15 (lw x4, 0(x0), tras el sw anterior)");
    CHECK(root.cpu_top__DOT__u_reg_file__DOT__regs[5], 0,
          "x5 = 0: NUNCA se ejecuta addi x5,x0,99 -> confirma que el branch saltó de verdad");
    CHECK(root.cpu_top__DOT__u_reg_file__DOT__regs[6], 1,
          "x6 = 1: se alcanzó el destino del branch (addr 28)");

    auto& dmem = root.cpu_top__DOT__u_dmem__DOT__mem;
    uint32_t stored_word = (uint32_t)dmem[0]        |
                            ((uint32_t)dmem[1] << 8)  |
                            ((uint32_t)dmem[2] << 16) |
                            ((uint32_t)dmem[3] << 24);
    CHECK(stored_word, 15, "data_mem[0..3]: 15 guardado correctamente por el sw (little-endian)");

    std::cout << "\n=== Resumen: " << (g_tests_run - g_tests_failed)
              << "/" << g_tests_run << " tests OK ===\n";
 
#if VM_TRACE
    tfp->close();
#endif
    dut->final();
    return (g_tests_failed > 0) ? 1 : 0;
}
