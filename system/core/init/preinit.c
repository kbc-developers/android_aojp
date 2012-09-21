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

#define ARRAY_SIZE(X) (sizeof(X)/(sizeof(X[0])))

typedef struct file_info {
    char* file_path;
    mode_t mode;
    unsigned uid;
    unsigned gid;
} file_info;

typedef struct rom_info {
   const char* base_dir;
   const file_info* file_list;
} rom_info;


const file_info const samsung_file_list[] = {
    { "/default.prop", 0644, AID_ROOT, AID_ROOT },
};

static const rom_info const samsung_info = {
    .base_dir = "/mbs/samsung",
    .file_list = samsung_file_list,
};

const file_info const aosp_jb_file_list[] = {
    { "/fstab.qcom", 0640, AID_ROOT, AID_ROOT },
    { "/init.bt.rc", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.class_core.sh", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.class_main.sh", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.early_boot.sh", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.rc", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.sh", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.syspart_fixup.sh", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.usb.rc", 0750, AID_ROOT, AID_ROOT },
    { "/init.qcom.usb.sh", 0750, AID_ROOT, AID_ROOT },
    { "/init.rc", 0750, AID_ROOT, AID_ROOT },
    { "/init.target.rc", 0750, AID_ROOT, AID_ROOT },
    { "/init.trace.rc", 0750, AID_ROOT, AID_ROOT },
    { "/init.usb.rc", 0750, AID_ROOT, AID_ROOT },
    { "/ueventd.qcom.rc", 0644, AID_ROOT, AID_ROOT },
    { "/ueventd.rc", 0644, AID_ROOT, AID_ROOT },
};

static const rom_info const aosp_jb_info = {
    .base_dir = "/mbs/aosp-jb",
    .file_list = aosp_jb_file_list,
};

#define PATH_SYSTEM      "/mbs/mnt/system"
#define PATH_DATA        "/mbs/mnt/data"
#define PART_NO_SYSTEM   (14)
#define PART_NO_DATA     (15)

static void mount_partition(int partNo)
{
    switch (partNo) {
    case PART_NO_SYSTEM:
        mknod("/mbs/mmcblk0p14", S_IFBLK | 0666, makedev(179, 14));
        mount("/mbs/mmcblk0p14", PATH_SYSTEM, "ext4", 0, NULL);
        break;
    case PART_NO_DATA:
        mknod("/mbs/mmcblk0p15", S_IFBLK | 0666, makedev(179, 15));
        mount("/mbs/mmcblk0p15", PATH_DATA, "ext4", 0, NULL);
        break;
    }
}

static void umount_partition(int partNo)
{
    switch (partNo) {
    case PART_NO_SYSTEM:
        umount(PATH_SYSTEM);
        remove("/mbs/mmcblk0p14");
        break;
    case PART_NO_DATA:
        umount(PATH_DATA);
        remove("/mbs/mmcblk0p15");
        break;
    }
}

#define FS_TYPE_ERROR 0x00
#define FS_TYPE_FAT32 0x0C
#define FS_TYPE_LINUX 0x83

static uint32_t check_external_sd_format(void)
{
    int fd;
    uint8_t mbuffer[512];
    uint32_t fs_type = FS_TYPE_ERROR;

    mknod("/mbs/mmcblk1", S_IFBLK | 0666, makedev(179, 96));

    fd = open("/mbs/mmcblk1", O_RDONLY);
    if (512 != read(fd, mbuffer, 512)) {
        ERROR("/mbs/mmcblk1 open error");
        return fs_type;
    }

    // Valid MBR Signature
    if (mbuffer[0x01FE] != 0x55 || mbuffer[0x01FF] != 0xaa) {
        ERROR("invalid partition table flag\n");
        goto l_end;
    }

    // Dump Partition Table1
    ERROR("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
        mbuffer[0x01BE], mbuffer[0x01BF], mbuffer[0x01C0], mbuffer[0x01C1],
        mbuffer[0x01C2], mbuffer[0x01C3], mbuffer[0x01C4], mbuffer[0x01C5],
        mbuffer[0x01C6], mbuffer[0x01C7], mbuffer[0x01C8], mbuffer[0x01C9],
        mbuffer[0x01CA], mbuffer[0x01CB], mbuffer[0x01CC], mbuffer[0x01CD]);

    if (mbuffer[0x01C2] == FS_TYPE_LINUX) {
        ERROR("external sd format is Linux");
    } else if (mbuffer[0x01C2] == FS_TYPE_FAT32) {
        ERROR("external sd format is Win95 FAT32 (LBA)");
    } else {
        ERROR("external sd format is unknown(0x%02x)", mbuffer[0x01C2]);
    }
    fs_type = (unsigned int)mbuffer[0x01C2];

l_end:
    close(fd);
    remove("/mbs/mmcblk1");
    return fs_type;
}

static void setup_tweak_props(void)
{
    char buf[300];
    int put_usb_config = 0;
    int i;

    FILE* fp = fopen(PATH_DATA"/tweakgs3.prop", "r");
    if (fp == NULL) {
        ERROR("tweakgs3.prop is not found.\n");
        return;
    }

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        char* key = strtok(buf, "=");
        char* value = strtok(NULL, "=");
        if (key == NULL || value == NULL) {
            continue;
        }
        i = strlen(value);
        value[i > 0 ? i - 1 : 0] = '\0';
        
        if (strcmp("ro.sys.usb.config", key) == 0) {
            put_usb_config = 1;
        }
        property_set(key, value);
    }

    if (put_usb_config == 0) {
        property_set("persist.sys.usb.config", "mtp,adb");
    }

    fclose(fp);
}

