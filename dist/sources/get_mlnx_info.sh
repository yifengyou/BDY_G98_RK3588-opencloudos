#!/bin/bash

mlnx_version="24.10-3.2.5.0"

mlnx_tgz_name="MLNX_OFED_LINUX-${mlnx_version}-rhel9.4-x86_64.tgz"
mlnx_tgz_sha256="a04cba2bd669a95a5e61699da8b6c201474b3bb44c1e3379d383fd81acbdd8b9"

if [[ $1 == mlnx_url0 ]]; then
	echo "https://content.mellanox.com/ofed/MLNX_OFED-${mlnx_version}/${mlnx_tgz_name}"
	exit 0
elif [[ $1 == mlnx_url1 ]]; then
	part1="https://mirror"
	part2="s.te"
	part3="nt.c"
	part4="om/os/tlinux_unified/drivers-src/mlnx/"
	echo "${part1}${part2}nce${part3}${part4}${mlnx_tgz_name}"
	exit 0
elif [[ $1 == mlnx_version ]]; then
	echo ${mlnx_version}
	exit 0
elif [[ $1 == mlnx_tgz_name ]]; then
	echo ${mlnx_tgz_name}
	exit 0
elif [[ $1 == mlnx_tgz_sha256 ]]; then
	echo ${mlnx_tgz_sha256}
	exit 0
elif [[ $1 == mlnx_check_sha256 ]]; then
    sha256_tmp=$(sha256sum ${mlnx_tgz_name} | awk '{printf $1}')
    if [[ $sha256_tmp == $mlnx_tgz_sha256 ]]; then
        exit 0
    else
        exit 1
    fi
else
	echo "Error: wrong parameter for get_mlnx_info.sh!"
	exit 1
fi
