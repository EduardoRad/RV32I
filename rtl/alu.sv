import riscv_pkg::*;

module alu #(
  parameter WIDTH = XLEN
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
    case (alu_op)
      ALU_ADD: result_o = operand_a_i + operand_b_i;
      ALU_SUB: result_o = operand_a_i - operand_b_i;
      ALU_AND: result_o = operand_a_i & operand_b_i;
      ALU_OR: result_o = operand_a_i | operand_b_i;
      ALU_XOR: result_o = operand_a_i ^ operand_b_i;
      ALU_SLL  : result_o = operand_a_i << operand_b_i[4:0];
      ALU_SRL  : result_o = operand_a_i >> operand_b_i[4:0];
      ALU_SRA  : result_o = $signed(operand_a_i) >>> operand_b_i[4:0];
      ALU_SLT  : result_o = {{(WIDTH-1){1'b0}}, less};
      ALU_SLTU : result_o = {{(WIDTH-1){1'b0}}, less_u};
      default: result_o = '0;
    endcase
  end

  assign zero = (operand_a_i == operand_b_i);
  assign less = ($signed(operand_a_i) < $signed(operand_b_i));
  assign less_u = (operand_a_i < operand_b_i);

endmodule : alu
