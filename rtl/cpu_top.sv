import riscv_pkg::*;

module cpu_top #(
    parameter string HEX_FILE            = "",
    parameter int    IMEM_SIZE_WORDS     = 1024,
    parameter int    DMEM_SIZE_BYTES     = 4096,
    parameter logic [31:0] RESET_ADDR    = 32'h0000_0000
)(
    input logic clk,
    input logic rst_n,

    input  logic [4:0]  dbg_reg_addr_i,
    output logic [31:0] dbg_reg_data_o,
    input  logic [31:0] dbg_mem_addr_i,
    output logic [31:0] dbg_mem_data_o
);

    logic [31:0] pc_current, pc_next, pc_plus4;
    logic [31:0] instr;
 
    pc #(.RESET_ADDR(RESET_ADDR)) u_pc (
        .clk_i       (clk),
        .rst_n     (rst_n),
        .next_pc_i (pc_next),
        .pc_o      (pc_current)
    );

    assign pc_plus4 = pc_current + 32'd4;
 
    instr_mem #(
        .MEM_SIZE_WORDS (IMEM_SIZE_WORDS),
        .HEX_FILE       (HEX_FILE)
    ) u_imem (
        .addr_i  (pc_current),
        .instr_o (instr)
    );

    opcode_e    opcode;
    logic [4:0] rd, rs1, rs2;
    logic [2:0] funct3;
    logic [6:0] funct7;
 
    decoder u_decoder (
        .instr_i  (instr),
        .opcode_o (opcode),
        .rd       (rd),
        .funct3   (funct3),
        .rs1      (rs1),
        .rs2      (rs2),
        .funct7   (funct7)
    );
 
    logic       reg_write, alu_src, alu_a_sel, mem_read, mem_write, branch, jump;
    alu_op_e    alu_op;
    wb_sel_e    wb_sel;
    imm_type_e  imm_type;
 
    control_unit u_ctrl (
        .opcode    (opcode),
        .funct3    (funct3),
        .funct7    (funct7),
        .reg_write (reg_write),
        .alu_src   (alu_src),
        .alu_a_sel (alu_a_sel),
        .alu_op    (alu_op),
        .mem_read  (mem_read),
        .mem_write (mem_write),
        .wb_sel    (wb_sel),
        .branch    (branch),
        .jump      (jump),
        .imm_type  (imm_type)
    );
 
    logic [31:0] imm;
 
    imm_gen u_imm_gen (
        .imm_type_i (imm_type),
        .instr_i    (instr),
        .imm_o      (imm)
    );
 
    logic [31:0] rs1_data, rs2_data;
    logic [31:0] wb_data;
 
    reg_file u_reg_file (
        .clk      (clk),
        .rst_n    (rst_n),
        .w        (reg_write),
        .rd_addr  (rd),
        .rd_data  (wb_data),
        .rs1_addr (rs1),
        .rs2_addr (rs2),
        .rs1_data (rs1_data),
        .rs2_data (rs2_data),
        .dbg_addr_i   (dbg_reg_addr_i),
        .dbg_data_o   (dbg_reg_data_o)
    );

    logic [31:0] alu_operand_a, alu_operand_b, alu_result;
    logic        zero, less, less_u;
 
    assign alu_operand_a = alu_a_sel ? pc_current : rs1_data;
    assign alu_operand_b = alu_src   ? imm        : rs2_data;
 
    alu u_alu (
        .alu_op      (alu_op),
        .operand_a_i (alu_operand_a),
        .operand_b_i (alu_operand_b),
        .result_o    (alu_result),
        .zero        (zero),
        .less        (less),
        .less_u      (less_u)
    );

    logic branch_taken;
 
    always_comb begin
        unique case (funct3_branch_e'(funct3))
            F3_BEQ  : branch_taken = zero;
            F3_BNE  : branch_taken = ~zero;
            F3_BLT  : branch_taken = less;
            F3_BGE  : branch_taken = ~less;
            F3_BLTU : branch_taken = less_u;
            F3_BGEU : branch_taken = ~less_u;
            default : branch_taken = 1'b0;
        endcase
    end

    logic [31:0] pc_rel_target;
    assign pc_rel_target = pc_current + imm;
 
    assign pc_next = jump                     ? ((opcode == OP_JALR) ? alu_result : pc_rel_target) :
                     (branch && branch_taken) ? pc_rel_target :
                                                 pc_plus4;

    logic [31:0] mem_read_data;
 
    data_mem #(.MEM_SIZE_BYTES(DMEM_SIZE_BYTES), .HEX_FILE(HEX_FILE)) u_dmem (
        .clk          (clk),
        .addr_i       (alu_result),
        .write_data_i (rs2_data),
        .mem_read_i   (mem_read),
        .mem_write_i  (mem_write),
        .funct3_i     (funct3),
        .read_data_o  (mem_read_data),
        .dbg_addr_i   (dbg_mem_addr_i),
        .dbg_data_o   (dbg_mem_data_o)
    );

    always_comb begin
        unique case (wb_sel)
            WB_ALU  : wb_data = alu_result;
            WB_MEM  : wb_data = mem_read_data;
            WB_PC4  : wb_data = pc_plus4;
            WB_IMM  : wb_data = imm;
            default : wb_data = alu_result;
        endcase
    end
 
endmodule : cpu_top
