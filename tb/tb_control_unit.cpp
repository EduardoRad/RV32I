#include <memory>
#include <iostream>
#include <cstdint>
 
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vcontrol_unit.h"
using VDut = Vcontrol_unit;

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
 
// -----------------------------------------------------------------------
// Helper: fuerza opcode/funct3/funct7 y evalúa. Evita repetir 3 líneas
// en cada test.
// -----------------------------------------------------------------------
static void decode(VDut* dut, uint8_t opcode, uint8_t funct3, uint8_t funct7) {
    dut->opcode = opcode;
    dut->funct3 = funct3;
    dut->funct7 = funct7;
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
    std::cout << "\n=== Tests: control_unit ===\n\n";
 
    // ===================================================================
    // OP_REG — ADD vs SUB (mismo funct3, distinto funct7)
    // ===================================================================
    decode(dut.get(), OP_REG, F3_ADD_SUB, F7_ADD_SRL);
    CHECK(dut->alu_op,    ALU_ADD, "ADD: alu_op = ALU_ADD");
    CHECK(dut->reg_write, 1,       "ADD: reg_write = 1");
    CHECK(dut->alu_src,   0,       "ADD: alu_src = 0 (usa rs2)");
    CHECK(dut->wb_sel,    WB_ALU,  "ADD: wb_sel = WB_ALU");
 
    decode(dut.get(), OP_REG, F3_ADD_SUB, F7_SUB_SRA);
    CHECK(dut->alu_op, ALU_SUB, "SUB: alu_op = ALU_SUB (funct7 distingue)");
 
    // OP_REG — SRL vs SRA (mismo funct3, distinto funct7)
    decode(dut.get(), OP_REG, F3_SRL_SRA, F7_ADD_SRL);
    CHECK(dut->alu_op, ALU_SRL, "SRL: alu_op = ALU_SRL");
 
    decode(dut.get(), OP_REG, F3_SRL_SRA, F7_SUB_SRA);
    CHECK(dut->alu_op, ALU_SRA, "SRA: alu_op = ALU_SRA (funct7 distingue)");
 
    // OP_REG — resto de operaciones (funct7 no debería importar)
    decode(dut.get(), OP_REG, F3_AND, F7_ADD_SRL);
    CHECK(dut->alu_op, ALU_AND, "AND: alu_op = ALU_AND");
 
    decode(dut.get(), OP_REG, F3_OR, F7_ADD_SRL);
    CHECK(dut->alu_op, ALU_OR, "OR: alu_op = ALU_OR");
 
    decode(dut.get(), OP_REG, F3_SLT, F7_ADD_SRL);
    CHECK(dut->alu_op, ALU_SLT, "SLT: alu_op = ALU_SLT");
 
    // ===================================================================
    // OP_IMM — ADDI nunca debe generar SUB, aunque funct7 valga F7_SUB_SRA
    // (ese bit en realidad forma parte del inmediato, no es un funct7 real)
    // ===================================================================
    decode(dut.get(), OP_IMM, F3_ADD_SUB, F7_SUB_SRA);
    CHECK(dut->alu_op,  ALU_ADD, "ADDI: alu_op = ALU_ADD (no existe SUBI)");
    CHECK(dut->alu_src, 1,       "ADDI: alu_src = 1 (usa imm)");
 
    // OP_IMM — SRLI vs SRAI (aquí funct7 sí importa, viene de instr[31:25])
    decode(dut.get(), OP_IMM, F3_SRL_SRA, F7_ADD_SRL);
    CHECK(dut->alu_op, ALU_SRL, "SRLI: alu_op = ALU_SRL");
 
    decode(dut.get(), OP_IMM, F3_SRL_SRA, F7_SUB_SRA);
    CHECK(dut->alu_op, ALU_SRA, "SRAI: alu_op = ALU_SRA");
 
    decode(dut.get(), OP_IMM, F3_AND, F7_ADD_SRL);
    CHECK(dut->alu_op, ALU_AND, "ANDI: alu_op = ALU_AND");
 
    // ===================================================================
    // OP_LOAD / OP_STORE
    // ===================================================================
    decode(dut.get(), OP_LOAD, F3_LW, 0);
    CHECK(dut->reg_write, 1,      "LW: reg_write = 1");
    CHECK(dut->alu_src,   1,      "LW: alu_src = 1 (rs1+imm)");
    CHECK(dut->alu_op,    ALU_ADD,"LW: alu_op = ALU_ADD (calcula dirección)");
    CHECK(dut->mem_read,  1,      "LW: mem_read = 1");
    CHECK(dut->wb_sel,    WB_MEM, "LW: wb_sel = WB_MEM");
    CHECK(dut->imm_type,  IMM_I,  "LW: imm_type = IMM_I");
 
    decode(dut.get(), OP_STORE, F3_SW, 0);
    CHECK(dut->reg_write,  0,      "SW: reg_write = 0 (no escribe registro)");
    CHECK(dut->mem_write,  1,      "SW: mem_write = 1");
    CHECK(dut->imm_type,   IMM_S,  "SW: imm_type = IMM_S");
 
    // ===================================================================
    // OP_BRANCH
    // ===================================================================
    decode(dut.get(), OP_BRANCH, F3_BEQ, 0);
    CHECK(dut->branch,    1,      "BEQ: branch = 1");
    CHECK(dut->reg_write, 0,      "BEQ: reg_write = 0");
    CHECK(dut->alu_src,   0,      "BEQ: alu_src = 0 (compara rs1 vs rs2)");
    CHECK(dut->imm_type,  IMM_B,  "BEQ: imm_type = IMM_B");
 
    // ===================================================================
    // OP_JAL / OP_JALR
    // ===================================================================
    decode(dut.get(), OP_JAL, 0, 0);
    CHECK(dut->reg_write, 1,      "JAL: reg_write = 1");
    CHECK(dut->jump,      1,      "JAL: jump = 1");
    CHECK(dut->wb_sel,    WB_PC4, "JAL: wb_sel = WB_PC4");
    CHECK(dut->imm_type,  IMM_J,  "JAL: imm_type = IMM_J");
 
    decode(dut.get(), OP_JALR, 0, 0);
    CHECK(dut->jump,     1,       "JALR: jump = 1");
    CHECK(dut->alu_src,  1,       "JALR: alu_src = 1 (rs1+imm para el target)");
    CHECK(dut->alu_op,   ALU_ADD, "JALR: alu_op = ALU_ADD");
    CHECK(dut->wb_sel,   WB_PC4,  "JALR: wb_sel = WB_PC4");
    CHECK(dut->imm_type, IMM_I,   "JALR: imm_type = IMM_I");
 
    // ===================================================================
    // OP_LUI / OP_AUIPC
    // ===================================================================
    decode(dut.get(), OP_LUI, 0, 0);
    CHECK(dut->reg_write, 1,      "LUI: reg_write = 1");
    CHECK(dut->wb_sel,    WB_IMM, "LUI: wb_sel = WB_IMM (no pasa por la ALU)");
    CHECK(dut->imm_type,  IMM_U,  "LUI: imm_type = IMM_U");
 
    decode(dut.get(), OP_AUIPC, 0, 0);
    CHECK(dut->reg_write, 1,      "AUIPC: reg_write = 1");
    CHECK(dut->alu_a_sel, 1,      "AUIPC: alu_a_sel = 1 (operando A = PC)");
    CHECK(dut->alu_src,   1,      "AUIPC: alu_src = 1 (operando B = imm)");
    CHECK(dut->wb_sel,    WB_ALU, "AUIPC: wb_sel = WB_ALU");
    CHECK(dut->imm_type,  IMM_U,  "AUIPC: imm_type = IMM_U");
 
    // ===================================================================
    // Resumen final
    // ===================================================================
    std::cout << "\n=== Resumen: " << (g_tests_run - g_tests_failed)
              << "/" << g_tests_run << " tests OK ===\n";
 
#if VM_TRACE
        tfp->close();
#endif
    dut->final();
    return (g_tests_failed > 0) ? 1 : 0;
}
