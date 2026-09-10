#!/bin/bash --norc
# SPDX-License-Identifier: GPL-2.0

set -eu

SOURCE_TOPDIR=${SOURCE_TOPDIR:-$(git rev-parse --show-toplevel)}
TEST_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/config-errors.XXXXXX")
TEST_TREE=$TEST_ROOT/tree
trap 'rm -rf "$TEST_ROOT"' EXIT

mkdir -p "$TEST_TREE/dist/scripts" "$TEST_TREE/dist/workdir" \
	"$TEST_TREE/dist/rpm/SOURCES" "$TEST_TREE/bin" "$TEST_TREE/scripts"
for script in lib.sh lib-config.sh gen-configs.sh check-configs.sh; do
	cp "$SOURCE_TOPDIR/dist/scripts/$script" "$TEST_TREE/dist/scripts/"
done
touch "$TEST_TREE/Kbuild"
printf 'VERSION = 6\nPATCHLEVEL = 6\nSUBLEVEL = 0\n' > "$TEST_TREE/Makefile"
printf 'DISTPATH = dist\nSPEC_ARCH := x86_64 aarch64\nVENDOR = opencloudos\n' \
	> "$TEST_TREE/dist/Makefile"
printf 'config FOO\n\tbool\n' > "$TEST_TREE/Kconfig"
printf '#!/bin/sh\nexit 0\n' > "$TEST_TREE/scripts/diffconfig"
chmod +x "$TEST_TREE/scripts/diffconfig"
for target in a b c; do
	mkdir -p "$TEST_TREE/dist/configs/00base/$target"
	printf 'CONFIG_LOCALVERSION=""\nCONFIG_FOO=y\n' \
		> "$TEST_TREE/dist/configs/00base/$target/default.config"
done

cat > "$TEST_TREE/bin/make" <<'EOF'
#!/bin/bash
count=$(cat "$CALL_COUNT")
count=$((count + 1))
echo "$count" > "$CALL_COUNT"
if [[ $count -eq $FAIL_AT ]]; then
	printf '%s' "${FAIL_OUTPUT:-}"
	echo 'injected make failure' >&2
	exit 23
fi
exit 0
EOF
chmod +x "$TEST_TREE/bin/make"

# No errexit here: the library must propagate failures itself, including
# when a caller explicitly checks the status of a function.
cat > "$TEST_TREE/dist/scripts/driver.sh" <<'EOF'
#!/bin/bash
. "$(dirname "$0")/lib-config.sh"
count=0
callback() {
	count=$((count + 1))
	echo "$count" > "$CALL_COUNT"
	if [[ $count -eq $FAIL_AT ]]; then
		return 29
	fi
	return 0
}
case $1 in
target|product)
	mode=$1; shift
	for_each_config_"$mode" callback "$@"
	;;
make)
	mkdir -p "$TOPDIR/subdir"
	cd "$TOPDIR/subdir" || exit 1
	before=$PWD
	if config_make x86_64 olddefconfig; then
		ret=0
	else
		ret=$?
	fi
	[[ $PWD == "$before" ]] || exit 97
	exit "$ret"
	;;
missing)
	CONFIG_OUTDIR=$TOPDIR/missing
	makedef_configs a
	;;
populate)
	# A directory in place of a fragment makes cat fail, even as root.
	rm "$CONFIG_PATH/00base/a/default.config"
	mkdir "$CONFIG_PATH/00base/a/default.config"
	populate_configs a
	;;
shell-options)
	set +o pipefail
	populate_configs a || exit $?
	[[ $(set -o | awk '$1 == "pipefail" { print $2 }') == off ]]
	;;
esac
EOF

export TOPDIR=$TEST_TREE DISTDIR=$TEST_TREE/dist
export PATH=$TEST_TREE/bin:$PATH
export CALL_COUNT=$TEST_ROOT/calls FAIL_AT=0 FAIL_OUTPUT=
checks=0

