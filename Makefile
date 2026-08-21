RISCV_TESTS_DIR ?= riscv-tests
RISCV_TESTS_REPO = https://github.com/riscv-software-src/riscv-tests.git
RISCV_PREFIX     ?= riscv64-unknown-elf-
SEL_DIR           = tests/riscv-tests-bin

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
