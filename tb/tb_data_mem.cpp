#include <memory>
#include <iostream>
#include <cstdint>
 
#include <verilated.h>
#include "Vdata_mem.h"
#include "riscv_pkg.h"
#if VM_TRACE
#include <verilated_vcd_c.h>
#endif
 
using VDut = Vdata_mem;
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

static void store(VDut* dut, uint32_t addr, uint32_t data, uint8_t funct3) {
    dut->mem_write_i   = 1;
    dut->mem_read_i    = 0;
    dut->addr_i        = addr;
    dut->write_data_i  = data;
    dut->funct3_i      = funct3;
    tick_cycle(dut);
    dut->mem_write_i = 0;
}
 
static uint32_t load(VDut* dut, uint32_t addr, uint8_t funct3) {
    dut->mem_write_i = 0;
    dut->mem_read_i  = 1;
    dut->addr_i      = addr;
    dut->funct3_i    = funct3;
    dut->eval();
    return dut->read_data_o;
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
 
    dut->clk           = 0;
    dut->mem_read_i    = 0;
    dut->mem_write_i   = 0;
    dut->addr_i        = 0;
    dut->write_data_i  = 0;
    dut->funct3_i      = 0;
    tick_cycle(dut.get());
 
    std::cout << "\n=== Tests: data_mem ===\n\n";

    store(dut.get(), 0, 0x12345678u, F3_SW);
    CHECK(load(dut.get(), 0, F3_LW), 0x12345678u, "SW+LW: word completa se recupera igual");

    CHECK(load(dut.get(), 0, F3_LBU), 0x78u, "Little-endian: byte 0 = 0x78 (LSB)");
    CHECK(load(dut.get(), 1, F3_LBU), 0x56u, "Little-endian: byte 1 = 0x56");
    CHECK(load(dut.get(), 2, F3_LBU), 0x34u, "Little-endian: byte 2 = 0x34");
    CHECK(load(dut.get(), 3, F3_LBU), 0x12u, "Little-endian: byte 3 = 0x12 (MSB)");

    store(dut.get(), 100, 0xFFFFFFFFu /* solo se usa el byte bajo: 0xFF */, F3_SB);
    CHECK(load(dut.get(), 100, F3_LB),  0xFFFFFFFFu, "LB: 0xFF sign-extended a 0xFFFFFFFF");
    CHECK(load(dut.get(), 100, F3_LBU), 0x000000FFu, "LBU: 0xFF zero-extended a 0x000000FF");

    store(dut.get(), 104, 0x0000007Fu, F3_SB);
    CHECK(load(dut.get(), 104, F3_LB),  0x0000007Fu, "LB: 0x7F (positivo) sin cambios");
    CHECK(load(dut.get(), 104, F3_LBU), 0x0000007Fu, "LBU: 0x7F (positivo) sin cambios");

    store(dut.get(), 200, 0x0000FFF0u /* half bajo = 0xFFF0 */, F3_SH);
    CHECK(load(dut.get(), 200, F3_LH),  0xFFFFFFF0u, "LH: 0xFFF0 sign-extended a 0xFFFFFFF0");
    CHECK(load(dut.get(), 200, F3_LHU), 0x0000FFF0u, "LHU: 0xFFF0 zero-extended a 0x0000FFF0");

    {
        dut->mem_write_i = 0;
        dut->mem_read_i  = 0;
        dut->addr_i      = 0; // esta dirección SÍ tiene 0x12345678 guardado
        dut->funct3_i    = F3_LW;
        dut->eval();
        CHECK(dut->read_data_o, 0, "mem_read_i=0: read_data_o = 0 aunque haya datos en esa dirección");
    }

    store(dut.get(), 300, 0xAAAAAAAAu, F3_SW); // primero un valor real conocido
    dut->mem_write_i  = 0; // ahora "intentamos" sobreescribir con we=0
    dut->addr_i       = 300;
    dut->write_data_i = 0xBBBBBBBBu;
    dut->funct3_i     = F3_SW;
    tick_cycle(dut.get());
    CHECK(load(dut.get(), 300, F3_LW), 0xAAAAAAAAu, "mem_write_i=0: no sobreescribe, mantiene 0xAAAAAAAA");

    store(dut.get(), 400, 0x11111111u, F3_SW);
    store(dut.get(), 404, 0x22222222u, F3_SW);
    CHECK(load(dut.get(), 400, F3_LW), 0x11111111u, "Independencia: addr=400 no se corrompe con la escritura en 404");
    CHECK(load(dut.get(), 404, F3_LW), 0x22222222u, "Independencia: addr=404 correcto");

    std::cout << "\n=== Resumen: " << (g_tests_run - g_tests_failed)
              << "/" << g_tests_run << " tests OK ===\n";
 
#if VM_TRACE
    tfp->close();
#endif
    dut->final();
    return (g_tests_failed > 0) ? 1 : 0;
}
