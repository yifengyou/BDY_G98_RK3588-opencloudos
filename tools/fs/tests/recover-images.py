#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
"""Linux CLI regression using unmounted, disposable filesystem images.

Requires built recovery tools, gcc, e2fsprogs and xfsprogs. The fixtures keep
extent metadata intact while setting the file link count to zero. They test
remaining-metadata recovery, not the deletion policy of a mounted filesystem.
"""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import tempfile


def run(*args, cwd=None, env=None):
    return subprocess.run(args, cwd=cwd, env=env, check=True, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          timeout=60).stdout


def main():
    source = Path(__file__).resolve().parent
    fs = source.parent
    ext4 = fs / "ext4_recover/src/ext4recover"
    xfs = fs / "xfs_recover/recover/xfsrecover"
    with tempfile.TemporaryDirectory(prefix="recover-images-") as directory:
        root = Path(directory)
        pattern = root / "pattern"
        contents = b"".join(bytes([65 + i]) * 4096 if i % 2 == 0
                            else bytes(4096) for i in range(9))
        pattern.write_bytes(contents)
        ext_image, xfs_image = root / "ext4.img", root / "xfs.img"
        with ext_image.open("wb") as output:
            output.truncate(64 * 1024 * 1024)
        run("mkfs.ext4", "-q", "-F", "-b", "4096", str(ext_image))
        run("debugfs", "-w", "-R", f"write {pattern} /victim", str(ext_image))
        info = run("debugfs", "-R", "stat /victim", str(ext_image))
        assert "(ETB0)" in info, info
        ext_ino = re.search(r"Inode: (\d+)", info)[1]
        run("debugfs", "-w", "-R", "set_inode_field /victim links_count 0",
            str(ext_image))
        proto = root / "prototype"
        proto.write_text(f"dummy\n0 0\nd--755 0 0\nvictim ---644 0 0 {pattern}\n$\n")
        with xfs_image.open("wb") as output:
            output.truncate(400 * 1024 * 1024)
        run("mkfs.xfs", "-f", "-m", "crc=1,reflink=0,bigtime=0,inobtcount=0",
            "-i", "sparse=0", "-p", str(proto), str(xfs_image))
        info = run("xfs_db", "-r", "-c", "sb 0", "-c", "p rootino", str(xfs_image))
        rootino = re.search(r"rootino = (\d+)", info)[1]
        info = run("xfs_db", "-r", "-c", f"inode {rootino}", "-c", "ls", str(xfs_image))
        ino = re.search(r"^\d+\s+(\d+)\s+regular.*victim", info, re.M)[1]
        # mkfs leaves atime zero; the recovery scanner intentionally skips it.
        run("xfs_db", "-x", "-c", f"inode {ino}", "-c",
            "write core.atime.sec 1700000000", str(xfs_image))
        info = run("xfs_db", "-r", "-c", f"inode {ino}", "-c",
                   "p core.atime", str(xfs_image))
        if "1970" in info:  # Newer xfs_db writes the whole legacy timestamp.
            run("xfs_db", "-x", "-c", f"inode {ino}", "-c",
                f"write core.atime.sec {1700000000 << 32}", str(xfs_image))
        run("xfs_db", "-x", "-c", f"inode {ino}", "-c", "write core.nlinkv2 0",
            "-c", "write core.size 0", str(xfs_image))
        preload = root / "fault.so"
        run(os.environ.get("CC", "cc"), "-shared", "-fPIC", "-o", str(preload),
            str(source / "recover-fault.c"), "-ldl")
        modes = ("normal", "short", "read-eintr", "write-eintr", "read-error",
                 "write-error", "read-zero", "write-zero")
        for name, binary, image, inode, options in (
                ("ext4", ext4, ext_image, ext_ino, []),
                ("xfs", xfs, xfs_image, ino, ["-n"])):
            for mode in modes:
                destination = root / f"{name}-{mode}"
                destination.mkdir()
                env = dict(os.environ)
                if mode != "normal":
                    env.update(LD_PRELOAD=str(preload), RECOVER_FAULT=mode)
                result = subprocess.run([str(binary), *options, str(image)],
                                        cwd=destination, env=env, text=True,
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                        timeout=20)
                success = mode in modes[:4]
                assert (result.returncode == 0) == success, result.stdout
                output = destination / "RECOVER" / f"{inode}_file"
                if success:
                    assert output.read_bytes() == contents, (name, mode)
                else:
                    assert "Recovery incomplete!" in result.stdout, result.stdout
                    assert "Recover error:" in result.stdout, result.stdout
                    assert "Recover success!" not in result.stdout, result.stdout
                print(f"ok - {name} {mode} exit={result.returncode}")
        metadata = root / "xfs-metadata"
        metadata.mkdir()
        run(str(xfs), "-m", "-n", str(xfs_image), cwd=metadata)
        print("ok - xfs metadata-only scan")
        print("expected SHA256:", hashlib.sha256(contents).hexdigest())


if __name__ == "__main__":
    main()
