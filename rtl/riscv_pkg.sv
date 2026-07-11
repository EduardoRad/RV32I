// =============================================================================
// Archivo: riscv_pkg.sv
// Descripcion: Paquete con las definiciones necesarias para el core.
// Package común: aquí van todas las constantes, tipos y funciones que
// comparten los distintos módulos del CPU (decoder, control_unit, alu...)
// Se incluye con: import riscv_pkg::*;
// =============================================================================


package riscv_pkg;

  parameter int XLEN = 32;

  typedef enum logic [6:0] {
    OP_LUI    = 7'b0110111,
    OP_AUIPC  = 7'b0010111,
    OP_JAL    = 7'b1101111,
    OP_JALR   = 7'b1100111,
    OP_BRANCH = 7'b1100011,
    OP_LOAD   = 7'b0000011,
    OP_STORE  = 7'b0100011,
    OP_IMM    = 7'b0010011, // ADDI, ANDI, ORI, etc.
    OP_REG    = 7'b0110011  // ADD, SUB, AND, OR, etc.
  } opcode_e;

  typedef enum logic [3:0] {
    ALU_ADD  = 4'b0000,
    ALU_SUB  = 4'b0001,
    ALU_AND  = 4'b0010,
    ALU_OR   = 4'b0011,
    ALU_XOR  = 4'b0100,
    ALU_SLL  = 4'b0101,
    ALU_SRL  = 4'b0110,
    ALU_SRA  = 4'b0111,
    ALU_SLT  = 4'b1000,
    ALU_SLTU = 4'b1001
  } alu_op_e;

  typedef enum logic [2:0] {
    F3_ADD_SUB   = 3'b000, // ADDI/ADD (funct7 distingue ADD vs SUB)
    F3_SLL       = 3'b001, // SLLI/SLL
    F3_SLT       = 3'b010, // SLTI/SLT
    F3_SLTU      = 3'b011, // SLTIU/SLTU
    F3_XOR       = 3'b100, // XORI/XOR
    F3_SRL_SRA   = 3'b101, // SRLI/SRAI/SRL/SRA (funct7 distingue lógico vs aritmético)
    F3_OR        = 3'b110, // ORI/OR
    F3_AND       = 3'b111  // ANDI/AND
  } funct3_alu_e;

  typedef enum logic [6:0] {
    F7_ADD_SRL = 7'b0000000, // ADD, SRL, SRLI (variante "base")
    F7_SUB_SRA = 7'b0100000  // SUB, SRA, SRAI (variante "alternativa")
  } funct7_alt_e;

  typedef enum logic [2:0] {
    F3_BEQ  = 3'b000,
    F3_BNE  = 3'b001,
    F3_BLT  = 3'b100,
    F3_BGE  = 3'b101,
    F3_BLTU = 3'b110,
    F3_BGEU = 3'b111
  } funct3_branch_e;

  typedef enum logic [2:0] {
    F3_LB  = 3'b000, // load byte, con signo
    F3_LH  = 3'b001, // load half-word, con signo
    F3_LW  = 3'b010, // load word
    F3_LBU = 3'b100, // load byte, sin signo (zero-extend)
    F3_LHU = 3'b101  // load half-word, sin signo (zero-extend)
  } funct3_load_e;

  typedef enum logic [2:0] {
    F3_SB = 3'b000, // store byte
    F3_SH = 3'b001, // store half-word
    F3_SW = 3'b010  // store word
  } funct3_store_e;

  typedef enum logic [2:0] {
    IMM_I,
    IMM_S,
    IMM_B,
    IMM_U,
    IMM_J
  } imm_type_e;

endpackage : riscv_pkg
