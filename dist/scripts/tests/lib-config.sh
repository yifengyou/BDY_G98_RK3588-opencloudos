#!/bin/bash --norc
# SPDX-License-Identifier: GPL-2.0

set -euo pipefail

SOURCE_TOPDIR=$(git rev-parse --show-toplevel)
TEST_TREE=$(mktemp -d)
trap 'rm -rf "$TEST_TREE"' EXIT

mkdir -p "$TEST_TREE/dist/scripts" "$TEST_TREE/dist/workdir/config_cache"
cp "$SOURCE_TOPDIR/dist/scripts/lib.sh" "$TEST_TREE/dist/scripts/"
cp "$SOURCE_TOPDIR/dist/scripts/lib-config.sh" "$TEST_TREE/dist/scripts/"

touch "$TEST_TREE/Kbuild"
printf 'VERSION = 6\n' > "$TEST_TREE/Makefile"
printf 'DISTPATH = dist\nSPEC_ARCH := x86_64\nVENDOR = opencloudos\n' \
	> "$TEST_TREE/dist/Makefile"

cat > "$TEST_TREE/.config" <<'EOF'
CONFIG_FOO=128
CONFIG_BAR=1024
CONFIG_BAZ=0x100
CONFIG_64BIT=y
EOF

cat > "$TEST_TREE/dist/workdir/config_cache/TEST_NUMERIC_LITERALS" <<'EOF'
depends on FOO > 64 && BAR = 1024 && BAZ = 0x100
EOF

cat > "$TEST_TREE/dist/scripts/test-lib-config.sh" <<'EOF'
#!/bin/bash --norc

set -eo pipefail

cd "$(dirname "$(realpath "$0")")/../.."
# shellcheck source=lib-config.sh
. ./dist/scripts/lib-config.sh

assert_eq() {
	local expected=$1 actual=$2

	if [[ $actual != "$expected" ]]; then
		echo "expected '$expected', got '$actual'" >&2
		exit 1
	fi
}

for literal in y m n 9 64 1024 0x100 0XFF; do
	assert_eq "$literal" "$(get_config_val "$literal" .config)"
done
assert_eq y "$(get_config_val 64BIT .config)"
assert_eq 'not set' "$(get_config_val UNKNOWN .config)"

depends=$(get_all_depends TEST_NUMERIC_LITERALS .config)
for expected in 'FOO[=128]' '64[=64]' 'BAR[=1024]' '1024[=1024]' \
	'BAZ[=0x100]' '0x100[=0x100]'; do
	if [[ " $depends " != *" $expected "* ]]; then
		echo "missing '$expected' in dependency output: $depends" >&2
		exit 1
	fi
done
if [[ $depends == *'[=not set]'* ]]; then
	echo "numeric literal was treated as a config symbol: $depends" >&2
	exit 1
fi
EOF

chmod +x "$TEST_TREE/dist/scripts/test-lib-config.sh"
git -C "$TEST_TREE" init -q
"$TEST_TREE/dist/scripts/test-lib-config.sh"

echo 'ok - Kconfig tristates and numeric literals are preserved'
