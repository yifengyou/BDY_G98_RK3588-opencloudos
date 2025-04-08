#ifndef __SXE_VER_H__
#define __SXE_VER_H__

#define SXE_VERSION                "1.5.0.19"
#define SXE_COMMIT_ID              "17beb85"
#define SXE_BRANCH                 "develop/rc/sagitta-1.5.0_B019"
#define SXE_BUILD_TIME             "2024-12-14 11:53:45"


#define SXE_DRV_NAME                   "sxe"
#define SXEVF_DRV_NAME                 "sxevf"
#define SXE_DRV_LICENSE                "GPL v2"
#define SXE_DRV_AUTHOR                 "sxe"
#define SXEVF_DRV_AUTHOR               "sxevf"
#define SXE_DRV_DESCRIPTION            "sxe driver"
#define SXEVF_DRV_DESCRIPTION          "sxevf driver"


#define SXE_FW_NAME                     "soc"
#define SXE_FW_ARCH                     "arm32"

#ifndef PS3_CFG_RELEASE
#define PS3_SXE_FW_BUILD_MODE             "debug"
#else
#define PS3_SXE_FW_BUILD_MODE             "release"
#endif

#endif
