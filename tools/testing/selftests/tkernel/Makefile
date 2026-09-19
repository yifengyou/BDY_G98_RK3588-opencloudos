# SPDX-License-Identifier: GPL-2.0

TEST_GEN_FILES := netbind_test shield_mounts_test signal_test ttools_test \
	aegis_readerr_test
TEST_PROGS := netbind.sh killprotect.sh killblock.sh irqlatency.sh \
	memcg_async.sh shield_mounts.sh cgroupfs.sh ttools.sh \
	aegis_readerr.sh
TEST_FILES := tkernel.sh

TEST_GEN_PROGS += ttools_ioctl
TEST_PROGS += ttools_compat.sh
TEST_FILES += README.ttools-compat

include ../lib.mk

# Override CC32 for architectures that need a separate compat compiler.
CC32 ?= $(CC) -m32
EXTRA_CLEAN += $(OUTPUT)/ttools_ioctl_32
TTOOLS_CAN_BUILD_32 := $(shell \
	printf '\043include <unistd.h>\nint main(void) {\n%s\nreturn 0; }\n' \
	'_Static_assert(sizeof(long) == 4, "32-bit ABI");' | \
	$(CC32) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -x c - -o /dev/null >/dev/null 2>&1 && echo 1)

TTOOLS_HDRS := ../../../../kernel/tkernel/ttools/ttools.h ../kselftest.h
$(OUTPUT)/aegis_readerr_test: aegis_readerr_test.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wall -Wextra -o $@ $< $(LDFLAGS) $(LDLIBS)

$(OUTPUT)/ttools_ioctl: ttools_ioctl.c $(TTOOLS_HDRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wall -Wextra -o $@ $< $(LDFLAGS) $(LDLIBS)

ifeq ($(TTOOLS_CAN_BUILD_32),1)
TEST_GEN_FILES += $(OUTPUT)/ttools_ioctl_32
all: $(OUTPUT)/ttools_ioctl_32
$(OUTPUT)/ttools_ioctl_32: ttools_ioctl.c $(TTOOLS_HDRS)
	$(CC32) $(CPPFLAGS) $(CFLAGS) -Wall -Wextra -o $@ $< $(LDFLAGS) $(LDLIBS)
else
$(warning 32-bit ttools test unavailable; install multilib or set CC32)
endif
