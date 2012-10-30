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

static const file_info const semc_file_list[] = {
    { "/mbs/semc-ics/default.prop", "/default.prop", 0644 },
    { "/mbs/semc-ics/init.semc.rc", "/init.semc.rc", 0750 },
    { "/mbs/semc-ics/init.rc", "/init.rc", 0750 },
    { "/mbs/semc-ics/ueventd.rc", "/ueventd.rc", 0644 },
    { "/mbs/semc-ics/ueventd.semc.rc", "/ueventd.semc.rc", 0644 },
};

static const file_info const aosp_file_list[] = {
    { "/mbs/semc-ics/default.prop", "/default.prop", 0644 },
    { "/mbs/semc-ics/init.semc.rc", "/init.semc.rc", 0750 },
    { "/mbs/semc-ics/init.rc", "/init.rc", 0750 },
    { "/mbs/semc-ics/ueventd.rc", "/ueventd.rc", 0644 },
    { "/mbs/semc-ics/ueventd.semc.rc", "/ueventd.semc.rc", 0644 },
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

    mknod("/mbs/mtdblock0", S_IFBLK | 0666, makedev(31, 0));
    mount("/mbs/mtdblock0", "/mbs/mnt/system", "yaffs2", 0, NULL);
    feature_aosp = (access("/mbs/mnt/system/framework/SemcGenericUxpRes.apk", R_OK) == 0) ? 0 : 1;
    if ((access("/mbs/mnt/system/bin/su", R_OK) == 0)
    ||  (access("/mbs/mnt/system/xbin/su", R_OK) == 0)) {
        ERROR("rom is rooted, remove internal su binary.\n");
        remove("mbs/bin/su");
    }
    umount("/mbs/mnt/system");
    remove("/mbs/mtdblock0");

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
        ERROR("init to semc rom\n");
        file_list_ptr = semc_file_list;
        file_list_size = ARRAY_SIZE(semc_file_list);
    }

    for (i = 0; i < file_list_size; i++) {
        rename(file_list_ptr[i].src_path, file_list_ptr[i].dst_path);
        chmod(file_list_ptr[i].dst_path, file_list_ptr[i].mode);
    }
}

