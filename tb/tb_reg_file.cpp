#include <memory>
#include <iostream>
#include <cstdint>
 
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vreg_file.h"
using VDut = Vreg_file;

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

 static void read_regs(VDut* dut, uint8_t rs1, uint8_t rs2) {
    dut->rs1_addr = rs1;
    dut->rs2_addr = rs2;
    dut->eval();
}

static void write_reg(VDut* dut, uint8_t rd, uint32_t data) {
    dut->w       = 1;
    dut->rd_addr = rd;
    dut->rd_data = data;
    tick_cycle(dut);
    dut->w = 0;
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

    dut->clk      = 0;
    dut->rst_n    = 0;
    dut->w        = 0;
    dut->rd_addr  = 0;
    dut->rd_data  = 0;
    dut->rs1_addr = 0;
    dut->rs2_addr = 0;

    for (int i = 0; i < 4; i++) tick_cycle(dut.get());
    dut->rst_n = 1;
    tick_cycle(dut.get());

    std::cout << "\n=== Tests: reg_file ===\n\n";

    read_regs(dut.get(), 5, 10);
    CHECK(dut->rs1_data, 0, "Reset: x5 lee 0");
    CHECK(dut->rs2_data, 0, "Reset: x10 lee 0");

    write_reg(dut.get(), 0, 0xDEADBEEF);
    read_regs(dut.get(), 0, 0);
    CHECK(dut->rs1_data, 0, "x0: sigue leyendo 0 tras intentar escribir 0xDEADBEEF");
    CHECK(dut->rs2_data, 0, "x0: mismo resultado en el segundo puerto de lectura");

    write_reg(dut.get(), 3, 0x12345678);
    read_regs(dut.get(), 3, 3);
    CHECK(dut->rs1_data, 0x12345678, "x3: lee lo escrito, un ciclo después (rs1)");
    CHECK(dut->rs2_data, 0x12345678, "x3: lee lo escrito, un ciclo después (rs2)");

    dut->w        = 1;
    dut->rd_addr  = 7;
    dut->rd_data  = 0xCAFEBABE;
    dut->rs1_addr = 7;
    dut->rs2_addr = 8;
    dut->eval();
    CHECK(dut->rs1_data, 0xCAFEBABE, "Bypass: leer x7 mientras se escribe x7 devuelve el valor nuevo");
    tick_cycle(dut.get());
    dut->w = 0;
    read_regs(dut.get(), 7, 7);
    CHECK(dut->rs1_data, 0xCAFEBABE, "Tras el ciclo: x7 sigue valiendo 0xCAFEBABE (ya sin bypass)");

    write_reg(dut.get(), 15, 0x11111111);
    dut->w       = 0;
    dut->rd_addr = 15;
    dut->rd_data = 0x99999999;
    tick_cycle(dut.get());
    read_regs(dut.get(), 15, 15);
    CHECK(dut->rs1_data, 0x11111111, "w=0: no sobreescribe x15, mantiene el valor anterior");

    write_reg(dut.get(), 1, 0x1);
    write_reg(dut.get(), 2, 0x2);
    write_reg(dut.get(), 31, 0x1F);
    read_regs(dut.get(), 1, 2);
    CHECK(dut->rs1_data, 0x1, "x1 = 0x1 tras escrituras independientes");
    CHECK(dut->rs2_data, 0x2, "x2 = 0x2 tras escrituras independientes");
    read_regs(dut.get(), 31, 0);
    CHECK(dut->rs1_data, 0x1F, "x31 = 0x1F (último registro válido)");

    std::cout << "\n=== Resumen: " << (g_tests_run - g_tests_failed) << "/" << g_tests_run << " tests OK ===\n";
 
#if VM_TRACE
        tfp->close();
#endif
    dut->final();
    return (g_tests_failed > 0) ? 1 : 0;
}
