#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <errno.h>
#include <stdarg.h>

#include "log.h"


#define ARRAY_SIZE(X) (sizeof(X)/(sizeof(X[0])))

typedef struct file_info {
    char* src_path;
    char* dst_path;
    mode_t mode;
} file_info;

static const file_info const samsung_file_list[] = {
    { "/mbs/samsung-ics/default.prop", "/default.prop", 0644 },
    { "/mbs/samsung-ics/init.bt.rc", "/init.bt.rc", 0750 },
    { "/mbs/samsung-ics/init.carrier.rc", "/init.carrier.rc", 0750 },
    { "/mbs/samsung-ics/init.qcom.rc", "/init.qcom.rc", 0750 },
    { "/mbs/samsung-ics/init.qcom.sh", "/init.qcom.sh", 0750 },
    { "/mbs/samsung-ics/init.qcom.usb.rc", "/init.qcom.usb.rc", 0750 },
    { "/mbs/samsung-ics/init.qcom.usb.sh", "/init.qcom.usb.sh", 0750 },
    { "/mbs/samsung-ics/init.rc", "/init.rc", 0750 },
    { "/mbs/samsung-ics/init.sensor.rc", "/init.sensor.rc", 0750 },
    { "/mbs/samsung-ics/init.target.rc", "/init.target.rc", 0750 },
    { "/mbs/samsung-ics/ueventd.rc", "/ueventd.rc", 0644 },
};

static const file_info const aosp_file_list[] = {
    { "/mbs/aosp-ics/default.prop", "/default.prop", 0644 },
    { "/mbs/aosp-ics/init.bt.rc", "/init.bt.rc", 0750 },
    { "/mbs/aosp-ics/init.emmc.rc", "/init.emmc.rc", 0750 },
    { "/mbs/aosp-ics/init.qcom.rc", "/init.qcom.rc", 0750 },
    { "/mbs/aosp-ics/init.qcom.sh", "/init.qcom.sh", 0750 },
    { "/mbs/aosp-ics/init.qcom.usb.rc", "/init.qcom.usb.rc", 0750 },
    { "/mbs/aosp-ics/init.qcom.usb.sh", "/init.qcom.usb.sh", 0750 },
    { "/mbs/aosp-ics/init.rc", "/init.rc", 0750 },
    { "/mbs/aosp-ics/init.sensor.rc", "/init.sensor.rc", 0750 },
    { "/mbs/aosp-ics/init.target.rc", "/init.target.rc", 0750 },
    { "/mbs/aosp-ics/ueventd.rc", "/ueventd.rc", 0644 },
};


void preinit(void)
{
    int feature_aosp;
    int rooted;
    char build_target[4] = { 0 };
    FILE* fp;
    const file_info* file_list_ptr = NULL;
    int file_list_size = 0;
    int i;

    mknod("/mbs/mmcblk0p14", S_IFBLK | 0666, makedev(179, 14));
    mount("/mbs/mmcblk0p14", "/mbs/mnt/system", "ext4", 0, NULL);
    feature_aosp = (access("/mbs/mnt/system/framework/twframework.jar", R_OK) == 0) ? 0 : 1;
    if ((access("/mbs/mnt/system/bin/su", R_OK) == 0)
    ||  (access("/mbs/mnt/system/xbin/su", R_OK) == 0)) {
        ERROR("rom is rooted, remove internal su binary.\n");
        remove("mbs/bin/su");
    }
    umount("/mbs/mnt/system");
    remove("/mbs/mmcblk0p14");

    fp = fopen("/proc/sys/kernel/build_target", "rb");
    if (fp) {
        fread(&build_target, 1, 4, fp);
        fclose(fp);
    }

    fp = fopen("/proc/sys/kernel/feature_aosp", "wb");
    if (fp) {
        fwrite(&feature_aosp, sizeof(int), 1, fp);
        fclose(fp);
    }
    ERROR("build_target=%c feature_aosp=%d\n", build_target[0], feature_aosp);

    if (feature_aosp) {
        ERROR("init to aosp rom\n");
        file_list_ptr = aosp_file_list;
        file_list_size = ARRAY_SIZE(aosp_file_list);
    } else {
        ERROR("init to samsung rom\n");
        file_list_ptr = samsung_file_list;
        file_list_size = ARRAY_SIZE(samsung_file_list);
    }

    for (i = 0; i < file_list_size; i++) {
        rename(file_list_ptr[i].src_path, file_list_ptr[i].dst_path);
        chmod(file_list_ptr[i].dst_path, file_list_ptr[i].mode);
    }
}

