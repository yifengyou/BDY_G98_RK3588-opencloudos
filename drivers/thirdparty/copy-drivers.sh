#!/bin/bash

thirdparty_mlnx(){
	mlnx_tgz_name=$(./get_mlnx_info.sh mlnx_tgz_name)
	mlnx_tgz_sha256=$(./get_mlnx_info.sh mlnx_tgz_sha256)

	if [ ! -e release-drivers/mlnx/${mlnx_tgz_name} ] ; then
		./download-and-copy-drivers.sh
		if (( $? != 0 )) ; then exit 1 ; fi
		mv ${mlnx_tgz_name} release-drivers/mlnx/
	else
		sha256_tmp=$(sha256sum release-drivers/mlnx/${mlnx_tgz_name} | awk '{printf $1}')
		if [[ $sha256_tmp != $mlnx_tgz_sha256 ]]; then
			echo "Warning: release-drivers/mlnx/${mlnx_tgz_name} is exist, but sha256sum is not correct!"
			./download-and-copy-drivers.sh
			if (( $? != 0 )) ; then exit 1 ; fi
			mv ${mlnx_tgz_name} release-drivers/mlnx/
		fi
	fi
}

thirdparty_bnxt(){
	if [ -e release-drivers/bnxt ]; then
		rm -rf ../../drivers/net/ethernet/broadcom/bnxt
		cp -a release-drivers/bnxt ../../drivers/net/ethernet/broadcom/

		## Use sed to replace "&& BNXT" with "&& BNXT && !THIRDPARTY_BNXT" in
		## drivers/infiniband/hw/bnxt_re/Kconfig
		## Because compile kernel native bnxt_re will fail when using thirdparty bnxt.
		sed -i 's/\(&& BNXT\)$/\1 \&\& !THIRDPARTY_BNXT/g' ../../drivers/infiniband/hw/bnxt_re/Kconfig
		echo "thirdparty_bnxt: has overriden thirdparty bnxt driver code to kernel native dir."
	fi
}

thirdparty_mpt3sas(){
	if [ -e release-drivers/mpt3sas ]; then
		rm -rf ../../drivers/scsi/mpt3sas
		cp -a release-drivers/mpt3sas ../../drivers/scsi/
		sed -i 's/---help---/help/g' ../../drivers/scsi/mpt3sas/Kconfig
	fi
}

thirdparty_megaraid_sas(){
	if [ -e release-drivers/megaraid_sas ]; then
		rm -rf ../../drivers/scsi/megaraid_sas
		cp -a release-drivers/megaraid_sas ../../drivers/scsi/
		sed -i 's/megaraid\//megaraid_sas\//g' ../../drivers/scsi/Makefile
	fi
}

thirdparty_mpi3mr(){
	if [ -e release-drivers/mpi3mr ]; then
		rm -rf ../../drivers/scsi/mpi3mr
		cp -a release-drivers/mpi3mr ../../drivers/scsi/
	fi
}

##
## main , script start run at here.
##
if [[ $1 != without_mlnx ]]; then
	thirdparty_mlnx
fi

thirdparty_bnxt

thirdparty_mpt3sas

thirdparty_megaraid_sas

thirdparty_mpi3mr
