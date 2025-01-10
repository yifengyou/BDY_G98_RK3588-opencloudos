#! /bin/bash

# 驱动名称
DRIVER="sxevf"

# 内核版本
BUILD_KERNEL=$(uname -r)

# 参数获取
PARAM=$1

# 脚本所在路径
SCRIPT_PATH="$( cd "$( dirname "$0" )" >/dev/null 2>&1 && pwd )"

# 驱动安装路径
# Default to using updates/drivers/net/ethernet/intel/ path, since depmod since
# v3.1 defaults to checking updates folder first, and only checking kernels/
# and extra afterwards. We use updates instead of kernel/* due to desire to
# prevent over-writing built-in modules files.
INSTALL_MOD_DIR="updates/drivers/net/ethernet/Linkdata/${DRIVER}"

DRIVER_PATH="/lib/modules/${BUILD_KERNEL}/${INSTALL_MOD_DIR}/${DRIVER}.ko"
DRIVER_BAK="/lib/modules/${BUILD_KERNEL}/${INSTALL_MOD_DIR}/${DRIVER}.ko.bak"

# 帮助信息
help() {
    echo "  -h | --help           : help for this scripts"
    echo "  -i | --install        : compile & install ${DRIVER} driver"
    echo "  -a | --auto           : compile & install ${DRIVER} driver, then configure automatic loading at startup"
    echo "  -u | --uninstall      : uninstall driver ${DRIVER} from OS"
    echo "  -r | --unauto         : uninstall driver ${DRIVER} from OS, then remove automatic loading at startup"
}

