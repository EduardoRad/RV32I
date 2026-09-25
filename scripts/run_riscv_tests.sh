#!/usr/bin/env bash

# =============================================================================
# run_riscv_tests.sh — corre todos los rv32ui-p-* de tests/riscv-tests-bin/
# sobre cpu_top y reporta un resumen PASS/FAIL.
#
# Uso: ./run_riscv_tests.sh [n_ciclos]
# =============================================================

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

TESTS_DIR="tests/riscv-tests-bin"
HEX_DIR="tb/hex/riscv-tests"
OBJ_DIR="sim/obj_dir/riscv_test"
N_CYCLES="${1:-5000}"

RTL_FILES="rtl/riscv_pkg.sv rtl/cpu_top.sv rtl/pc.sv rtl/instr_mem.sv rtl/decoder.sv rtl/control_unit.sv rtl/imm_gen.sv rtl/reg_file.sv rtl/alu.sv rtl/data_mem.sv"

mkdir -p "$HEX_DIR"

PASS=0
FAIL=0
FAILED_TESTS=()

for elf in "$TESTS_DIR"/rv32ui-p-*; do
  [ -f "$elf" ] || continue
  name=$(basename "$elf")

  instr_hex="${HEX_DIR}/${name}_instr.hex"
  data_hex="${HEX_DIR}/${name}_data.hex"

  python3 "${SCRIPT_DIR}/elf_to_hex.py" "$elf" "$instr_hex" "$data_hex" >/dev/null

  rm -rf "$OBJ_DIR"
  verilator --cc --exe --Wall --Wno-fatal \
    tb/tb_riscv_test.cpp $RTL_FILES \
    --top-module cpu_top -Mdir "$OBJ_DIR" -I./tb \
    -GHEX_FILE="\"${instr_hex}\"" \
    -GRESET_ADDR="32'h80000000" \
    -GIMEM_SIZE_WORDS=16384 -GDMEM_SIZE_BYTES=65536 \
    >/dev/null 2>&1

  if [ $? -ne 0 ]; then
    echo "[BUILD ERROR] $name"
    FAIL=$((FAIL + 1))
    FAILED_TESTS+=("$name (build)")
    continue
  fi

  make -C "$OBJ_DIR" -f Vcpu_top.mk >/dev/null 2>&1

  result=$("$OBJ_DIR/Vcpu_top" "$N_CYCLES")
  status=$?

  if [ $status -eq 0 ]; then
    echo "[PASS] $name"
    PASS=$((PASS + 1))
  else
    echo "[FAIL] $name -> $result"
    FAIL=$((FAIL + 1))
    FAILED_TESTS+=("$name")
  fi
done

echo ""
echo "=== Resumen: $PASS/$((PASS + FAIL)) PASS ==="
if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
  echo "Fallos:"
  printf '  - %s\n' "${FAILED_TESTS[@]}"
fi
