# Checklist — Procesador RV32I con Verilator
 
Marca cada tarea con `x` dentro de los corchetes según la vayas completando: `- [x]`
 
---
 
## Fase 0 — Preparación del entorno
- [x] Instalar Verilator (`sudo apt install verilator` o compilar desde fuente)
- [x] Instalar GTKWave (para ver formas de onda)
- [x] Instalar toolchain RISC-V (`riscv64-unknown-elf-gcc` o `riscv32-unknown-elf-gcc`)
- [x] Clonar `riscv-tests` y verificar que puedes compilar los tests a ELF
- [x] Crear estructura de carpetas del proyecto (`rtl/`, `tb/`, `sw/`, `sim/`)
## Fase 1 — Módulos individuales (con testbench propio cada uno)
- [x] `reg_file.v` — banco de 32 registros, x0 fijo a 0, lectura combinacional/escritura síncrona
- [x] `imm_gen.v` — extractor de inmediatos para los formatos I, S, B, U, J
- [x] `alu.v` — todas las operaciones aritmético-lógicas + flags
- [x] `decoder.v` — extraer opcode, funct3, funct7, rs1, rs2, rd
- [x] `control_unit.v` — generar señales de control desde el decoder
- [x] Testbench en C++ (Verilator) para cada módulo, verificando casos borde (overflow, x0, inmediatos negativos)
## Fase 2 — Memorias
- [x] `instr_mem.v` — ROM simple, cargable desde archivo `.hex`
- [x] `data_mem.v` — RAM con soporte para byte/half/word (LB, LH, LW y variantes con signo)
- [x] Verificar carga de un `.hex` de prueba y lectura correcta
## Fase 3 — Integración del datapath monociclo
- [x] `pc.v` — contador de programa con lógica de siguiente PC
- [x] `cpu_top.v` — conectar todos los módulos anteriores
- [x] Mux de siguiente PC (secuencial / branch / jump)
- [x] Mux de escritura a registro (ALU result / memoria / PC+4 para JAL)
- [x] Escribir programa de prueba a mano (5-10 instrucciones) y verificar registros manualmente con el testbench
## Fase 4 — Verificación con riscv-tests
- [x] Script para compilar cada test individual a ELF → binario/hex
- [x] Adaptar testbench para detectar pass/fail (dirección de memoria estándar `tohost`)
- [x] Correr tests aritméticos (`rv32ui-p-add`, `rv32ui-p-sub`, etc.)
- [x] Correr tests de load/store (`rv32ui-p-lw`, `rv32ui-p-sw`, etc.)
- [x] Correr tests de branches/jumps (`rv32ui-p-beq`, `rv32ui-p-jal`, etc.)
- [x] Automatizar con un script (bash/Makefile) que corra todos los tests y reporte resumen pass/fail

