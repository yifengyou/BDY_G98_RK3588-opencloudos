#!/bin/bash

thirdparty_mlnx(){
	mlnx_tgz_name=$(release-drivers/mlnx/get_mlnx_info.sh mlnx_tgz_name)
	mlnx_tgz_sha256=$(release-drivers/mlnx/get_mlnx_info.sh mlnx_tgz_sha256)

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

thirdparty_i40e(){
	if [ -e release-drivers/i40e ]; then
		rm -rf ../../drivers/net/ethernet/intel/i40e
		cp -a release-drivers/i40e ../../drivers/net/ethernet/intel/

		## Use sed to replace "I40E" with "I40E && !THIRDPARTY_I40E" in
		## drivers/infiniband/hw/irdma/Kconfig
		## Avoid compiling native kernel irdma when using thirdparty i40e due to compatibility concerns.
		sed -i 's/\(I40E\)/\1 \&\& !THIRDPARTY_I40E/g' ../../drivers/infiniband/hw/irdma/Kconfig
		echo "thirdparty_i40e: has overriden thirdparty i40e driver code to kernel native dir."
	fi
}

thirdparty_ice(){
	if [ -e release-drivers/ice ]; then
		rm -rf ../../drivers/net/ethernet/intel/ice
		cp -a release-drivers/ice ../../drivers/net/ethernet/intel/

		## Use sed to replace "ICE" with "ICE && !THIRDPARTY_ICE" in
		## drivers/infiniband/hw/irdma/Kconfig
		## Avoid compiling native kernel irdma when using thirdparty ice due to compatibility concerns.
		sed -i 's/\(ICE\)/\1 \&\& !THIRDPARTY_ICE/g' ../../drivers/infiniband/hw/irdma/Kconfig
		echo "thirdparty_ice: has overriden thirdparty ice driver code to kernel native dir."
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

thirdparty_i40e

thirdparty_ice
