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
    input logic [WIDTH-1:0] rs1_data,
    input logic [WIDTH-1:0] rs2_data,
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
        else if (w && rs1_addr == rd_addr) begin
            rs1_data = rd_data;
        end
        else
            rs1_data = regs[rs1_addr];

        if (rs2_addr == 5'b00000) begin
            rs2_data = '0;
        end
        else if (w && rs2_addr == rd_addr) begin
            rs2_data = rd_data;
        end
        else
            rs2_data = regs[rs2_addr];
    end
endmodule : reg_file
