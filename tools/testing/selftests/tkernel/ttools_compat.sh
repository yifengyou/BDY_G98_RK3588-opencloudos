#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

KSFT_SKIP=4
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
helper="$script_dir/ttools_ioctl_32"

skip_all()
{
	echo "TAP version 13"
	echo "1..0 # SKIP $1"
	exit "$KSFT_SKIP"
}

case "$(uname -m)" in
	*64*) ;;
	*) skip_all "a 64-bit kernel is required for compat testing" ;;
esac

[ -x "$helper" ] || skip_all \
	"32-bit helper unavailable; rebuild with a working CC32 toolchain"
"$helper"
result=$?
case "$result" in
	126|127) skip_all "32-bit execution or its runtime loader is unavailable" ;;
esac
exit "$result"
