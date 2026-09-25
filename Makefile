RISCV_TESTS_DIR ?= riscv-tests
RISCV_TESTS_REPO = https://github.com/riscv-software-src/riscv-tests.git
RISCV_PREFIX     ?= riscv64-unknown-elf-
SEL_DIR           = tests/riscv-tests-bin

SCRIPTS_DIR      = scripts
HEX_DIR          = tb/hex/riscv-tests
OBJ_DIR_SINGLE   = sim/obj_dir/riscv_test
N_CYCLES        ?= 20000

TEST_PREFIX ?= rv32ui-p-

riscv-tests-clone:
	@if [ -d "$(RISCV_TESTS_DIR)/.git" ]; then \
		echo "[riscv-tests] ya clonado en $(RISCV_TESTS_DIR)"; \
	else \
		echo "[riscv-tests] clonando..."; \
		git clone --recursive $(RISCV_TESTS_REPO) $(RISCV_TESTS_DIR); \
	fi
	@echo "[riscv-tests] actualizando submódulos (env/ y demás)..."
	cd $(RISCV_TESTS_DIR) && git submodule update --init --recursive

riscv-tests-build: riscv-tests-clone
	@echo "[riscv-tests] compilando"
	cd $(RISCV_TESTS_DIR) && autoconf
	cd $(RISCV_TESTS_DIR) && ./configure \
		--prefix=$(CURDIR)/$(RISCV_TESTS_DIR)/build \
		--with-xlen=32
	$(MAKE) -C $(RISCV_TESTS_DIR) RISCV_PREFIX=$(RISCV_PREFIX)

riscv-tests-select: riscv-tests-build
	@echo "[riscv-tests] limpiando $(SEL_DIR) antes de seleccionar (evita arrastrar tests de una selección anterior)..."
	@rm -rf $(SEL_DIR)
	@mkdir -p $(SEL_DIR)
	@echo "[riscv-tests] seleccionando tests con prefijo '$(TEST_PREFIX)'..."
	@count=0; \
	for f in $(RISCV_TESTS_DIR)/isa/$(TEST_PREFIX)*; do \
		case "$$f" in \
			*.dump) continue ;; \
		esac; \
		[ -f "$$f" ] || continue; \
		cp "$$f" $(SEL_DIR)/; \
		count=$$((count+1)); \
	done; \
	echo "[riscv-tests] $$count tests copiados a $(SEL_DIR)/"
	@echo "[riscv-tests] listado:"
	@ls $(SEL_DIR)/$(TEST_PREFIX)* 2>/dev/null | xargs -n1 basename

riscv-tests: riscv-tests-select

riscv-tests-clean:
	rm -rf $(SEL_DIR)
	$(MAKE) -C $(RISCV_TESTS_DIR) clean 2>/dev/null || true

CPU_RTL_FILES = rtl/riscv_pkg.sv rtl/cpu_top.sv rtl/pc.sv rtl/instr_mem.sv \
                rtl/decoder.sv rtl/control_unit.sv rtl/imm_gen.sv \
                rtl/reg_file.sv rtl/alu.sv rtl/data_mem.sv

ifdef TRACE
  VERILATOR_TRACE_FLAG = --trace
else
  VERILATOR_TRACE_FLAG =
endif
 
.PHONY: run_tests run_test

run_tests:
	@bash $(SCRIPTS_DIR)/run_riscv_tests.sh $(N_CYCLES)

run_test:
ifndef TEST
	$(error Debes indicar TEST=<nombre>, p.ej.: make run_test TEST=rv32ui-p-add)
endif
	@if [ ! -f "$(SEL_DIR)/$(TEST)" ]; then \
		echo "[run_test] No se encuentra $(SEL_DIR)/$(TEST) — ¿has hecho 'make riscv-tests-select'?"; \
		exit 1; \
	fi
	@mkdir -p $(HEX_DIR)
	@echo "[run_test] Convirtiendo $(TEST) a .hex..."
	@python3 $(SCRIPTS_DIR)/elf_to_hex.py \
		$(SEL_DIR)/$(TEST) \
		$(HEX_DIR)/$(TEST)_instr.hex \
		$(HEX_DIR)/$(TEST)_data.hex
	@rm -rf $(OBJ_DIR_SINGLE)
	@echo "[run_test] Elaborando y compilando cpu_top para $(TEST)..."
	@verilator --cc --exe --Wall --Wno-fatal $(VERILATOR_TRACE_FLAG) \
		tb/tb_riscv_test.cpp $(CPU_RTL_FILES) \
		--top-module cpu_top -Mdir $(OBJ_DIR_SINGLE) -I./tb \
		-GHEX_FILE="\"$(HEX_DIR)/$(TEST)_instr.hex\"" \
		-GRESET_ADDR="32'h80000000" \
		-GIMEM_SIZE_WORDS=16384 -GDMEM_SIZE_BYTES=65536
	@$(MAKE) -C $(OBJ_DIR_SINGLE) -f Vcpu_top.mk
	@echo "[run_test] Ejecutando (hasta $(N_CYCLES) ciclos)..."
	@$(OBJ_DIR_SINGLE)/Vcpu_top $(N_CYCLES) wave_$(TEST).vcd
