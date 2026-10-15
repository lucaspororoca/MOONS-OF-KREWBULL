"""Encrypt resources as .pak files for the C++ in-memory resource loader."""

import sys
from pathlib import Path

# Keep this key in sync with scripts/resources/resource_archive.h.
XOR_KEY = b"MoonsOfKrewbull2024SecretKey!"

def xor_crypt(data: bytes) -> bytes:
    n = len(XOR_KEY)
    return bytes(b ^ XOR_KEY[i % n] for i, b in enumerate(data))

def encrypt_paths(paths: list[Path]) -> int:
    count = 0
    for path in paths:
        if not path.exists():
            print(f"[ERROR] Resource path not found: {path}")
            return -1

        sources = path.rglob("*") if path.is_dir() else [path]
        for src in sources:
            if not src.is_file() or src.suffix == ".pak":
                continue

            data = src.read_bytes()
            pak = Path(str(src) + ".pak")
            pak.write_bytes(xor_crypt(data))
            src.unlink()
            count += 1

    return count

def main():
    if len(sys.argv) < 2:
        print("Usage: encrypt_resources.py <resource_path> [<resource_path> ...]")
        sys.exit(1)

    paths = [Path(value) for value in sys.argv[1:]]
    n = encrypt_paths(paths)

    if n < 0:
        sys.exit(1)
    if n == 0:
        print("[WARN] No files encrypted.")
    else:
        print(f"[OK] Encrypted {n} file(s).")

if __name__ == "__main__":
    main()