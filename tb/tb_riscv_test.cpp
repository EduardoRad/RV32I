#include <memory>
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <string>
 
#include <verilated.h>
#include "Vcpu_top.h"
#if VM_TRACE
#include <verilated_vcd_c.h>
#endif
 
using VDut = Vcpu_top;

#if VM_TRACE
static VerilatedVcdC* g_tfp = nullptr;
#endif
static vluint64_t g_sim_time = 0;

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

static uint32_t read_reg(VDut* dut, uint8_t idx) {
    dut->dbg_reg_addr_i = idx;
    dut->eval();
#if VM_TRACE
    if (g_tfp) { g_tfp->dump(g_sim_time); g_sim_time++; }
#endif
    return dut->dbg_reg_data_o;
}
 
int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    int n_cycles = (argc > 1) ? std::atoi(argv[1]) : 5000;
    std::string vcd_name = (argc > 2) ? argv[2] : "wave.vcd";
 
    std::unique_ptr<VDut> dut = std::make_unique<VDut>();

#if VM_TRACE
    Verilated::traceEverOn(true);
    std::unique_ptr<VerilatedVcdC> tfp = std::make_unique<VerilatedVcdC>();
    dut->trace(tfp.get(), 99);
    tfp->open(vcd_name.c_str());
    g_tfp = tfp.get();
#endif
 
    dut->clk            = 0;
    dut->rst_n          = 0;
    dut->dbg_reg_addr_i = 0;
    dut->dbg_mem_addr_i = 0;
 
    for (int i = 0; i < 4; i++) tick_cycle(dut.get());
    dut->rst_n = 1;
 
    for (int i = 0; i < n_cycles; i++) tick_cycle(dut.get());
 
    uint32_t gp = read_reg(dut.get(), 3); // x3 = gp = TESTNUM

#if VM_TRACE
    tfp->close();
    std::cout << "Onda generada: " << vcd_name << " (gtkwave " << vcd_name << ")\n";
#endif
 
    dut->final();
 
    if (gp == 1) {
        std::cout << "PASS\n";
        return 0;
    } else if (gp & 1) {
        std::cout << "FAIL (test " << (gp >> 1) << ")\n";
        return 1;
    } else {
        std::cout << "TIMEOUT/UNKNOWN (gp=0x" << std::hex << gp << ")\n";
        return 1;
    }
}
