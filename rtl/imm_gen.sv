import riscv_pkg::*;

module imm_gen#(
  input imm_type_e    imm_type_i,
  input logic [31:0]  instr_i,
  output logic [31:0] imm_o
);
  logic signed_ext;
  
  assign signed_ext = instr_i[31];

  always_comb begin
    unique case (imm_type_i)
      IMM_I: imm_o = { {20{signed_ext}}, instr_i[31:20] };
      IMM_S: imm_o = { {20{signed_ext}}, instr_i[31:25], instr_i[11:7] };
      IMM_B: imm_o = { {19{signed_ext}}, instr_i[31], instr_i[7], instr_i[30:25], instr_i[11:8], 1'b0 };
      IMM_U: imm_o = { instr_i[31:12], 12'b0 };
      IMM_J: imm_o = { {11{signed_ext}}, instr_i[31], instr_i[19:12], instr_i[20], instr_i[30:21], 1'b0 };
      default : imm = '0;
    endcase
  end

endmodule : imm_gen
