#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

KSFT_SKIP=4
test_no=0
failures=0

ksft_skip_all()
{
	echo "1..0 # SKIP $1"
	exit "$KSFT_SKIP"
}

ksft_plan()
{
	echo "1..$1"
}

ksft_result()
{
	test_no=$((test_no + 1))
	if [ "$1" -eq 0 ]; then
		echo "ok $test_no - $2"
	else
		echo "not ok $test_no - $2"
		failures=$((failures + 1))
	fi
}

ksft_finished()
{
	[ "$failures" -eq 0 ]
}
