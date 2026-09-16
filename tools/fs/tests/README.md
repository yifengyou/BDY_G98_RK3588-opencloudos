# Recovery I/O tests

Build `ext4_recover/src` and `xfs_recover` first. The latter configures the
xfsprogs version pinned by its Makefile. On Linux, run from `tools/fs`:

```sh
sh tests/recover-io.sh
python3 tests/recover-images.py
```

The C tests include the production source files and exercise the actual copy
functions with ordinary temporary files. They check bytes, sparse prefixes,
lengths and failure returns for 1, 2 and 4 KiB blocks. `CPPFLAGS`, `CFLAGS`,
`CC` and `XFS_PROGS_DIR` may override build dependencies. Assertions must stay
enabled. Fault cases are bounded by a five-second alarm.

The CLI tests require Python 3, a C compiler, `mkfs.ext4`, `debugfs`, `mkfs.xfs`
and `xfs_db`. They create temporary images without mounting them, retain extent
metadata while clearing link counts, and verify recovery through both complete
programs. The preload helper limits transfers and injects faults only while a
recovery output is open, including fortified libc reads. Each invocation has a
20-second timeout. Temporary images occupy up to about 500 MiB and are removed
on exit. These tests do not change or read any host block device.
