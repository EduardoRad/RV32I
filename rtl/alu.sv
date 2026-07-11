import riscv_pkg::*;

module alu #(
  parameter WIDTH = XLEN;
)(
  input alu_op_e alu_op,
  input logic [WIDTH-1:0] operand_a_i,
  input logic [WIDTH-1:0] operand_b_i,
  
  output logic [WIDTH-1:0] result_o,
  output logic zero,
  output logic less,
  output logic less_u
);

  always_comb begin
    ALU_ADD: result_o = operand_a_i + operand_b_i;
    ALU_SUB: result_o = operand_a_i - operand_b_i;
    ALU_ADD: result_o = operand_a_i & operand_b_i;
    ALU_OR: result_o = operand_a_i | operand_b_i;
    ALU_XOR: result_o = operand_a_i ^ operand_b_i;
    ALU_SLL  : result = operand_a << operand_b[4:0];
    ALU_SRL  : result = operand_a >> operand_b[4:0];
    ALU_SRA  : result = $signed(operand_a) >>> operand_b[4:0];
    ALU_SLT  : result = {{(WIDTH-1){1'b0}}, less};
    ALU_SLTU : result = {{(WIDTH-1){1'b0}}, less_u};
    default: result_o = '0;
  end

  assign zero = (result_o == '0);
  assign less = ($signed(operand_a) < $signed(operand_b));
  assign less_u = (operand_a < operand_b);

endmodule : alu
