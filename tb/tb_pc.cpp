#include <memory>
#include <iostream>
#include <cstdint>
 
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vpc.h"
using VDut = Vpc;

#include "riscv_pkg.h"
using namespace riscv;

#if VM_TRACE
static VerilatedVcdC* g_tfp      = nullptr;
#endif

static vluint64_t     g_sim_time = 0;

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

static void tick_half_cycle(VDut* dut) {
    dut->clk_i = !dut->clk_i;
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
 
    dut->clk_i       = 0;
    dut->rst_n     = 0;
    dut->next_pc_i = 0;

    std::cout << "\n=== Tests: pc ===\n\n";

    dut->next_pc_i = 0xDEADBEEFu;
    tick_cycle(dut.get());
    CHECK(dut->pc_o, 0x00000000u, "Reset: pc_o = RESET_ADDR (0x0) con rst_n=0");

    dut->rst_n     = 1;
    dut->next_pc_i = 0x00000004u;
    tick_cycle(dut.get());
    CHECK(dut->pc_o, 0x00000004u, "Tras liberar reset: pc_o adopta next_pc_i (0x4)");

    dut->next_pc_i = 0x00000008u;
    tick_cycle(dut.get());
    CHECK(dut->pc_o, 0x00000008u, "PC+4: 0x4 -> 0x8");
 
    dut->next_pc_i = 0x0000000Cu;
    tick_cycle(dut.get());
    CHECK(dut->pc_o, 0x0000000Cu, "PC+4: 0x8 -> 0xC");

    dut->next_pc_i = 0x00001000u;
    tick_cycle(dut.get());
    CHECK(dut->pc_o, 0x00001000u, "Salto: pc_o adopta un target arbitrario (0x1000)");

    dut->rst_n = 0;
    dut->eval();
#if VM_TRACE
    if (g_tfp) { g_tfp->dump(g_sim_time); g_sim_time++; }
#endif
    CHECK(dut->pc_o, 0x00000000u, "Reset asíncrono: pc_o vuelve a 0x0 sin esperar flanco de clk");

    std::cout << "\n=== Resumen: " << (g_tests_run - g_tests_failed)
              << "/" << g_tests_run << " tests OK ===\n";
 
#if VM_TRACE
    tfp->close();
#endif
    dut->final();
    return (g_tests_failed > 0) ? 1 : 0;
}
