import riscv_pkg::*;

module reg_file#(
    parameter int WIDTH = XLEN,
    parameter int NREGS = 32
)(
    input logic             clk,
    input logic             rst_n,

    input logic             w,
    input logic [4:0]       rd_addr,
    input logic [WIDTH-1:0] rd_data,


    input logic [4:0]       rs1_addr,
    input logic [4:0]       rs2_addr,
    output logic [WIDTH-1:0] rs1_data,
    output logic [WIDTH-1:0] rs2_data,

    input  logic [4:0]       dbg_addr_i,
    output logic [WIDTH-1:0] dbg_data_o
);

    logic [WIDTH-1:0] regs [NREGS];
    
    //Escritura
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            for (int i = 0; i < NREGS; i++) begin
                regs[i] <= '0;
            end
        end else if (w && rd_addr != 5'b00000) begin
            regs[rd_addr] <= rd_data;
        end
    end

    //Lectura
    always_comb begin
        if (rs1_addr == 5'b00000) begin
            rs1_data = '0;
        end
        else begin
            rs1_data = regs[rs1_addr];
        end

        if (rs2_addr == 5'b00000) begin
            rs2_data = '0;
        end
        else begin
            rs2_data = regs[rs2_addr];
        end
    end

    assign dbg_data_o = (dbg_addr_i == 5'b00000) ? '0 : regs[dbg_addr_i];

endmodule : reg_file
