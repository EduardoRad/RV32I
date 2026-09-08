#include <memory>
#include <iostream>
#include <cstdint>
 
#include <verilated.h>
#include "Vinstr_mem.h"
#include "riscv_pkg.h"
#if VM_TRACE
#include <verilated_vcd_c.h>
#endif
 
using VDut = Vinstr_mem;
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
                      << " | esperado=0x" << std::hex << (uint64_t)_exp     \
                      << " obtenido=0x" << std::hex << (uint64_t)_act       \
                      << std::dec << "\n";                                  \
        } else {                                                            \
            std::cout << "[PASS] " << (description) << "\n";               \
        }                                                                    \
    } while (0)

static void fetch(VDut* dut, uint32_t byte_addr) {
    dut->addr_i = byte_addr;
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
 
    std::cout << "\n=== Tests: instr_mem ===\n\n";
    std::cout << "(requiere compilar con -GHEX_FILE apuntando a instr_mem_test.hex)\n\n";

    fetch(dut.get(), 0);
    CHECK(dut->instr_o, 0x00000013u, "addr=0  -> primera palabra (NOP)");
 
    fetch(dut.get(), 4);
    CHECK(dut->instr_o, 0x00100093u, "addr=4  -> segunda palabra (addi x1,x0,1)");
 
    fetch(dut.get(), 8);
    CHECK(dut->instr_o, 0x00200113u, "addr=8  -> tercera palabra (addi x2,x0,2)");
 
    fetch(dut.get(), 12);
    CHECK(dut->instr_o, 0x00300193u, "addr=12 -> cuarta palabra (addi x3,x0,3)");
 
    fetch(dut.get(), 16);
    CHECK(dut->instr_o, 0x00400213u, "addr=16 -> quinta palabra (addi x4,x0,4)");

    fetch(dut.get(), 4);
    CHECK(dut->instr_o, 0x00100093u,
          "addr=4 (no addr=1): confirma que se descarta addr_i[1:0], no se usa como índice directo");

    fetch(dut.get(), 20);
    CHECK(dut->instr_o, 0x00000000u,
          "addr=20: fuera del .hex cargado, Verilator rellena con 0 por defecto");

    std::cout << "\n=== Resumen: " << (g_tests_run - g_tests_failed)
              << "/" << g_tests_run << " tests OK ===\n";
 
#if VM_TRACE
    tfp->close();
#endif
    dut->final();
    return (g_tests_failed > 0) ? 1 : 0;
}
