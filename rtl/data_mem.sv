import riscv_pkg::*;

module data_mem #(
  parameter int MEM_SIZE_BYTES = 4096
  )(
  input logic clk,
  input logic [31:0] addr_i,
  input logic [31:0] write_data_i,
  input logic mem_read_i,
  input logic mem_write_i,
  input logic [2:0] funct3_i,

  output logic [31:0] read_data_o,

  input  logic [31:0] dbg_addr_i,
  output logic [31:0] dbg_data_o
);

  logic [7:0] mem [MEM_SIZE_BYTES];

  always_ff @(posedge clk) begin
    if (mem_write_i) begin
        unique case (funct3_i)
            F3_SB: mem[addr_i] <= write_data_i[7:0];
            F3_SH: begin
                mem[addr_i]   <= write_data_i[7:0];
                mem[addr_i+1] <= write_data_i[15:8];
            end
            F3_SW: begin
                mem[addr_i]   <= write_data_i[7:0];
                mem[addr_i+1] <= write_data_i[15:8];
                mem[addr_i+2] <= write_data_i[23:16];
                mem[addr_i+3] <= write_data_i[31:24];
            end
            default: ;
        endcase
    end
  end

  always_comb begin
    if (!mem_read_i) begin
        read_data_o = '0;
    end else begin
      unique case (funct3_i)
          F3_LB:  read_data_o = {{24{mem[addr_i][7]}}, mem[addr_i]};
          F3_LBU: read_data_o = {24'b0, mem[addr_i]};
          F3_LH:  read_data_o = {{16{mem[addr_i+1][7]}}, mem[addr_i+1], mem[addr_i]};
          F3_LHU: read_data_o = {16'b0, mem[addr_i+1], mem[addr_i]};
          F3_LW:  read_data_o = {mem[addr_i+3], mem[addr_i+2], mem[addr_i+1], mem[addr_i]};
          default: read_data_o = '0;
      endcase
    end
  end

  assign dbg_data_o = {mem[dbg_addr_i+3], mem[dbg_addr_i+2], mem[dbg_addr_i+1], mem[dbg_addr_i]};

endmodule : data_mem
