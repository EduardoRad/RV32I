import riscv_pkg::*;

module decoder #(
  parameter WIDTH = XLEN
  )(
  input logic [WIDTH-1:0] instr_i,

  output opcode_e opcode_o,
  output logic [4:0] rd,
  output logic [2:0] funct3,
  output logic [4:0] rs1,
  output logic [4:0] rs2,
  output logic [6:0] funct7
);

  assign opcode_o = opcode_e'(instr_i[6:0]);
  assign rd = instr_i[11:7];
  assign funct3 = instr_i[14:12];
  assign rs1 = instr_i[19:15];
  assign rs2 = instr_i[24:20];
  assign funct7 = instr_i[31:25];

endmodule : decoder
