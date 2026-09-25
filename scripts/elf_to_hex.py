import subprocess
import sys
import tempfile
import os
 
def main():
    if len(sys.argv) != 4:
        print(f"Uso: {sys.argv[0]} <elf> <out_instr.hex> <out_data.hex>")
        sys.exit(1)
 
    elf_path, out_instr, out_data = sys.argv[1:4]
 
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as tmp:
        bin_path = tmp.name
 
    try:
        subprocess.run(
            ["riscv64-unknown-elf-objcopy", "-O", "binary", elf_path, bin_path],
            check=True,
        )
 
        with open(bin_path, "rb") as f:
            data = f.read()
 
        if len(data) % 4 != 0:
            data += b"\x00" * (4 - len(data) % 4)
 
        with open(out_instr, "w") as f:
            for i in range(0, len(data), 4):
                word = int.from_bytes(data[i:i+4], byteorder="little")
                f.write(f"{word:08x}\n")
 
        with open(out_data, "w") as f:
            for b in data:
                f.write(f"{b:02x}\n")
 
        print(f"OK: {len(data)} bytes -> {out_instr} ({len(data)//4} palabras), {out_data} ({len(data)} bytes)")
 
    finally:
        os.unlink(bin_path)
 
if __name__ == "__main__":
    main()