run_status() {
	local expected=$1
	shift
	echo 0 > "$CALL_COUNT"
	status=0
	bash "$TEST_TREE/dist/scripts/$1" "${@:2}" \
		> "$TEST_ROOT/stdout" 2> "$TEST_ROOT/stderr" || status=$?
	if [[ $status -ne $expected ]]; then
		cat "$TEST_ROOT/stdout" "$TEST_ROOT/stderr" >&2
		echo "not ok - $*: expected status $expected, got $status" >&2
		exit 1
	fi
	checks=$((checks + 1))
}

assert_calls() {
	[[ $(cat "$CALL_COUNT") -eq $1 ]] || {
		echo "not ok - expected $1 calls, got $(cat "$CALL_COUNT")" >&2
		exit 1
	}
}

# Generation and both checks stop at the first failed make, preserving its
# status instead of reporting successful generation or checking.
for fail_at in 1 3 6; do
	export FAIL_AT=$fail_at
	run_status 23 gen-configs.sh
	assert_calls "$fail_at"
	if grep -q '^Checking ' "$TEST_ROOT/stdout"; then
		echo 'not ok - sanity checking ran after failed generation' >&2
		exit 1
	fi
	run_status 23 check-configs.sh check-new-configs
	assert_calls "$fail_at"
	clear_count=$(grep -c 'Config is all clear' "$TEST_ROOT/stdout" || :)
	[[ $clear_count -eq $((fail_at - 1)) ]]
	run_status 23 check-configs.sh check-diff-configs
	assert_calls "$fail_at"
done

for fail_at in 1 2 3; do
	export FAIL_AT=$fail_at
	run_status 29 driver.sh target
	assert_calls "$fail_at"
done
for fail_at in 1 3 6; do
	export FAIL_AT=$fail_at
	run_status 29 driver.sh product
	assert_calls "$fail_at"
done

export FAIL_AT=1
run_status 23 driver.sh make
assert_calls 1
run_status 1 driver.sh missing
assert_calls 0

# Failed listnewconfig must not append even plausible config output. Keep
# an explicit matching filter: --autofix parsing is handled separately.
cp "$TEST_TREE/dist/configs/00base/a/default.config" "$TEST_ROOT/before"
export FAIL_OUTPUT='CONFIG_INJECTED=y'
run_status 23 check-configs.sh check-new-configs --autofix a
assert_calls 1
cmp "$TEST_ROOT/before" "$TEST_TREE/dist/configs/00base/a/default.config"
if grep -q 'Config is all clear' "$TEST_ROOT/stdout"; then
	echo 'not ok - failed listnewconfig reported all clear' >&2
	exit 1
fi

export FAIL_AT=0 FAIL_OUTPUT=
run_status 0 driver.sh make
assert_calls 1
run_status 0 gen-configs.sh
assert_calls 6
run_status 0 check-configs.sh check-new-configs
assert_calls 6
run_status 0 gen-configs.sh b
assert_calls 2
run_status 0 gen-configs.sh no-match
assert_calls 0
run_status 0 driver.sh target b
assert_calls 1
run_status 0 driver.sh product b
assert_calls 2
run_status 0 driver.sh target no-match
assert_calls 0
run_status 0 driver.sh product no-match
assert_calls 0
run_status 0 driver.sh shell-options

# Output creation failures must stop before make, regardless of privileges.
mkdir "$TEST_TREE/dist/rpm/SOURCES/a.x86_64.config.blocked"
mv "$TEST_TREE/dist/rpm/SOURCES/a.x86_64.config" "$TEST_ROOT/product"
mv "$TEST_TREE/dist/rpm/SOURCES/a.x86_64.config.blocked" \
	"$TEST_TREE/dist/rpm/SOURCES/a.x86_64.config"
run_status 1 gen-configs.sh a
assert_calls 0
rmdir "$TEST_TREE/dist/rpm/SOURCES/a.x86_64.config"
mv "$TEST_ROOT/product" "$TEST_TREE/dist/rpm/SOURCES/a.x86_64.config"
run_status 1 driver.sh populate
assert_calls 0

echo "ok - $checks configuration failure and success cases"
