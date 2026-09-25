import riscv_pkg::*;

module pc #(
  parameter logic [31:0] RESET_ADDR = 32'h0000_0000
)(
  input logic clk_i,
  input logic rst_n,
  input logic [31:0] next_pc_i,

  output logic [31:0] pc_o
);

  always_ff @(posedge clk_i or negedge rst_n) begin
    if (!rst_n) begin
      pc_o <= RESET_ADDR;
    end else begin
      pc_o <= next_pc_i;
    end
  end

endmodule : pc
