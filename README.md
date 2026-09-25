# RV32I — Procesador RISC-V (RV32I)
 
Implementación de un procesador RISC-V RV32I monociclo en SystemVerilog, verificado con testbenches en C++ sobre Verilator.
 
## Estructura del proyecto
 
```
.
├── rtl/                  # Módulos SystemVerilog (.sv)
│   ├── riscv_pkg.sv       # Package común: opcodes, enums, constantes
│   ├── control_unit.sv
│   └── ...
├── tb/                   # Testbenches en C++
│   ├── riscv_pkg.h        # Espejo en C++ de los enums de riscv_pkg.sv
│   ├── tb_control_unit.cpp
│   └── ...
└── obj_dir/              # Salida generada por Verilator (no versionar)
```
 
## Cómo correr los testbenches
 
Cada testbench se compila y ejecuta en tres pasos: **elaborar** el RTL con Verilator, **compilar** el C++ generado, y **ejecutar** el binario resultante.
 
### Pasos generales
 
```bash
verilator --cc --exe tb/tb_<nombre>.cpp rtl/riscv_pkg.sv rtl/<modulos_necesarios>.sv \
          --top-module <nombre> -Mdir obj_dir -I./tb/
 
make -C obj_dir -f V<nombre>.mk
 
./obj_dir/V<nombre>
```
 
| Parámetro | Significado |
|---|---|
| `tb/tb_<nombre>.cpp` | Testbench en C++ del módulo a probar |
| `rtl/riscv_pkg.sv` | Package común — **siempre debe ir antes** que cualquier módulo que lo importe |
| `rtl/<modulos_necesarios>.sv` | El/los archivo(s) `.sv` con el módulo a testear (y sus dependencias, si las tiene) |
| `--top-module <nombre>` | Módulo raíz de la elaboración |
| `-Mdir obj_dir` | Carpeta donde Verilator genera el modelo C++ |
| `-I./tb/` | Ruta donde buscar headers de C++ (p. ej. `riscv_pkg.h`) |
 
### Ejemplo — `control_unit`
 
```bash
verilator --cc --exe tb/tb_control_unit.cpp rtl/riscv_pkg.sv rtl/control_unit.sv \
          --top-module control_unit -Mdir sim/obj_dir -I./tb/
 
make -C sim/obj_dir -f Vcontrol_unit.mk
 
./sim/obj_dir/Vcontrol_unit

```
 ### Atajo con `run_tb.sh`
 
En vez de repetir estos tres comandos a mano por cada módulo, puedes usar el script `run_tb.sh` incluido en la raíz del proyecto:
 
```bash
./run_tb.sh control_unit
```
 
Opciones disponibles: `--trace` (genera `wave.vcd` para GTKWave) y `--clean` (borra el build anterior de ese módulo antes de recompilar). Ver `./run_tb.sh` sin argumentos para más detalles.
 
## Ejecutar `riscv-tests` sobre `cpu_top`
 
Además de los testbenches unitarios, `cpu_top` se verifica contra el subset `rv32ui-p-*` de [riscv-tests](https://github.com/riscv-software-src/riscv-tests) (instrucciones base RV32I, modo físico sin MMU — el único subset compatible con este core, que no implementa CSRs/privilegios ni extensiones M/A).
 
### 1. Clonar, compilar y seleccionar los tests (una sola vez)
 
```bash
make riscv-tests-select
```
 
Esto clona `riscv-tests`, lo compila con el toolchain RISC-V, y copia solo los ELF `rv32ui-p-*` a `tests/riscv-tests-bin/`. Ver `Makefile.riscv-tests` para los targets individuales (`riscv-tests-clone`, `riscv-tests-build`) y variables ajustables (`RISCV_PREFIX`, `TEST_PREFIX`).
 
### 2. Correr todos los tests
 
```bash
make run_tests
```
 
Convierte cada ELF a `.hex` (`scripts/elf_to_hex.py`), compila `cpu_top` para ese test y ejecuta, mostrando un resumen final `PASS`/`FAIL`. Ajusta el presupuesto de ciclos si algún test necesita más margen:
 
```bash
make run_tests N_CYCLES=10000
```
 
### 3. Correr un solo test
 
```bash
make run_test TEST=rv32ui-p-add
```
 
Con forma de onda para depurar en GTKWave:
 
```bash
make run_test TEST=rv32ui-p-add TRACE=1
gtkwave wave_rv32ui-p-add.vcd
```
 
### Cómo se detecta pass/fail
 
Los tests de `riscv-tests` escriben en `gp` (`x3`) antes de un `ecall` final: `gp == 1` es pass, `gp` impar `> 1` es fail (`gp >> 1` identifica el sub-test que falló). Como `control_unit` no implementa `ECALL` (cae al `default`, no-op), el `ecall` no interrumpe la ejecución — el testbench simplemente corre un número fijo de ciclos y lee `gp` por el puerto de depuración de `cpu_top` (`dbg_reg_addr_i`/`dbg_reg_data_o`).
 
### Puertos de depuración de `cpu_top`
 
`cpu_top` expone dos puertos de solo lectura pensados para verificación, sin acoplarse a rutas internas de Verilator:
 
| Puerto | Uso |
|---|---|
| `dbg_reg_addr_i` / `dbg_reg_data_o` | Lee cualquier registro de `reg_file` (combinacional) |
| `dbg_mem_addr_i` / `dbg_mem_data_o` | Lee una word de 32 bits de `data_mem` (combinacional) |

