# Checklist — Procesador RV32I con Verilator
 
Marca cada tarea con `x` dentro de los corchetes según la vayas completando: `- [x]`
 
---
 
## Fase 0 — Preparación del entorno
- [x] Instalar Verilator (`sudo apt install verilator` o compilar desde fuente)
- [x] Instalar GTKWave (para ver formas de onda)
- [ ] Instalar toolchain RISC-V (`riscv64-unknown-elf-gcc` o `riscv32-unknown-elf-gcc`)
- [ ] Clonar `riscv-tests` y verificar que puedes compilar los tests a ELF
- [x] Crear estructura de carpetas del proyecto (`rtl/`, `tb/`, `sw/`, `sim/`)
## Fase 1 — Módulos individuales (con testbench propio cada uno)
- [ ] `reg_file.v` — banco de 32 registros, x0 fijo a 0, lectura combinacional/escritura síncrona
- [ ] `imm_gen.v` — extractor de inmediatos para los formatos I, S, B, U, J
- [ ] `alu.v` — todas las operaciones aritmético-lógicas + flags
- [ ] `decoder.v` — extraer opcode, funct3, funct7, rs1, rs2, rd
- [ ] `control_unit.v` — generar señales de control desde el decoder
- [ ] Testbench en C++ (Verilator) para cada módulo, verificando casos borde (overflow, x0, inmediatos negativos)
## Fase 2 — Memorias
- [ ] `instr_mem.v` — ROM simple, cargable desde archivo `.hex`
- [ ] `data_mem.v` — RAM con soporte para byte/half/word (LB, LH, LW y variantes con signo)
- [ ] Verificar carga de un `.hex` de prueba y lectura correcta
## Fase 3 — Integración del datapath monociclo
- [ ] `pc.v` — contador de programa con lógica de siguiente PC
- [ ] `cpu_top.v` — conectar todos los módulos anteriores
- [ ] Mux de siguiente PC (secuencial / branch / jump)
- [ ] Mux de escritura a registro (ALU result / memoria / PC+4 para JAL)
- [ ] Escribir programa de prueba a mano (5-10 instrucciones) y verificar registros manualmente con el testbench
## Fase 4 — Verificación con riscv-tests
- [ ] Script para compilar cada test individual a ELF → binario/hex
- [ ] Adaptar testbench para detectar pass/fail (dirección de memoria estándar `tohost`)
- [ ] Correr tests aritméticos (`rv32ui-p-add`, `rv32ui-p-sub`, etc.)
- [ ] Correr tests de load/store (`rv32ui-p-lw`, `rv32ui-p-sw`, etc.)
- [ ] Correr tests de branches/jumps (`rv32ui-p-beq`, `rv32ui-p-jal`, etc.)
- [ ] Automatizar con un script (bash/Makefile) que corra todos los tests y reporte resumen pass/fail
## Fase 5 — Debug y pulido
- [ ] Revisar formas de onda con GTKWave para cualquier test que falle
- [ ] Documentar el diseño (diagrama de datapath, mapa de señales de control)
- [ ] Medir CPI (debería ser 1, al ser monociclo) y frecuencia estimada de reloj
## Fase 6 — Extensión opcional (pipeline)
- [ ] Dividir el datapath en 5 etapas (IF, ID, EX, MEM, WB) con registros intermedios
- [ ] Detección de hazards y forwarding
- [ ] Manejo de branches en pipeline (stall o predicción simple)
- [ ] Re-correr riscv-tests sobre la versión pipelined