# 检查环境信息
check_env() {
    echo "check build environment"

    # 1. 内核树检查
    ## ALL KERNEL SOURCE PATH
    KSP=("/lib/modules/${BUILD_KERNEL}/source" \
        "/lib/modules/${BUILD_KERNEL}/build")
    ## EXIST KERNEL SOURCE PATH
    EXIST_KSP=()
    for dir in ${KSP[*]}
    do
        if [[ -e ${dir}/include/linux ]]; then
            EXIST_KSP[${#EXIST_KSP[*]}]=${dir}
        fi
    done

    ## 取第一个path
    KSRC=${EXIST_KSP[0]}
    if [[ -z ${KSRC} ]]; then
        echo "error: Kernel header files not in any of the expected locations."
        echo "Install the appropriate kernel development package, e.g."
        echo "kernel-devel(redhat) or linux-headers(debian),"
        echo "for building kernel modules and try again"
        exit 1
    elif [ ${KSRC}="/lib/modules/${BUILD_KERNEL}/source" ]; then
        KOBJ="/lib/modules/${BUILD_KERNEL}/build"
    else
        KOBJ=${KSRC}
    fi

    # 2. 内核配置检查
    ## 2.1 内核版本文件
    ### Version file Search Path
    VSP=("${KOBJ}/include/generated/utsrelease.h" \
        "${KOBJ}/include/linux/utsrelease.h" \
        "${KOBJ}/include/linux/version.h" \
        "${KOBJ}/include/generated/uapi/linux/version.h" \
        "/boot/vmlinuz.version.h")
    EXIST_VSP=()
    ### prune the lists down to only files that exist
    for file in ${VSP[*]}
    do
        if [[ -f ${file} ]]; then
            EXIST_VSP[${#EXIST_VSP[*]}]=${file}
        fi
    done
    ### and use the first valid entry in the Search Paths
    VERSION_FILE=${EXIST_VSP[0]}
    if [[ ! -f ${VERSION_FILE} ]]; then
        echo "error: Linux kernel source not configured - missing version header file"
        exit 1
    fi

    ## 2.2 内核配置宏
    ### Config file Search Path
    CSP=("${KOBJ}/include/generated/autoconf.h" \
        "${KOBJ}/include/linux/autoconf.h" \
        "/boot/vmlinuz.autoconf.h")
    EXIST_CSP=()
    ### prune the lists down to only files that exist
    for file in ${CSP[*]}
    do
        if [[ -f ${file} ]]; then
            EXIST_CSP[${#EXIST_CSP[*]}]=${file}
        fi
    done
    ### and use the first valid entry in the Search Paths
    CONFIG_FILE=${EXIST_CSP[0]}
    if [[ ! -f ${CONFIG_FILE} ]]; then
        echo "error: Linux kernel source not configured - missing autoconf.h"
        exit 1
    fi

    ## 2.3 内核符号表
    ### System.map Search Path (for depmod)
    MSP=("${KSRC}/System.map" \
        "/usr/lib/debug/boot/System.map-${BUILD_KERNEL}" \
        "/boot/System.map-${BUILD_KERNEL}")
    EXIST_MSP=()
    ### prune the lists down to only files that exist
    for file in ${MSP[*]}
    do
        if [[ -f ${file} ]]; then
            EXIST_MSP[${#EXIST_MSP[*]}]=${file}
        fi
    done
    ### and use the first valid entry in the Search Paths
    SYSTEM_MAP_FILE=${EXIST_MSP[0]}
    if [[ ! -f ${SYSTEM_MAP_FILE} ]]; then
        echo "warn: Missing System.map file - depmod will not check for missing symbols during module installation"
    fi

    # 3. 已安装驱动程序检查
    ## 3.1 ko
    ko_installed=$(lsmod | grep -w ${DRIVER})
    if [[ -n ${ko_installed} ]]; then
        echo "error: driver ${DRIVER} installed, please exec 'rmmod ${DRIVER}' for uninstalling driver firstly"
        exit 1
    fi

    ## 3.2 rpm or dpkg
    # 这里通过检查 rpm 和 deb 工具是否存在来确定。若都没有则不检查
    if [[ -x /usr/bin/rpm ]]; then
        rpm_installed=$(rpm -qa | grep -w ${DRIVER})
        if [[ -n ${rpm_installed} ]]; then
            echo "error: driver ${DRIVER} installed, please exec 'rpm -e ${DRIVER}' for uninstalling driver firstly"
            exit 1
        fi
    elif [[ -x /usr/bin/dpkg ]]; then
        deb_installed=$(dpkg -l | grep -w ${DRIVER} | grep -w ii)
        if [[ -n ${deb_installed} ]]; then
            echo "error: driver ${DRIVER} installed, please exec 'dpkg -r ${DRIVER}' for uninstalling driver firstly"
            exit 1
        fi
    fi

    # 针对dca模块进行检查，如果内核定义成模块形式，则需要检测是否安装。若是模式为y或者未编译状态，则不做检查
    dca_conf=$(cat ${KOBJ}/include/config/auto.conf | grep -w CONFIG_DCA)
    if [[ -n ${dca_conf} ]];then
        dca_conf=${dca_conf#*=}
        if [[ ${dca_conf} = "m" ]];then
            dca_installed=$(lsmod | grep -w dca)
                if [[ -z ${dca_installed} ]]; then
                modprobe dca
                if [[ $? -ne 0 ]]; then
                    echo "error: installing the driver ${DRIVER} depends on the dca module."
                    echo "Needs to insmod dca first"
                    exit 1
                fi
            fi
        fi
    fi

    ptp_installed=$(lsmod | grep -w ptp)
    if [[ -z ${ptp_installed} ]]; then
        modprobe ptp
        if [[ $? -ne 0 ]]; then
            echo "error: installing the driver ${DRIVER} depends on the ptp module, which needs to be installed first"
            exit 1
        fi
    fi

    echo "check build environment pass!"
}

# 编译
compile() {
    echo "compile driver ${DRIVER}.ko"
    cd ${SCRIPT_PATH}
    # 清理编译残留
    make clean
    # 开始编译
    make
    if [[ $? -ne 0 ]]; then
        echo "error: compile failed"
        exit 1
    fi
    echo "compile driver ${DRIVER}.ko success!"
}

# 安装
install() {
    echo "install driver ${DRIVER}.ko"

    cd ${SCRIPT_PATH}
    insmod ${DRIVER}.ko
    if [[ $? -ne 0 ]]; then
        echo "error: compile failed"
        exit 1
    fi
    echo "install ${DRIVER} success!"
}

# 备份开机加载项中的原始ko
backup() {
    echo "backup ${DRIVER}.ko"
    if [[ -e ${DRIVER_PATH} ]]; then
        echo "backup"
        mv ${DRIVER_PATH} ${DRIVER_BAK}
    fi
}

# 删除备份
delbak() {
    echo "del ${DRIVER}.ko.bak"
    rm -f ${DRIVER_BAK}
}

# 备份恢复
restore() {
    if [[ -e ${DRIVER_PATH} ]]; then
        echo "rm ko"
        rm -f ${DRIVER_PATH}
    fi
    if [[ -e ${DRIVER_BAK} ]]; then
        echo "restore bak"
        mv ${DRIVER_BAK} ${DRIVER_PATH}
    fi
}

# 检查返回值，如果不为0，则进行文件回退
checkret() {
    if [[ $? -ne 0 ]]; then
        echo "error: check retrun failed, restore files"
        restore
        exit 1
    fi
}

# 配置开机自动加载
autoload() {
    echo "configure automatic loading at startup start"
    cd ${SCRIPT_PATH}
    # 创建安装路径
    if [[ ! -e /lib/modules/${BUILD_KERNEL}/${INSTALL_MOD_DIR} ]]; then
        mkdir -p /lib/modules/${BUILD_KERNEL}/${INSTALL_MOD_DIR}
    fi
    if [[ $? -ne 0 ]]; then
        echo "error: create install dir failed"
        exit 1
    fi

    # 备份原始文件
    backup

    # 拷贝驱动到安装路径
    cp -f ${DRIVER}.ko /lib/modules/${BUILD_KERNEL}/${INSTALL_MOD_DIR}
    if [[ $? -ne 0 ]]; then
        echo "error: copy driver failed"
        exit 1
    fi
    # 创建依赖关系
    /sbin/depmod
    # 检查返回值
    checkret

    # 重新生成img文件
    # 这里通过检查 dracut mkinitrd mkinitramfs 工具是否存在来确定能否继续执行
    if [[ -x /usr/bin/dracut ]]; then
        dracut -f -v
    elif [[ -x /usr/bin/mkinitrd ]]; then
        mkinitrd /boot/initramfs-${BUILD_KERNEL}.img ${BUILD_KERNEL} -v -f
    elif [[ -x /usr/sbin/mkinitramfs ]]; then
        mkinitramfs -o /boot/initrd.img-${BUILD_KERNEL}
    else
        # 备份恢复
        restore
        echo "error: configure automatic loading at startup failed! can not update initramfs"
        exit 0
    fi

    #检查返回值
    checkret

    # 删除备份文件
    delbak

    echo "configure automatic loading at startup success!"
}

# 卸载驱动
uninstall() {
    echo "uninstall driver ${DRIVER}.ko"
    ko_installed=$(lsmod | grep -w ${DRIVER})
    if [[ -z ${ko_installed} ]]; then
        echo "driver ${DRIVER} has already been uninstalled."
        return 0
    else
        rmmod ${DRIVER}
        echo "uninstall ${DRIVER} success!"
    fi
}

# 移除自动加载
unauto() {
    echo "remove automatic loading at startup start."
    rm -rf /lib/modules/${BUILD_KERNEL}/${INSTALL_MOD_DIR}
    # 创建依赖关系
    /sbin/depmod
    # 重新生成img文件
    # 这里通过检查 dracut mkinitrd mkinitramfs 工具是否存在来确定能否继续执行
    if [[ -x /usr/bin/dracut ]]; then
        dracut -f -v
    elif [[ -x /usr/bin/mkinitrd ]]; then
        mkinitrd /boot/initramfs-${BUILD_KERNEL}.img ${BUILD_KERNEL} -v -f
    elif [[ -x /usr/sbin/mkinitramfs ]]; then
        mkinitramfs -o /boot/initrd.img-${BUILD_KERNEL}
    else
        echo "error: remove automatic loading at startup failed! can not update initramfs"
        exit 0
    fi
    echo "remove automatic loading at startup success!"
}

if [[ $# -eq 0 || "${PARAM}" = "-h" || "${PARAM}" = "--help" ]]; then
    help
    exit 0
elif [[ "${PARAM}" = "-i" || "${PARAM}" = "--install" ]]; then
    check_env
    compile
    install
elif [[ "${PARAM}" = "-a" || "${PARAM}" = "--auto" ]]; then
    check_env
    compile
    install
    autoload
elif [[ "${PARAM}" = "-u" || "${PARAM}" = "--uninstall" ]]; then
    uninstall
elif [[ "${PARAM}" = "-r" || "${PARAM}" = "--unauto" ]]; then
    uninstall
    unauto
else
    echo "invalid param, please exec '$0 -h' or '$0 --help' for valid param"
fi
