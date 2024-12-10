#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Check whether TencentOS Kernel KAPI is compliant
#

srctree=$(dirname "$0")/../

RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[0;33m"
NO_COLOR="\033[0m"

function pr_err()
{
	printf "${RED}$1${NO_COLOR}\n"
}

function pr_info()
{
	printf "${GREEN}$1${NO_COLOR}\n"
}

function pr_warn()
{
	printf "${YELLOW}$1${NO_COLOR}\n"
}

function usage_info()
{
	echo "Usage: $0 <arch>"
	echo "  arch: x86 x86_64 arm64"
}

function compile_kernel()
{
	local machine=$1
	local arch=$2
	local thread_num=$(nproc)
	thread_num=$((thread_num * 2))

	if [ "${machine}" == "x86_64" ] && [ "${arch}" == "arm64" ] && [ "X$CROSS_COMPILE" == "X" ]; then
		local CROSS_COMPILE="aarch64-linux-gnu-"
	fi

	make ARCH=${arch} CROSS_COMPILE=${CROSS_COMPILE} tencentconfig -sk 1> /dev/null
	if [ $? -ne 0 ]; then
		pr_err "make tencentconfig failed"
		return 1
	fi

	echo "start to compile kernel, may take a long time..."
	make ARCH=${arch} CROSS_COMPILE=${CROSS_COMPILE} -j ${thread_num} -sk 1> /dev/null
	if [ $? -ne 0 ]; then
		pr_err "compile kernel failed"
		return 1
	fi

	return 0
}

function check_kapi()
{
	# XXX: The Ruyi config needs to be closed to compile normally.
	./scripts/config -m CONFIG_TKERNEL_CHECK_KAPI -d CONFIG_RUE
	if [ $? -ne 0 ]; then
		pr_err "enable CONFIG_TKERNEL_CHECK_KAPI failed"
		return 1
	fi

	make ARCH=${arch} CROSS_COMPILE=${CROSS_COMPILE} M=kernel/tkernel/checkkapi modules
	if [ $? -ne 0 ]; then
		pr_err "check KAPI failed"
		return 1
	fi
	pr_info "check KAPI success"

	return 0
}

function check_param()
{
	local arch=$1
	local machine=$(uname -m)

	if [ "$#" -ne 1 ]; then
		usage_info
		return 1
	fi

	if [ "${machine}" != "x86_64" ] && [ "${machine}" != "aarch64" ]; then
		pr_err "not support machine:${machine}"
		usage_info
		return 1
	fi

	if [ "${arch}" != "x86" ] && [ "${arch}" != "x86_64" ] && [ "${arch}" != "arm64" ]; then
		pr_err "not support arch:${arch}"
		usage_info
		return 1
	fi

	if [ "${machine}" == "aarch64" ] && [ "${arch}" == "x86" -o "${arch}" == "x86_64" ]; then
		pr_err "machine aarch64 not support cross compile"
		usage_info
		return 1
	fi

	return 0
}

function main()
{
	local arch=$1
	local machine=$(uname -m)

	check_param $@
	if [ $? -ne 0 ]; then
		return 1
	fi

	cd ${srctree}

	echo "arch: ${arch}"
	echo "machine: ${machine}"

	compile_kernel ${machine} ${arch}
	if [ $? -ne 0 ]; then
		return 1
	fi

	check_kapi
	if [ $? -ne 0 ]; then
		return 1
	fi

	return 0
}

main $@
exit $?
