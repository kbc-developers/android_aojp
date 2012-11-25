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

void mount_partition(int partNo, const char* mount_dir)
{
    char dev_name[100];
    sprintf(dev_name, "/mbs/mmcblk0p%d", partNo);
    mknod(dev_name, S_IFBLK | 0666, makedev(179, partNo));
    mount(dev_name, mount_dir, "ext4", 0, NULL);
}

void umount_partition(int partNo, const char* mount_dir)
{
    char dev_name[100];
    sprintf(dev_name, "/mbs/mmcblk0p%d", partNo);
    umount(mount_dir);
    remove(dev_name);
}

uint32_t check_external_sd_format(void)
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

void setup_tweak_props(const char* prop_file_path)
{
    char buf[300];
    int put_usb_config = 0;
    int i;
#if 0
    FILE* fp = fopen(prop_file_path, "r");
    if (fp == NULL) {
        ERROR("%s is not found.\n", prop_file_path);
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
        ERROR("key=%s value=%s\n", key, value);
        if (strcmp("ro.sys.usb.config", key) == 0) {
            put_usb_config = 1;
            property_set("persist.sys.usb.config", value);
        } else {
            property_set(key, value);
        }
    }

    if (put_usb_config == 0) {
        property_set("persist.sys.usb.config", "mtp,adb");
    }
    fclose(fp);
#endif
}

void setup_ext4sd(void)
{
    filerep("/init.rc",
        "#@ext4sd_mkdir",
        "    mkdir /mnt/ext4sd 0775 media_rw media_rw\n"
        "    chown media_rw media_rw /mnt/ext4sd");

    filerep("/init.qcom.rc",
        "#@ext4sd_service",
        "service ext4sd /sbin/ext4sd /mnt/ext4sd 1023 1023\n"
        "    class late_start\n"
        "    disabled");

    filerep("/init.emmc.rc",
        "#@ext4sd_mount",
        "    exec /system/bin/e2fsck -p /dev/block/mmcblk1p1\n"
        "    mount ext4 /dev/block/mmcblk1p1 /mnt/ext4sd nosuid nodev noatime barrier=1 discard noauto_da_alloc journal_async_commit");
}

void setup_rom_file(const rom_info* rom_info_ptr, int file_list_size)
{
    int i;

    for (i = 0; i < file_list_size; i++) {
        char src_path[100];
        char* dst_path = rom_info_ptr->file_list[i].file_path;
        sprintf(src_path, "%s%s", rom_info_ptr->base_dir, dst_path);

        rename(src_path, dst_path);
        chmod(dst_path, rom_info_ptr->file_list[i].mode);
        chown(dst_path, rom_info_ptr->file_list[i].uid, rom_info_ptr->file_list[i].gid);
    }
}


