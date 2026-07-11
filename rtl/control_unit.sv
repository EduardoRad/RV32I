import riscv_pkg::*;

module control_unit #(
  parameter WIDTH = XLEN
  )(
  input opcode_e opcode,
  input logic [2:0] funct3,
  input logic [6:0] funct7,

  output logic reg_write,
  output logic alu_src,
  output logic alu_a_sel,
  output alu_op_e alu_op,
  output logic mem_read,
  output logic mem_write,
  output wb_sel_e wb_sel,
  output logic branch,
  output logic jump,
  output imm_type_e imm_type
);

  always_comb begin

    reg_write  = 1'b0;
    alu_src    = 1'b0;
    alu_a_sel  = 1'b0;
    alu_op     = ALU_ADD;
    mem_read   = 1'b0;
    mem_write  = 1'b0;
    wb_sel     = WB_ALU;
    branch     = 1'b0;
    jump       = 1'b0;
    imm_type   = IMM_I;

    unique case (opcode)
      OP_REG: begin

        reg_write = 1'b1;
        alu_src   = 1'b0;
        wb_sel    = WB_ALU;

        unique case (funct3_alu_e'(funct3))
          F3_ADD_SUB: alu_op = (funct7_alt_e'(funct7) == F7_SUB_SRA) ? ALU_SUB : ALU_ADD;
          F3_SLL: alu_op = ALU_SLL;
          F3_SLT: alu_op = ALU_SLT;
          F3_SLTU: alu_op = ALU_SLTU;
          F3_XOR: alu_op = ALU_XOR;
          F3_SRL_SRA: alu_op = (funct7_alt_e'(funct7) == F7_SUB_SRA) ? ALU_SRA : ALU_SRL;
          F3_OR: alu_op = ALU_OR;
          F3_AND: alu_op = ALU_AND;
          default : alu_op = ALU_ADD;
        endcase
      end

      OP_IMM: begin

        reg_write = 1'b1;
        alu_src = 1'b1;
        wb_sel = WB_ALU;
        imm_type = IMM_I;

        unique case (funct3_alu_e'(funct3))
          F3_ADD_SUB: alu_op = ALU_ADD;
          F3_SLL: alu_op = ALU_SLL;
          F3_SLT: alu_op = ALU_SLT;
          F3_SLTU: alu_op = ALU_SLTU;
          F3_XOR: alu_op = ALU_XOR;
          F3_SRL_SRA: alu_op = (funct7_alt_e'(funct7) == F7_SUB_SRA) ? ALU_SRA : ALU_SRL;
          F3_OR: alu_op = ALU_OR;
          F3_AND: alu_op = ALU_AND;
          default : alu_op = ALU_ADD;
        endcase
      end

      OP_LOAD: begin
        reg_write = 1'b1;
        alu_src = 1'b1;
        alu_op = ALU_ADD;
        mem_read = 1'b1;
        wb_sel = WB_MEM;
        imm_type = IMM_I;
      end

      OP_STORE: begin
        reg_write = 1'b0;
        alu_src = 1'b1;
        alu_op = ALU_ADD;
        mem_write = 1'b1;
        imm_type = IMM_S;
      end

      OP_BRANCH: begin
        reg_write = 1'b0;
        alu_src = 1'b0;
        branch = 1'b1;
        imm_type = IMM_B;
      end

      OP_JAL: begin
        reg_write = 1'b1;
        jump      = 1'b1;
        wb_sel    = WB_PC4;
        imm_type  = IMM_J;
      end

      OP_JALR: begin
        reg_write = 1'b1;
        alu_src   = 1'b1;
        alu_op    = ALU_ADD;
        jump      = 1'b1;
        wb_sel    = WB_PC4;
        imm_type  = IMM_I;
      end

      OP_LUI: begin
        reg_write = 1'b1;
        wb_sel    = WB_IMM;
        imm_type  = IMM_U;
      end

      OP_AUIPC: begin
        reg_write = 1'b1;
        alu_src   = 1'b1;
        alu_a_sel = 1'b1;
        alu_op    = ALU_ADD;
        wb_sel    = WB_ALU;
        imm_type  = IMM_U;
      end

      default : begin
        
      end
    endcase
  end

endmodule
