#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Check whether TencentOS Kennel configs is compliant
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
	echo "  arch: x86 x86_64 arm64 loongarch"
}

function check_advice()
{
	local config_file=$1
	local config=$2
	local advice=$3
	local value=$4

	if [ "$value" == "n" ]; then
		label="# $config is not set"
	else
		label="$config=$value"
	fi
	grep -q "$label" "$config_file" && return

	real_info=$(grep -w "$config" "$config_file")
	case "$advice" in
		"Should")
			err_info=$(printf "[ERROR] %-64s %-64s" "$label" "$real_info")
			pr_err "$err_info"
			;;
		"Could")
			warn_info=$(printf "[ WARN] %-64s %-64s" "$label" "$real_info")
			pr_warn "$warn_info"
			;;
	esac
}

function check_config()
{
	local arch=$1
	local config_file=$2
	local standard_file=$3

	local temp_file=$(mktemp)
	if [[ $? -ne 0 ]]; then
		pr_err "create temporary file failed"
		return 1
	fi

	tail -n +2 ${standard_file} > ${temp_file}

	printf "        %-64s %-64s\n" "Recommend" "Real"
	while read -r id config advice_x86_64 advice_aarch64 advice_loongarch value_x86_64 value_aarch64 value_loongarch
	do
		case "$arch" in
			"x86_64")
				check_advice "$config_file" "$config" "$advice_x86_64" "$value_x86_64"
				;;
			"aarch64")
				check_advice "$config_file" "$config" "$advice_aarch64" "$value_aarch64"
				;;
			"loongarch64")
				check_advice "$config_file" "$config" "$advice_loongarch" "$value_loongarch"
				;;
		esac
	done < ${temp_file}

	rm -rf ${temp_file}
}

function generate_config()
{
	local arch=$1

	echo "start to generate configs..."
	rm -rf ./dist/rpm/SOURCES/*.config
	make ARCH=${arch} dist-config &>/dev/null
	if [ $? -ne 0 ]; then
		pr_err "generate dist-config failed"
		return 1
	fi

	return 0
}

function main()
{
	local arch=$1
	local std_configs_list="dist/configs/std-configs-list"

	if [ $# -ne 1 ]; then
		usage_info
		return 1
	fi

	case ${arch} in
		x86)
			arch=x86_64
			;;
		arm64)
			arch=aarch64
			;;
		loongarch)
			arch=loongarch64
			;;
		x86_64|aarch64|loongarch64)
			;;
		*)
			pr_err "not support arch:${arch}"
			return 1
			;;
	esac

	cd ${srctree}

	echo "arch: ${arch}"

	generate_config ${arch}
	if [ $? -ne 0 ]; then
		return 1
	fi

	check_config ${arch} .config ${std_configs_list}
	if [ $? -ne 0 ]; then
		return 1
	fi

	return 0
}

main $@
exit $?
