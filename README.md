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
          --top-module control_unit -Mdir obj_dir -I./tb/
 
make -C obj_dir -f Vcontrol_unit.mk
 
./obj_dir/Vcontrol_unit

```
 ### Atajo con `run_tb.sh`
 
En vez de repetir estos tres comandos a mano por cada módulo, puedes usar el script `run_tb.sh` incluido en la raíz del proyecto:
 
```bash
./run_tb.sh control_unit
```
 
Opciones disponibles: `--trace` (genera `wave.vcd` para GTKWave) y `--clean` (borra el build anterior de ese módulo antes de recompilar). Ver `./run_tb.sh` sin argumentos para más detalles.
 
