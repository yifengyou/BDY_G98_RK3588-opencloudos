#!/bin/bash --norc
# SPDX-License-Identifier: GPL-2.0

set -eu

TOPDIR=$(git rev-parse --show-toplevel)
TEST_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/check-commits-test.XXXXXX")
TEST_REPO=$TEST_ROOT/repo

cleanup()
{
	rm -rf "$TEST_ROOT"
}
trap cleanup EXIT

mkdir -p "$TEST_REPO/dist/scripts" "$TEST_REPO/scripts"
cp "$TOPDIR/dist/scripts/check-commits.sh" "$TEST_REPO/dist/scripts/"
cp "$TOPDIR/dist/scripts/lib.sh" "$TEST_REPO/dist/scripts/"

cat > "$TEST_REPO/Kbuild" <<'EOF'
obj-y :=
EOF
cat > "$TEST_REPO/Makefile" <<'EOF'
VERSION = 6
EOF
cat > "$TEST_REPO/dist/Makefile" <<'EOF'
# Minimal dist configuration for check-commits tests.
DISTPATH = dist
VENDOR = tencentos
SPEC_ARCH := x86_64
EOF
cat > "$TEST_REPO/scripts/checkpatch.pl" <<'EOF'
#!/bin/sh
exit 0
EOF
chmod +x "$TEST_REPO/dist/scripts/check-commits.sh"
chmod +x "$TEST_REPO/scripts/checkpatch.pl"

git -C "$TEST_REPO" init -q
git -C "$TEST_REPO" config user.name "Test User"
git -C "$TEST_REPO" config user.email "test@example.com"
git -C "$TEST_REPO" add .
git -C "$TEST_REPO" commit -q -m "test: initial fixture"

create_fixture_commit()
{
	local name=$1

	echo "$name" > "$TEST_REPO/fixture"
	git -C "$TEST_REPO" add fixture
	git -C "$TEST_REPO" commit -q --signoff \
		-m "tencentos: $name" \
		-m "Upstream status: downstream-only" \
		-m "Checkpatch: no"
}

create_fixture_commit "fixture one"
create_fixture_commit "fixture two"

multi_output=$(cd "$TEST_REPO" && \
	./dist/scripts/check-commits.sh HEAD HEAD^)

if [ "$(grep -c '^=== Checking ' <<< "$multi_output")" -ne 2 ]; then
	echo "not ok - multiple revisions were not checked separately" >&2
	exit 1
fi
grep -q "tencentos: fixture one" <<< "$multi_output"
grep -q "tencentos: fixture two" <<< "$multi_output"

range_output=$(cd "$TEST_REPO" && \
	./dist/scripts/check-commits.sh HEAD~1..HEAD)

if [ "$(grep -c '^=== Checking ' <<< "$range_output")" -ne 1 ]; then
	echo "not ok - a single revision range changed behavior" >&2
	exit 1
fi
grep -q "tencentos: fixture two" <<< "$range_output"

echo "ok - check-commits preserves separate revision arguments"
