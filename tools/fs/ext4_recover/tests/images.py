#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
"""Recover retained extent metadata from unmounted temporary ext4 images."""
import hashlib
from pathlib import Path
import re
import struct
import subprocess
import tempfile


def run(*args, cwd=None):
    return subprocess.run(args, cwd=cwd, check=True, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          timeout=60).stdout


def image_case(root, binary, bs, mode):
    case = root / f"{bs}-{mode}"
    case.mkdir()
    image, pattern = case / "fs.img", case / "pattern"
    capacity = (bs - 12) // 12
    extents = capacity + 5 if mode in ("multiple-leaves", "partial-leaf") else 5
    if mode == "depth-two":
        extents = 4 * capacity + 5
    contents = b"".join(bytes([65 + i % 26]) * bs if i % 2 == 0
                        else bytes(bs) for i in range(2 * extents - 1))
    pattern.write_bytes(contents)
    with image.open("wb") as output:
        output.truncate(64 * 1024 * 1024)
    # Disable inode/extent checksums only for the explicit cleared-root fixture.
    options = ["-O", "^metadata_csum"] if mode in ("cleared-root", "partial-leaf") else []
    run("mkfs.ext4", "-q", "-F", "-b", str(bs), *options, str(image))
    run("debugfs", "-w", "-R", f"write {pattern} /victim", str(image))
    if mode == "unwritten":
        run("debugfs", "-w", "-R", "fallocate /victim 1 1", str(image))
        run("debugfs", "-w", "-R", "fallocate /victim 10 12", str(image))
        contents += bytes(4 * bs)
    info = run("debugfs", "-R", "stat /victim", str(image))
    assert "(ETB0)" in info, info
    inode = re.search(r"Inode: (\d+)", info)[1]
    if mode == "multiple-leaves":
        assert info.count("(ETB0)") > 1, info
    if mode == "depth-two":
        assert "(ETB1)" in info, info
    if mode == "unwritten":
        assert "[u]" in info, info
    run("debugfs", "-w", "-R", "set_inode_field /victim links_count 0", str(image))
    if mode in ("cleared-root", "partial-leaf"):
        info = run("debugfs", "-R", f"imap <{inode}>", str(image))
        match = re.search(r"block (\d+), offset (0x[0-9a-f]+)", info)
        assert match, info
        offset = int(match[1]) * bs + int(match[2], 16) + 40
        with image.open("r+b") as output:
            if mode == "cleared-root":
                output.seek(offset + 2)  # inode.i_block extent header entries
                output.write(struct.pack("<H", 0))
                output.seek(offset + 6)  # retain index slots, clear depth
                output.write(struct.pack("<H", 0))
            else:
                # Move the first leaf's last record to the second leaf. Both
                # leaves remain valid, but the first is deliberately not full.
                output.seek(offset)
                tree = bytearray(output.read(60))
                assert struct.unpack_from("<H", tree, 6)[0] == 1
                leaves = []
                for index in (12, 24):
                    logical, low, high = struct.unpack_from("<IIH", tree, index)
                    block = low | (high << 32)
                    output.seek(block * bs)
                    leaves.append((block, bytearray(output.read(bs))))
                first, second = leaves[0][1], leaves[1][1]
                n1, n2 = (struct.unpack_from("<H", leaf, 2)[0]
                          for leaf in (first, second))
                assert n1 > 1 and n2 < capacity
                record = first[12 * n1:12 * (n1 + 1)]
                second[24:24 + n2 * 12] = second[12:12 + n2 * 12]
                second[12:24] = record
                struct.pack_into("<H", first, 2, n1 - 1)
                struct.pack_into("<H", second, 2, n2 + 1)
                tree[24:28] = record[:4]
                for block, leaf in leaves:
                    output.seek(block * bs)
                    output.write(leaf)
                output.seek(offset)
                output.write(tree)
    result = run(str(binary), str(image), cwd=case)
    actual = (case / "RECOVER" / f"{inode}_file").read_bytes()
    assert actual == contents, (bs, mode, len(actual), len(contents), result)
    print(f"ok - {bs} {mode} bytes={len(actual)} "
          f"sha256={hashlib.sha256(actual).hexdigest()}")


def main():
    binary = Path(__file__).resolve().parents[1] / "src/ext4recover"
    with tempfile.TemporaryDirectory(prefix="ext4-extents-") as directory:
        for bs in (1024, 2048, 4096):
            for mode in ("initialized", "multiple-leaves", "partial-leaf",
                         "depth-two", "unwritten", "cleared-root"):
                image_case(Path(directory), binary, bs, mode)
    print("ok - 18 ext4 image recovery cases")


if __name__ == "__main__":
    main()
