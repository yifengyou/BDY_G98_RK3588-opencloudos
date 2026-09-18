# Extent format regression tests

On Linux, build the tool in `../src`, then run:

```sh
sh extents.sh
python3 images.py
```

The C test includes the production source and requires the same libext2fs and
libcom_err development files as the tool. `CC`, `CPPFLAGS`, `CFLAGS` and
`LDFLAGS` may select private build dependencies; assertions must remain enabled.
It verifies 1/2/4 KiB block geometry, encoded lengths 1, 0x7fff, 0x8000, 0x8001
and 0xffff, mixed initialized/unwritten ranges, sparse gaps and failure returns.
Unwritten tests deliberately supply an invalid source descriptor to verify
that no physical data is copied. Tests use ordinary temporary files, including
a 128 MiB initialized extent, with an alarm around recovery.

The image tests require Python 3, `mkfs.ext4` and `debugfs`. They create
temporary, unmounted 64 MiB images for each block size, exercising multiple
leaf blocks, partially filled children, a depth-two tree, unwritten extents
and a cleared inode root with residual indexes.
Link counts are cleared while external extent metadata remains intact; this
models the tool's recovery input, not the deletion behavior of every mounted
filesystem. No host block devices are accessed. Images are removed on exit.

The recovered output represents the remaining extent map: initialized extents
copy physical blocks; unwritten extents read as zero, including trailing
unwritten ranges. The output ends at the highest recovered extent boundary.
The original byte length may have been cleared on deletion and is not inferred
from stale inode size fields. Existing partial-block size limitations remain.
