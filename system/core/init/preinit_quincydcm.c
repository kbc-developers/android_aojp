#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
#include <private/android_filesystem_config.h>
#include <sys/system_properties.h>

#include "preinit.h"


static const file_info const samsung_ics_file_list[] = {
    { "/init.qcom.rc", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.sh", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.usb.rc", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.usb.sh", 0750, AID_ROOT, AID_ROOT },
    { "/init.rc", 0750, AID_ROOT, AID_ROOT },
    { "/init.target.rc", 0750, AID_ROOT, AID_ROOT },
    { "/ueventd.rc", 0644, AID_ROOT, AID_ROOT },
};

static const rom_info const samsung_ics_info = {
    .base_dir = "/mbs/samsung-ics",
    .file_list = samsung_ics_file_list,
};

static const file_info const aosp_ics_file_list[] = {
    { "/init.emmc.rc", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.rc", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.sh", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.usb.rc", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.usb.sh", 0750, AID_ROOT, AID_ROOT },
    { "/init.rc", 0750, AID_ROOT, AID_ROOT },
    { "/init.target.rc", 0750, AID_ROOT, AID_ROOT },
    { "/ueventd.rc", 0644, AID_ROOT, AID_ROOT },
};

static const rom_info const aosp_ics_info = {
    .base_dir = "/mbs/aosp-ics",
    .file_list = aosp_ics_file_list,
};

#define PATH_SYSTEM      "/mbs/mnt/system"
#define PATH_DATA        "/mbs/mnt/data"
#define PART_NO_SYSTEM   (24)
#define PART_NO_DATA     (25)

void preinit(void)
{
    int feature_aosp;
    int rooted;
    char build_target[4] = { 0 };
    FILE* fp;
    const rom_info* rom_info_ptr = NULL;
    int i, file_list_size;

    uint32_t fs_type = check_external_sd_format();

    mount_partition(PART_NO_SYSTEM, PATH_SYSTEM);
    feature_aosp = (access(PATH_SYSTEM"/framework/com.mcafee.android.scanner.jar", R_OK) == 0) ? 0 : 1;
    if ((access(PATH_SYSTEM"/bin/su", R_OK) == 0)
    ||  (access(PATH_SYSTEM"/xbin/su", R_OK) == 0)) {
        ERROR("rom is rooted, remove internal su binary.\n");
        remove("mbs/bin/su");
    }
    umount_partition(PART_NO_SYSTEM, PATH_SYSTEM);

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
        ERROR("init to aosp-ics rom\n");
        rom_info_ptr = &aosp_ics_info;
        file_list_size = ARRAY_SIZE(aosp_ics_file_list);
    } else {
        ERROR("init to samsung-ics rom\n");
        rom_info_ptr = &samsung_ics_info;
        file_list_size = ARRAY_SIZE(samsung_ics_file_list);
    }

    setup_rom_file(rom_info_ptr, file_list_size);

    if (fs_type == FS_TYPE_LINUX) {
        setup_ext4sd();
    }

    //mount_partition(PART_NO_DATA, PATH_DATA);
    //setup_tweak_props(PATH_DATA"/tweakgnt.prop");
    //umount_partition(PART_NO_DATA, PATH_DATA);
}

