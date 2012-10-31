#ifndef __PREINIT_H__
#define __PREINIT_H__


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

void mount_partition(int partNo, const char* mount_dir);
void umount_partition(int partNo, const char* mount_dir);

#define FS_TYPE_ERROR 0x00
#define FS_TYPE_FAT32 0x0C
#define FS_TYPE_LINUX 0x83

extern uint32_t check_external_sd_format(void);

extern void setup_tweak_props(const char* prop_file_path);

extern void setup_ext4sd(void);

extern void setup_rom_file(const rom_info* rom_info_ptr, int file_list_size);


#endif /* __PREINIT_H__ */

