clone_riscv_tests:
		git clone https://github.com/riscv-software-src/riscv-tests.git
		cd riscv-tests
		git submodule update --init --recursive

build_riscv_tests:
		cd riscv-tests
		autoconf
		./configure --prefix=$RISCV/target
