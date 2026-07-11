#!/usr/bin/env bash
# =============================================================================
# run_tb.sh — compila y ejecuta un testbench de Verilator para un módulo dado
#
# Uso:
#   ./run_tb.sh <nombre_modulo> [archivos_extra.sv ...] [--trace] [--clean]
#
# Por defecto solo compila:
#   rtl/riscv_pkg.sv + rtl/<nombre_modulo>.sv
# (igual que harías a mano). Si tu módulo depende de OTROS archivos .sv
# (por ejemplo cpu_top.sv necesita pc.sv, decoder.sv, alu.sv...), pásalos
# explícitamente como argumentos extra.
#
# Ejemplos:
#   ./run_tb.sh control_unit
#   ./run_tb.sh alu --trace
#   ./run_tb.sh cpu_top rtl/pc.sv rtl/decoder.sv rtl/alu.sv rtl/reg_file.sv \
#               rtl/imm_gen.sv rtl/control_unit.sv rtl/instr_mem.sv rtl/data_mem.sv
#
# Convenciones de carpetas que asume este script (ajusta las variables
# de abajo si tu estructura es distinta):
#   rtl/<modulo>.sv        → RTL del módulo bajo test
#   tb/tb_<modulo>.cpp      → testbench en C++
#   tb/riscv_pkg.h          → header con los enums compartidos
#   obj_dir/<modulo>/       → salida de Verilator (uno por módulo, para no
#                             mezclar builds entre módulos distintos)
# =============================================================================

set -euo pipefail

# ---------------------------------------------------------------------
# Configuración (ajusta si tu estructura de carpetas es distinta)
# ---------------------------------------------------------------------
RTL_DIR="rtl"
TB_DIR="tb"
OBJ_ROOT="obj_dir"

# ---------------------------------------------------------------------
# Colores para la salida (se desactivan solos si no hay terminal TTY)
# ---------------------------------------------------------------------
if [ -t 1 ]; then
  C_RED=$'\033[0;31m'
  C_GREEN=$'\033[0;32m'
  C_YELLOW=$'\033[0;33m'
  C_BLUE=$'\033[0;34m'
  C_RESET=$'\033[0m'
else
  C_RED=""
  C_GREEN=""
  C_YELLOW=""
  C_BLUE=""
  C_RESET=""
fi

info() { echo "${C_BLUE}[run_tb]${C_RESET} $*"; }
ok() { echo "${C_GREEN}[run_tb]${C_RESET} $*"; }
warn() { echo "${C_YELLOW}[run_tb]${C_RESET} $*"; }
error() { echo "${C_RED}[run_tb]${C_RESET} $*" >&2; }

# ---------------------------------------------------------------------
# Argumentos
# ---------------------------------------------------------------------
if [ $# -lt 1 ]; then
  error "Uso: $0 <nombre_modulo> [archivos_extra.sv ...] [--trace] [--clean]"
  error "Ejemplo: $0 control_unit"
  exit 1
fi

MODULE="$1"
shift

TRACE=0
CLEAN=0
EXTRA_RTL=()
for arg in "$@"; do
  case "$arg" in
  --trace) TRACE=1 ;;
  --clean) CLEAN=1 ;;
  *.sv) EXTRA_RTL+=("$arg") ;;
  *)
    error "Opción/archivo no reconocido: $arg"
    exit 1
    ;;
  esac
done

TB_CPP="${TB_DIR}/tb_${MODULE}.cpp"
OBJ_DIR="${OBJ_ROOT}/${MODULE}"
TOP_CLASS="V${MODULE}"
PKG_FILE="${RTL_DIR}/riscv_pkg.sv"
MODULE_FILE="${RTL_DIR}/${MODULE}.sv"

# ---------------------------------------------------------------------
# Verificaciones previas
# ---------------------------------------------------------------------
if [ ! -f "$TB_CPP" ]; then
  error "No se encuentra el testbench: $TB_CPP"
  error "¿Seguro que el módulo se llama '${MODULE}' y el testbench sigue"
  error "la convención tb_<modulo>.cpp dentro de ${TB_DIR}/?"
  exit 1
fi

if [ ! -f "$MODULE_FILE" ]; then
  error "No se encuentra ${MODULE_FILE}."
  error "Si el módulo top vive en otro archivo, pásalo como argumento extra:"
  error "  $0 ${MODULE} rtl/archivo_real.sv"
  exit 1
fi

if [ ! -f "$PKG_FILE" ]; then
  warn "No se encuentra ${PKG_FILE} — se compilará sin el package común."
  PKG_FILE=""
fi

if [ "$CLEAN" -eq 1 ] && [ -d "$OBJ_DIR" ]; then
  info "Limpiando ${OBJ_DIR}..."
  rm -rf "$OBJ_DIR"
fi

# ---------------------------------------------------------------------
# 1) Elaboración con Verilator
#    Solo se compilan: riscv_pkg.sv + <modulo>.sv + los extra indicados.
#    Nada más — así el comportamiento coincide exactamente con hacerlo
#    a mano, y un error de sintaxis en un módulo no relacionado (p. ej.
#    alu.sv al testear control_unit) nunca se cuela en la compilación.
# ---------------------------------------------------------------------
VERILATOR_FLAGS=(--cc --exe --Wall --Wno-fatal)
[ "$TRACE" -eq 1 ] && VERILATOR_FLAGS+=(--trace)

RTL_FILES=()
[ -n "$PKG_FILE" ] && RTL_FILES+=("$PKG_FILE")
RTL_FILES+=("$MODULE_FILE")
RTL_FILES+=("${EXTRA_RTL[@]}")

info "Elaborando '${MODULE}' con Verilator..."
info "Archivos RTL: ${RTL_FILES[*]}"
verilator "${VERILATOR_FLAGS[@]}" \
  "$TB_CPP" "${RTL_FILES[@]}" \
  --top-module "$MODULE" \
  -Mdir "$OBJ_DIR" \
  -I"./${TB_DIR}"

# ---------------------------------------------------------------------
# 2) Compilación del C++ generado
# ---------------------------------------------------------------------
info "Compilando..."
make -C "$OBJ_DIR" -f "${TOP_CLASS}.mk" >/dev/null

# ---------------------------------------------------------------------
# 3) Ejecución
# ---------------------------------------------------------------------
info "Ejecutando testbench de '${MODULE}'..."
echo "----------------------------------------------------------------"
if "${OBJ_DIR}/${TOP_CLASS}"; then
  echo "----------------------------------------------------------------"
  ok "Todos los tests de '${MODULE}' pasaron."
  [ "$TRACE" -eq 1 ] && info "Onda generada: wave.vcd (ábrela con: gtkwave wave.vcd)"
  exit 0
else
  STATUS=$?
  echo "----------------------------------------------------------------"
  error "Algún test de '${MODULE}' ha fallado (código de salida: ${STATUS})."
  exit "$STATUS"
fi