static void strrep(char *buf, const char *target, const char *replace)
{
    char *find;
    size_t targetLen = strlen(target);
    size_t replaceLen = strlen(replace);

    if (targetLen == 0) {
        return;
    }

    while ((find = strstr(buf, target)) != NULL) {
        memmove(find + replaceLen, find + targetLen, strlen(buf) - (find + targetLen - buf ) + 1);
        memcpy(find, replace, replaceLen);
    }
}

static void filerep(char* filePath, const char* target, const char* replace)
{
    char* buf;
    FILE* fp;
    fpos_t fsize;

    fp = fopen(filePath, "r");
    if (fp == NULL) {
        return;
    }

    fseek(fp, 0, SEEK_END);
    fgetpos(fp, &fsize);
    fseek(fp, 0, SEEK_SET);

    buf = malloc(fsize * 2);
    memset(buf, 0, fsize * 2);

    if (fsize != (fpos_t)fread(buf, sizeof(char), fsize, fp)) {
        return;
    }
    fclose(fp);

    strrep(buf, target, replace);

    fsize = strlen(buf);
    fp = fopen(filePath, "w");
    fwrite(buf, sizeof(char), fsize, fp);
    fclose(fp);

    free(buf);
}

static void fileadd(char* filePath, const char* add)
{
    FILE* fp = fopen(filePath, "a+");
    if (fp == NULL) {
        ERROR("%s is not found.\n", filePath);
        return;
    }
    fputs(add, fp);

    fclose(fp);
}

void setup_ext4sd(void)
{
    filerep("/init.rc",
        "#@ext4sd_mkdir",
        "    mkdir /mnt/ext4sd 0775 media_rw media_rw\n    "
        "chown media_rw media_rw /mnt/ext4sd");

    filerep("/init.qcom.rc",
        "#@ext4sd_service",
        "service ext4sd /sbin/sdcard1 /mnt/ext4sd 1023 1023\n    "
        "class late_start");

    fileadd("/fstab.qcom",
        "/dev/block/mmcblk1p1                                    "
        "/mnt/ext4sd         "
        "ext4      "
        "noatime,nosuid,nodev,barrier=1,discard,noauto_da_alloc,journal_async_commit               "
        "wait,check,encryptable=footer");
}

void preinit(void)
{
    int feature_aosp;
    int rooted;
    char build_target[4] = { 0 };
    FILE* fp;
    const rom_info* rom_info_ptr = NULL;
    int i, file_list_size;

    uint32_t fs_type = check_external_sd_format();

    mount_partition(PART_NO_SYSTEM);
    feature_aosp = (access(PATH_SYSTEM"/framework/twframework.jar", R_OK) == 0) ? 0 : 1;
    if ((access(PATH_SYSTEM"/bin/su", R_OK) == 0)
    ||  (access(PATH_SYSTEM"/xbin/su", R_OK) == 0)) {
        ERROR("rom is rooted, remove internal su binary.\n");
        remove("mbs/bin/su");
    }
    umount_partition(PART_NO_SYSTEM);

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
        rom_info_ptr = &aosp_jb_info;
        file_list_size = ARRAY_SIZE(aosp_jb_file_list);
    } else {
        ERROR("init to samsung rom\n");
        rom_info_ptr = &samsung_info;
        file_list_size = ARRAY_SIZE(aosp_jb_file_list);
    }

    
    for (i = 0; i < file_list_size; i++) {
        char src_path[100];
        char* dst_path = rom_info_ptr->file_list[i].file_path;
        sprintf(src_path, "%s%s", rom_info_ptr->base_dir, dst_path);

        rename(src_path, dst_path);
        chmod(dst_path, rom_info_ptr->file_list[i].mode);
        chown(dst_path, rom_info_ptr->file_list[i].uid, rom_info_ptr->file_list[i].gid);
    }

    if (fs_type == FS_TYPE_LINUX) {
        setup_ext4sd();
    }

    mount_partition(PART_NO_DATA);
    setup_tweak_props();
    umount_partition(PART_NO_DATA);
}

