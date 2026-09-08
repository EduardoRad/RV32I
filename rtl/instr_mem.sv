import riscv_pkg::*;

module instr_mem#(
  parameter int MEM_SIZE_WORDS = 1024,
  parameter string HEX_FILE = ""
)(
  input logic [31:0] addr_i,
  output logic [31:0] instr_o
);

  logic [31:0] mem [MEM_SIZE_WORDS];

  initial begin
    if (HEX_FILE != "") $readmemh(HEX_FILE, mem);
  end

  assign instr_o = mem[addr_i[31:2]];

endmodule : instr_mem
