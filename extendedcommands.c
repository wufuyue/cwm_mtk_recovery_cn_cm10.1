#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "cutils/android_reboot.h"
#include <sys/wait.h>
#include <sys/limits.h>
#include <dirent.h>
#include <sys/stat.h>

#include <signal.h>
#include <sys/wait.h>

#include "bootloader.h"
#include "common.h"
#include "cutils/properties.h"
#include "firmware.h"
#include "cutils/android_reboot.h"
#include "install.h"
#include "make_ext4fs.h"
#include "minui/minui.h"
#include "minzip/DirUtil.h"
#include "roots.h"
#include "recovery_ui.h"
#include "adb_install.h"
#include "minadbd/adb.h"

#include "extendedcommands.h"
#include "nandroid.h"
#include "mounts.h"
#include "flashutils/flashutils.h"
#include "edify/expr.h"
#include <libgen.h>
#include "mtdutils/mtdutils.h"
#include "bmlutils/bmlutils.h"
#include "cutils/android_reboot.h"


int signature_check_enabled = 1;
int script_assert_enabled = 1;
static const char *SDCARD_UPDATE_FILE = "/sdcard/update.zip";
static const char *SDCARD_GUOHOW_FILE = "/sdcard/update.zip";


int
get_filtered_menu_selection(char** headers, char** items, int menu_only, int initial_selection, int items_count) {
    int index;
    int offset = 0;
    int* translate_table = (int*)malloc(sizeof(int) * items_count);
    for (index = 0; index < items_count; index++) {
        if (items[index] == NULL)
            continue;
        char *item = items[index];
        items[index] = NULL;
        items[offset] = item;
        translate_table[offset] = index;
        offset++;
    }
    items[offset] = NULL;

    initial_selection = translate_table[initial_selection];
    int ret = get_menu_selection(headers, items, menu_only, initial_selection);
    if (ret < 0 || ret >= offset) {
        free(translate_table);
        return ret;
    }

    ret = translate_table[ret];
    free(translate_table);
    return ret;
}

void write_string_to_file(const char* filename, const char* string) {
    ensure_path_mounted(filename);
    char tmp[PATH_MAX];
    sprintf(tmp, "mkdir -p $(dirname %s)", filename);
    __system(tmp);
    FILE *file = fopen(filename, "w");
    if( file != NULL) {
        fprintf(file, "%s", string);
        fclose(file);
    }
}

void write_recovery_version() {
    if ( is_data_media() ) {
        write_string_to_file("/sdcard/0/clockworkmod/.recovery_version",EXPAND(RECOVERY_VERSION) "\n" EXPAND(TARGET_DEVICE));
    }
    write_string_to_file("/sdcard/clockworkmod/.recovery_version",EXPAND(RECOVERY_VERSION) "\n" EXPAND(TARGET_DEVICE));
}

void
toggle_signature_check()
{
    signature_check_enabled = !signature_check_enabled;
     ui_print("签名校验: %s\n", signature_check_enabled ? "启用" : "禁用");
}

int install_zip(const char* packagefilepath)
{
    ui_print("\n-- 正在安装...: %s\n", packagefilepath);
    if (device_flash_type() == MTD) {
        set_sdcard_update_bootloader_message();
    }
    int status = install_package(packagefilepath);
    ui_reset_progress();
    if (status != INSTALL_SUCCESS) {
        ui_set_background(BACKGROUND_ICON_ERROR);
        ui_print("刷机失败.\n");
        return 1;
    }
    ui_set_background(BACKGROUND_ICON_NONE);
    ui_print("\n安装刷机包成功！\n");
    return 0;
}

// help
void show_guohowhelp_menu()
{
   static char* headers[] = {  "               一键刷机说明：",
                                 "\n",
                                 "\n",
                                 "---------------------------------------------------",
                                "一键刷机系统将自动为你清空数据",
                                "刷机之前你应该手动将update.zip复制到SD卡",
                                "请确认外部SD卡存在 update.zip，然后再继续",
                                "本功能仅限测试交流使用，不按要求操作造成资料",
                                "损失本人概不负责,感谢使用！",
                                "\n",
                                "\n",
                                "----------------跃跃制作 sjrom.cn------------------",
                                "\n",
                                "\n",
                                NULL
    };
    
    char* install_menu_items[] = {  "继续，请返回",
                                                    NULL };
                                                    
    int chosen_item = get_menu_selection(headers, install_menu_items, 0, 0);
        return chosen_item == 0;
        
        switch (chosen_item)
        case 0:
        {
           ui_print("\n --感谢阅读--\n");
        break;     
        }                                          
  
}

// 一键刷机
#define ITEM_NONONONO       0
#define ITEM_GFLASH       1
#define ITEM_NONONO       2
// 定义格式化函数
//static long tmplog_offset = 0;
static int
erase_volume(const char *volume) {
    ui_set_background(BACKGROUND_ICON_INSTALLING);
    ui_show_indeterminate_progress();
    ui_print("正在格式化 %s...\n", volume);

    if (strcmp(volume, "/cache") == 0) {
        // Any part of the log we'd copied to cache is now gone.
        // Reset the pointer so we copy from the beginning of the temp
        // log.
     //   tmplog_offset = 0;
    }

    return format_volume(volume);
}

// 实现recovery.c中选择函数
void show_guohowflash_menu()
{
/*    static char* headers[] = {  "帮助： 系统将自动为你清空数据",
                                "刷机之前你应该手动将update.zip复制到SD卡",
                                "请确认外部SD卡存在 upddate.zip，然后再继续",
                                "\n",
                                "\n",
                                "---------------------------------------------------",
                                NULL
    };
    
                                                                                     ui_print("-----测试-----\n");
    
    char* install_menu_items[] = {  "否",
                                                   "是，我要一键刷机",  //(4);
                                                    NULL,
                                                    NULL };
  
        int chosen_item = get_menu_selection(headers, install_menu_items, 0, 0);
        return chosen_item == 1;
        
        switch (chosen_item)
        {
            if (chosen_item = 1) {
            
            */
            
         // 此处原为刷写函数
                
               //}
            
        //}

 
   ui_print("开始一键刷机....\n");
            ui_print("第一步\n");
            
            ui_print("\n-- 清除数据中...\n");
            device_wipe_data();
            erase_volume("/data");
            erase_volume("/cache");
            if (has_datadata()) {
            erase_volume("/datadata");
              }
            erase_volume("/sd-ext");
            erase_volume("/sdcard/.android_secure");
            ui_print("已经清除数据.\n");
            ui_print("第二步\n");
            ui_print("开始解压文件到系统中....\n");
            install_zip(SDCARD_GUOHOW_FILE);
           //android_reboot(ANDROID_RB_RESTART, 0, 0);
          //  break;
            
          
            
    
}







// 

#define ITEM_CHOOSE_ZIP       0
#define ITEM_APPLY_SDCARD     1
#define ITEM_SIG_CHECK        2
#define ITEM_CHOOSE_ZIP_INT   3

void show_install_update_menu()
{
    static char* headers[] = {  "从SD卡选择ZIP格式刷机包",
                                "",
                                NULL
    };
    
    char* install_menu_items[] = {  "从SD卡中选择一个ZIP格式刷机包",
                                    "直接应用/sdcard/update.zip刷机",
                                    "启用/禁用签名校验",
                                    NULL,
                                    NULL };

    char *other_sd = NULL;
    if (volume_for_path("/emmc") != NULL) {
        other_sd = "/emmc/";
       install_menu_items[3] = "从内置SD卡选择ZIP文件";
    }
    else if (volume_for_path("/external_sd") != NULL) {
        other_sd = "/external_sd/";
        install_menu_items[3] = "从外置SD卡选择ZIP文件";
    }
    
    for (;;)
    {
        int chosen_item = get_menu_selection(headers, install_menu_items, 0, 0);
        switch (chosen_item)
        {
            case ITEM_SIG_CHECK:
                toggle_signature_check();
                break;
            case ITEM_APPLY_SDCARD:
            {
                if (confirm_selection("确认安装?", "安装/sdcard/update.zip"))
                    install_zip(SDCARD_UPDATE_FILE);
                break;
            }
            case ITEM_CHOOSE_ZIP:
                show_choose_zip_menu("/sdcard/");
                write_recovery_version();
                break;
            case ITEM_CHOOSE_ZIP_INT:
                if (other_sd != NULL)
                    show_choose_zip_menu(other_sd);
                break;
            default:
                return;
        }

    }
}

void free_string_array(char** array)
{
    if (array == NULL)
        return;
    char* cursor = array[0];
    int i = 0;
    while (cursor != NULL)
    {
        free(cursor);
        cursor = array[++i];
    }
    free(array);
}

char** gather_files(const char* directory, const char* fileExtensionOrDirectory, int* numFiles)
{
    char path[PATH_MAX] = "";
    DIR *dir;
    struct dirent *de;
    int total = 0;
    int i;
    char** files = NULL;
    int pass;
    *numFiles = 0;
    int dirLen = strlen(directory);

    dir = opendir(directory);
    if (dir == NULL) {
        ui_print("打开目录失败.\n");
        return NULL;
    }

    int extension_length = 0;
    if (fileExtensionOrDirectory != NULL)
        extension_length = strlen(fileExtensionOrDirectory);

    int isCounting = 1;
    i = 0;
    for (pass = 0; pass < 2; pass++) {
        while ((de=readdir(dir)) != NULL) {
            // skip hidden files
            if (de->d_name[0] == '.')
                continue;

            // NULL means that we are gathering directories, so skip this
            if (fileExtensionOrDirectory != NULL)
            {
                // make sure that we can have the desired extension (prevent seg fault)
                if (strlen(de->d_name) < extension_length)
                    continue;
                // compare the extension
                if (strcmp(de->d_name + strlen(de->d_name) - extension_length, fileExtensionOrDirectory) != 0)
                    continue;
            }
            else
            {
                struct stat info;
                char fullFileName[PATH_MAX];
                strcpy(fullFileName, directory);
                strcat(fullFileName, de->d_name);
                lstat(fullFileName, &info);
                // make sure it is a directory
                if (!(S_ISDIR(info.st_mode)))
                    continue;
            }

            if (pass == 0)
            {
                total++;
                continue;
            }

            files[i] = (char*) malloc(dirLen + strlen(de->d_name) + 2);
            strcpy(files[i], directory);
            strcat(files[i], de->d_name);
            if (fileExtensionOrDirectory == NULL)
                strcat(files[i], "/");
            i++;
        }
        if (pass == 1)
            break;
        if (total == 0)
            break;
        rewinddir(dir);
        *numFiles = total;
        files = (char**) malloc((total+1)*sizeof(char*));
        files[total]=NULL;
    }

    if(closedir(dir) < 0) {
        LOGE("关闭目录失败.");
    }

    if (total==0) {
        return NULL;
    }

    // sort the result
    if (files != NULL) {
        for (i = 0; i < total; i++) {
            int curMax = -1;
            int j;
            for (j = 0; j < total - i; j++) {
                if (curMax == -1 || strcmp(files[curMax], files[j]) < 0)
                    curMax = j;
            }
            char* temp = files[curMax];
            files[curMax] = files[total - i - 1];
            files[total - i - 1] = temp;
        }
    }

    return files;
}

// pass in NULL for fileExtensionOrDirectory and you will get a directory chooser
char* choose_file_menu(const char* directory, const char* fileExtensionOrDirectory, const char* headers[])
{
    char path[PATH_MAX] = "";
    DIR *dir;
    struct dirent *de;
    int numFiles = 0;
    int numDirs = 0;
    int i;
    char* return_value = NULL;
    int dir_len = strlen(directory);

    i = 0;
    while (headers[i]) {
        i++;
    }
    const char** fixed_headers = (const char*)malloc((i + 3) * sizeof(char*));
    i = 0;
    while (headers[i]) {
        fixed_headers[i] = headers[i];
        i++;
    }
    fixed_headers[i] = directory;
    fixed_headers[i + 1] = "";
    fixed_headers[i + 2 ] = NULL;

    char** files = gather_files(directory, fileExtensionOrDirectory, &numFiles);
    char** dirs = NULL;
    if (fileExtensionOrDirectory != NULL)
        dirs = gather_files(directory, NULL, &numDirs);
    int total = numDirs + numFiles;
    if (total == 0)
    {
        ui_print("没有找到可用文件.\n");
    }
    else
    {
        char** list = (char**) malloc((total + 1) * sizeof(char*));
        list[total] = NULL;


        for (i = 0 ; i < numDirs; i++)
        {
            list[i] = strdup(dirs[i] + dir_len);
        }

        for (i = 0 ; i < numFiles; i++)
        {
            list[numDirs + i] = strdup(files[i] + dir_len);
        }

        for (;;)
        {
            int chosen_item = get_menu_selection(fixed_headers, list, 0, 0);
            if (chosen_item == GO_BACK)
                break;
            static char ret[PATH_MAX];
            if (chosen_item < numDirs)
            {
                char* subret = choose_file_menu(dirs[chosen_item], fileExtensionOrDirectory, headers);
                if (subret != NULL)
                {
                    strcpy(ret, subret);
                    return_value = ret;
                    break;
                }
                continue;
            }
            strcpy(ret, files[chosen_item - numDirs]);
            return_value = ret;
            break;
        }
        free_string_array(list);
    }

    free_string_array(files);
    free_string_array(dirs);
    free(fixed_headers);
    return return_value;
}

void show_choose_zip_menu(const char *mount_point)
{
    if (ensure_path_mounted(mount_point) != 0) {
        LOGE ("Can't mount %s\n", mount_point);
        return;
    }

    static char* headers[] = {  "选择一个ZIP格式刷机包",
                                "",
                                NULL
    };

    char* file = choose_file_menu(mount_point, ".zip", headers);
    if (file == NULL)
        return;
    static char* confirm_install  = "确认安装？";
    static char confirm[PATH_MAX];
    sprintf(confirm, "是-安装%s", basename(file));
    if (confirm_selection(confirm_install, confirm))
        install_zip(file);
}

void show_nandroid_restore_menu(const char* path)
{
    if (ensure_path_mounted(path) != 0) {
        LOGE("不能挂载 %s\n", path);
        return;
    }

    static char* headers[] = {  "选择恢复镜像",
                                "",
                                NULL
    };

    char tmp[PATH_MAX];
    sprintf(tmp, "%s/clockworkmod/backup/", path);
    char* file = choose_file_menu(tmp, NULL, headers);
    if (file == NULL)
        return;

    if (confirm_selection("确认恢复?", "恢复"))
        nandroid_restore(file, 1, 1, 1, 1, 1, 0);
}

void show_nandroid_delete_menu(const char* path)
{
    if (ensure_path_mounted(path) != 0) {
        LOGE("不能挂载 %s\n", path);
        return;
    }

    static char* headers[] = {  "选择要删除的镜像",
                                "",
                                NULL
    };

    char tmp[PATH_MAX];
    sprintf(tmp, "%s/clockworkmod/backup/", path);
    char* file = choose_file_menu(tmp, NULL, headers);
    if (file == NULL)
        return;

    if (confirm_selection("确认删除?", "是 - 删除")) {
        // nandroid_restore(file, 1, 1, 1, 1, 1, 0);
        sprintf(tmp, "rm -rf %s", file);
        __system(tmp);
    }
}

#define MAX_NUM_USB_VOLUMES 3
#define LUN_FILE_EXPANDS    2

struct lun_node {
    const char *lun_file;
    struct lun_node *next;
};

static struct lun_node *lun_head = NULL;
static struct lun_node *lun_tail = NULL;

int control_usb_storage_set_lun(Volume* vol, bool enable, const char *lun_file) {
    const char *vol_device = enable ? vol->device : "";
    int fd;
    struct lun_node *node;

    // Verify that we have not already used this LUN file
    for(node = lun_head; node; node = node->next) {
        if (!strcmp(node->lun_file, lun_file)) {
            // Skip any LUN files that are already in use
            return -1;
        }
    }

    // Open a handle to the LUN file
    LOGI("Trying %s on LUN file %s\n", vol->device, lun_file);
    if ((fd = open(lun_file, O_WRONLY)) < 0) {
        LOGW("无法打开 ums lunfile %s (%s)\n", lun_file, strerror(errno));
        return -1;
    }

    // Write the volume path to the LUN file
    if ((write(fd, vol_device, strlen(vol_device) + 1) < 0) &&
       (!enable || !vol->device2 || (write(fd, vol->device2, strlen(vol->device2)) < 0))) {
        LOGW("无法写入 ums lunfile %s (%s)\n", lun_file, strerror(errno));
        close(fd);
        return -1;
    } else {
        // Volume path to LUN association succeeded
        close(fd);

        // Save off a record of this lun_file being in use now
        node = (struct lun_node *)malloc(sizeof(struct lun_node));
        node->lun_file = strdup(lun_file);
        node->next = NULL;
        if (lun_head == NULL)
           lun_head = lun_tail = node;
        else {
           lun_tail->next = node;
           lun_tail = node;
        }

        LOGI("成功 %s共享了 %s ，--通过： LUN file %s\n", enable ? "" : "un", vol->device, lun_file);
        return 0;
    }
}

int control_usb_storage_for_lun(Volume* vol, bool enable) {
    static const char* lun_files[] = {
#ifdef BOARD_UMS_LUNFILE
        BOARD_UMS_LUNFILE,
#endif
#ifdef TARGET_USE_CUSTOM_LUN_FILE_PATH
        TARGET_USE_CUSTOM_LUN_FILE_PATH,
#endif
        "/sys/devices/platform/usb_mass_storage/lun%d/file",
        "/sys/class/android_usb/android0/f_mass_storage/lun/file",
        "/sys/class/android_usb/android0/f_mass_storage/lun_ex/file",
        NULL
    };

    // If recovery.fstab specifies a LUN file, use it
    if (vol->lun) {
        return control_usb_storage_set_lun(vol, enable, vol->lun);
    }

    // Try to find a LUN for this volume
    //   - iterate through the lun file paths
    //   - expand any %d by LUN_FILE_EXPANDS
    int lun_num = 0;
    int i;
    for(i = 0; lun_files[i]; i++) {
        const char *lun_file = lun_files[i];
        for(lun_num = 0; lun_num < LUN_FILE_EXPANDS; lun_num++) {
            char formatted_lun_file[255];
    
            // Replace %d with the LUN number
            bzero(formatted_lun_file, 255);
            snprintf(formatted_lun_file, 254, lun_file, lun_num);
    
            // Attempt to use the LUN file
            if (control_usb_storage_set_lun(vol, enable, formatted_lun_file) == 0) {
                return 0;
            }
        }
    }

    // All LUNs were exhausted and none worked
    LOGW("未能 %s打开 %s 目标： LUN %d\n", enable ? "en" : "dis", vol->device, lun_num);

    return -1;  // -1 failure, 0 success
}

int control_usb_storage(Volume **volumes, bool enable) {
    int res = -1;
    int i;
    for(i = 0; i < MAX_NUM_USB_VOLUMES; i++) {
        Volume *volume = volumes[i];
        if (volume) {
            int vol_res = control_usb_storage_for_lun(volume, enable);
            if (vol_res == 0) res = 0; // if any one path succeeds, we return success
        }
    }

    // Release memory used by the LUN file linked list
    struct lun_node *node = lun_head;
    while(node) {
       struct lun_node *next = node->next;
       free((void *)node->lun_file);
       free(node);
       node = next;
    }
    lun_head = lun_tail = NULL;

    return res;  // -1 failure, 0 success
}

void show_mount_usb_storage_menu()
{
    // Build a list of Volume objects; some or all may not be valid
    Volume* volumes[MAX_NUM_USB_VOLUMES] = {
        volume_for_path("/sdcard"),
        volume_for_path("/emmc"),
        volume_for_path("/external_sd")
    };

    // Enable USB storage
    if (control_usb_storage(volumes, 1))
        return;

    static char* headers[] = {  "U盘模式",
                                "返回上级菜单将自动弹出U盘",
                                "",
                                "",
                                NULL
    };

    static char* list[] = { "卸载", NULL };

    for (;;)
    {
        int chosen_item = get_menu_selection(headers, list, 0, 0);
        if (chosen_item == GO_BACK || chosen_item == 0)
            break;
    }

    // Disable USB storage
    control_usb_storage(volumes, 0);
}

int confirm_selection(const char* title, const char* confirm)
{
    struct stat info;
    if (0 == stat("/sdcard/clockworkmod/.no_confirm", &info))
        return 1;

    char* confirm_headers[]  = {  title, "  该操作无法撤销.", "", NULL };
    int one_confirm = 0 == stat("/sdcard/clockworkmod/.one_confirm", &info);
#ifdef BOARD_TOUCH_RECOVERY
    one_confirm = 1;
#endif 
    if (one_confirm) {
        char* items[] = { "No",
                        confirm, //" Yes -- wipe partition",   // [1]
                        NULL };
        int chosen_item = get_menu_selection(confirm_headers, items, 0, 0);
        return chosen_item == 1;
    }
    else {
        char* items[] = { "否",
                        confirm, //" Yes -- wipe partition",   // [7]
                        NULL };
        int chosen_item = get_menu_selection(confirm_headers, items, 0, 0);
        return chosen_item == 1;
    }
    }

#define MKE2FS_BIN      "/sbin/mke2fs"
#define TUNE2FS_BIN     "/sbin/tune2fs"
#define E2FSCK_BIN      "/sbin/e2fsck"

extern struct selabel_handle *sehandle;
int format_device(const char *device, const char *path, const char *fs_type) {
    Volume* v = volume_for_path(path);
    if (v == NULL) {
        // silent failure for sd-ext
        if (strcmp(path, "/sd-ext") == 0)
            return -1;
        LOGE("未知存储器 \"%s\"\n", path);
        return -1;
    }
    if (is_data_media_volume_path(path)) {
        return format_unknown_device(NULL, path, NULL);
    }
    if (strstr(path, "/data") == path && is_data_media()) {
        return format_unknown_device(NULL, path, NULL);
    }
    if (strcmp(fs_type, "ramdisk") == 0) {
        // you can't format the ramdisk.
        LOGE("未能格式化 \"%s\"", path);
        return -1;
    }

    if (strcmp(fs_type, "rfs") == 0) {
        if (ensure_path_unmounted(path) != 0) {
            LOGE("format_volume failed to unmount \"%s\"\n", v->mount_point);
            return -1;
        }
        if (0 != format_rfs_device(device, path)) {
            LOGE("format_volume: format_rfs_device failed on %s\n", device);
            return -1;
        }
        return 0;
    }
 
    if (strcmp(v->mount_point, path) != 0) {
        return format_unknown_device(v->device, path, NULL);
    }

    if (ensure_path_unmounted(path) != 0) {
        LOGE("format_volume failed to unmount \"%s\"\n", v->mount_point);
        return -1;
    }

    if (strcmp(fs_type, "yaffs2") == 0 || strcmp(fs_type, "mtd") == 0) {
        mtd_scan_partitions();
        const MtdPartition* partition = mtd_find_partition_by_name(device);
        if (partition == NULL) {
            LOGE("format_volume: no MTD partition \"%s\"\n", device);
            return -1;
        }

        MtdWriteContext *write = mtd_write_partition(partition);
        if (write == NULL) {
            LOGW("format_volume: can't open MTD \"%s\"\n", device);
            return -1;
        } else if (mtd_erase_blocks(write, -1) == (off_t) -1) {
            LOGW("format_volume: can't erase MTD \"%s\"\n", device);
            mtd_write_close(write);
            return -1;
        } else if (mtd_write_close(write)) {
            LOGW("format_volume: can't close MTD \"%s\"\n",device);
            return -1;
        }
        return 0;
    }

    if (strcmp(fs_type, "ext4") == 0) {
        int length = 0;
        if (strcmp(v->fs_type, "ext4") == 0) {
            // Our desired filesystem matches the one in fstab, respect v->length
            length = v->length;
        }
        reset_ext4fs_info();
        int result = make_ext4fs(device, length, v->mount_point, sehandle);
        if (result != 0) {
            LOGE("format_volume: make_extf4fs failed on %s\n", device);
            return -1;
        }
        return 0;
    }

    return format_unknown_device(device, path, fs_type);
}

int format_unknown_device(const char *device, const char* path, const char *fs_type)
{
    LOGI("Formatting unknown device.\n");

    if (fs_type != NULL && get_flash_type(fs_type) != UNSUPPORTED)
        return erase_raw_partition(fs_type, device);

    // if this is SDEXT:, don't worry about it if it does not exist.
    if (0 == strcmp(path, "/sd-ext"))
    {
        struct stat st;
        Volume *vol = volume_for_path("/sd-ext");
        if (vol == NULL || 0 != stat(vol->device, &st))
        {
            ui_print("未找到 app2sd 分区. 跳过格式化 /sd-ext.\n");
            return 0;
        }
    }

    if (NULL != fs_type) {
        if (strcmp("ext3", fs_type) == 0) {
            LOGI("Formatting ext3 device.\n");
            if (0 != ensure_path_unmounted(path)) {
                LOGE("Error while unmounting %s.\n", path);
                return -12;
            }
            return format_ext3_device(device);
        }

        if (strcmp("ext2", fs_type) == 0) {
            LOGI("Formatting ext2 device.\n");
            if (0 != ensure_path_unmounted(path)) {
                LOGE("Error while unmounting %s.\n", path);
                return -12;
            }
            return format_ext2_device(device);
        }
    }

    if (0 != ensure_path_mounted(path))
    {
        ui_print("挂载 %s 失败!\n", path);
        ui_print("跳过格式化...\n");
        return 0;
    }

    static char tmp[PATH_MAX];
    if (strcmp(path, "/data") == 0) {
        sprintf(tmp, "cd /data ; for f in $(ls -a | grep -v ^media$); do rm -rf $f; done");
        __system(tmp);
        // if the /data/media sdcard has already been migrated for android 4.2,
        // prevent the migration from happening again by writing the .layout_version
        struct stat st;
        if (0 == lstat("/data/media/0", &st)) {
            char* layout_version = "2";
            FILE* f = fopen("/data/.layout_version", "wb");
            if (NULL != f) {
                fwrite(layout_version, 1, 2, f);
                fclose(f);
            }
            else {
                LOGI("error opening /data/.layout_version for write.\n");
            }
        }
        else {
            LOGI("/data/media/0 not found. migration may occur.\n");
        }
    }
    else {
        sprintf(tmp, "rm -rf %s/*", path);
        __system(tmp);
        sprintf(tmp, "rm -rf %s/.*", path);
        __system(tmp);
    }

    ensure_path_unmounted(path);
    return 0;
}

//#define MOUNTABLE_COUNT 5
//#define DEVICE_COUNT 4
//#define MMC_COUNT 2

typedef struct {
    char mount[255];
    char unmount[255];
    Volume* v;
} MountMenuEntry;

typedef struct {
    char txt[255];
    Volume* v;
} FormatMenuEntry;

int is_safe_to_format(char* name)
{
    char str[255];
    char* partition;
    property_get("ro.cwm.forbid_format", str, "/misc,/radio,/bootloader,/recovery,/efs,/wimax");

    partition = strtok(str, ", ");
    while (partition != NULL) {
        if (strcmp(name, partition) == 0) {
            return 0;
        }
        partition = strtok(NULL, ", ");
    }

    return 1;
}

void show_partition_menu()
{
    static char* headers[] = {  "分区挂载和U盘模式",
                                "",
                                NULL
    };

    static MountMenuEntry* mount_menu = NULL;
    static FormatMenuEntry* format_menu = NULL;

    typedef char* string;

    int i, mountable_volumes, formatable_volumes;
    int num_volumes;
    Volume* device_volumes;

    num_volumes = get_num_volumes();
    device_volumes = get_device_volumes();

    string options[255];

    if(!device_volumes)
        return;

    mountable_volumes = 0;
    formatable_volumes = 0;

    mount_menu = malloc(num_volumes * sizeof(MountMenuEntry));
    format_menu = malloc(num_volumes * sizeof(FormatMenuEntry));

    for (i = 0; i < num_volumes; ++i) {
        Volume* v = &device_volumes[i];
        if(strcmp("ramdisk", v->fs_type) != 0 && strcmp("mtd", v->fs_type) != 0 && strcmp("emmc", v->fs_type) != 0 && strcmp("bml", v->fs_type) != 0) {
            sprintf(&mount_menu[mountable_volumes].mount, "挂载 %s", v->mount_point);
            sprintf(&mount_menu[mountable_volumes].unmount, "卸载 %s", v->mount_point);
            mount_menu[mountable_volumes].v = &device_volumes[i];
            ++mountable_volumes;
            if (is_safe_to_format(v->mount_point)) {
                sprintf(&format_menu[formatable_volumes].txt, "格式化 %s", v->mount_point);
                format_menu[formatable_volumes].v = &device_volumes[i];
                ++formatable_volumes;
            }
        }
        else if (strcmp("ramdisk", v->fs_type) != 0 && strcmp("mtd", v->fs_type) == 0 && is_safe_to_format(v->mount_point))
        {
            sprintf(&format_menu[formatable_volumes].txt, "格式化 %s", v->mount_point);
            format_menu[formatable_volumes].v = &device_volumes[i];
            ++formatable_volumes;
        }
    }

    static char* confirm_format  = "确定格式化?";
    static char* confirm = "是 - 格式化该分区";
    char confirm_string[255];

    for (;;)
    {
        for (i = 0; i < mountable_volumes; i++)
        {
            MountMenuEntry* e = &mount_menu[i];
            Volume* v = e->v;
            if(is_path_mounted(v->mount_point))
                options[i] = e->unmount;
            else
                options[i] = e->mount;
        }

        for (i = 0; i < formatable_volumes; i++)
        {
            FormatMenuEntry* e = &format_menu[i];

            options[mountable_volumes+i] = e->txt;
        }

        if (!is_data_media()) {
          options[mountable_volumes + formatable_volumes] = "挂载U盘模式";
          options[mountable_volumes + formatable_volumes + 1] = NULL;
        }
        else {
          options[mountable_volumes + formatable_volumes] = "格式化 /data and /data/media (/sdcard)";
          options[mountable_volumes + formatable_volumes + 1] = NULL;
        }

        int chosen_item = get_menu_selection(headers, &options, 0, 0);
        if (chosen_item == GO_BACK)
            break;
        if (chosen_item == (mountable_volumes+formatable_volumes)) {
            if (!is_data_media()) {
                show_mount_usb_storage_menu();
            }
            else {
                if (!confirm_selection("格式化 /data 和 /data/media (/sdcard)", confirm))
                    continue;
                handle_data_media_format(1);
                ui_print("格式化DATA分区(/data)...\n");
                if (0 != format_volume("/data"))
                    ui_print("格式化DATA(/data)分区失败！\n");
                else
                    ui_print("格式化完成！\n");
                handle_data_media_format(0);  
            }
        }
        else if (chosen_item < mountable_volumes) {
            MountMenuEntry* e = &mount_menu[chosen_item];
            Volume* v = e->v;

            if (is_path_mounted(v->mount_point))
            {
                if (0 != ensure_path_unmounted(v->mount_point))
                    ui_print("卸载%s失败！\n", v->mount_point);
            }
            else
            {
                if (0 != ensure_path_mounted(v->mount_point))
                    ui_print("挂载%s失败！\n",  v->mount_point);
            }
        }
        else if (chosen_item < (mountable_volumes + formatable_volumes))
        {
            chosen_item = chosen_item - mountable_volumes;
            FormatMenuEntry* e = &format_menu[chosen_item];
            Volume* v = e->v;

            sprintf(confirm_string, "%s - %s", v->mount_point, confirm_format);

            if (!confirm_selection(confirm_string, confirm))
                continue;
            ui_print("正在格式化%s...\n", v->mount_point);
            if (0 != format_volume(v->mount_point))
                ui_print("格式化%s失败！\n", v->mount_point);
            else
                ui_print("格式化完成！\n");
        }
    }

    free(mount_menu);
    free(format_menu);
}

void show_nandroid_advanced_restore_menu(const char* path)
{
    if (ensure_path_mounted(path) != 0) {
        LOGE ("Can't mount sdcard\n");
        return;
    }

    static char* advancedheaders[] = {  "选择恢复镜像",
                                "",
                                "先选择一个需要恢复的镜像",
                                "下级菜单将会显示更多选项",
                                "",
                                "",
                                NULL
    };

    char tmp[PATH_MAX];
    sprintf(tmp, "%s/clockworkmod/backup/", path);
    char* file = choose_file_menu(tmp, NULL, advancedheaders);
    if (file == NULL)
        return;

    static char* headers[] = {  "高级恢复",
                                "",
                                NULL
    };

    static char* list[] = { "恢复 boot",
                            "恢复 system",
                            "恢复 data",
                            "恢复 cache",
                            "恢复 sd-ext",
                            "恢复 wimax",
                            NULL
    };
    
    if (0 != get_partition_device("wimax", tmp)) {
        // disable wimax restore option
        list[5] = NULL;
    }

    static char* confirm_restore  = "确认恢复?";

    int chosen_item = get_menu_selection(headers, list, 0, 0);
    switch (chosen_item)
    {
        case 0:
            if (confirm_selection(confirm_restore, "是 - 恢复 boot"))
                nandroid_restore(file, 1, 0, 0, 0, 0, 0);
            break;
        case 1:
            if (confirm_selection(confirm_restore, "是 - 恢复 system"))
                nandroid_restore(file, 0, 1, 0, 0, 0, 0);
            break;
        case 2:
            if (confirm_selection(confirm_restore, "是 - 恢复 data"))
                nandroid_restore(file, 0, 0, 1, 0, 0, 0);
            break;
        case 3:
            if (confirm_selection(confirm_restore, "是 - 恢复 cache"))
                nandroid_restore(file, 0, 0, 0, 1, 0, 0);
            break;
        case 4:
            if (confirm_selection(confirm_restore, "是 - 恢复 sd-ext"))
                nandroid_restore(file, 0, 0, 0, 0, 1, 0);
            break;
        case 5:
            if (confirm_selection(confirm_restore, "是 - 恢复 wimax"))
                nandroid_restore(file, 0, 0, 0, 0, 0, 1);
            break;
    }
}

static void run_dedupe_gc(const char* other_sd) {
    ensure_path_mounted("/sdcard");
    nandroid_dedupe_gc("/sdcard/clockworkmod/blobs");
    if (other_sd) {
        ensure_path_mounted(other_sd);
        char tmp[PATH_MAX];
        sprintf(tmp, "%s/clockworkmod/blobs", other_sd);
        nandroid_dedupe_gc(tmp);
    }
}

static void choose_default_backup_format() {
    static char* headers[] = {  "默认备份格式设置",
                                "",
                                NULL
    };

    char **list;
    char* list_tar_default[] = { "tar (默认)",
        "dup",
        NULL
    };
    char* list_dup_default[] = { "tar",
        "dup (默认)",
        NULL
    };

    if (nandroid_get_default_backup_format() == NANDROID_BACKUP_FORMAT_DUP) {
        list = list_dup_default;
    } else {
        list = list_tar_default;
    }

    int chosen_item = get_menu_selection(headers, list, 0, 0);
    switch (chosen_item) {
        case 0:
            write_string_to_file(NANDROID_BACKUP_FORMAT_FILE, "tar");
            ui_print("默认备份格式已设置为 tar.\n");
            break;
        case 1:
            write_string_to_file(NANDROID_BACKUP_FORMAT_FILE, "dup");
            ui_print("默认备份格式已设置为 dup.\n");
            break;
    }
}

void show_nandroid_menu()
{
    static char* headers[] = {  "备份与恢复",
                                "",
                                NULL
    };

    char* list[] = { "备份",
                            "恢复",
                            "删除",
                            "高级恢复",
                            "清理未使用的备份数据",
                            "选择默认备份格式",
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL
    };

    char *other_sd = NULL;
    if (volume_for_path("/emmc") != NULL) {
        other_sd = "/emmc";
        list[6] = "备份（内置SD卡）";
        list[7] = "恢复（内置SD卡）";
        list[8] = "高级恢复（内置SD卡）";
        list[9] = "删除（内置SD卡）";
    }
    else if (volume_for_path("/external_sd") != NULL) {
        other_sd = "/external_sd";
        list[6] = "备份（外置SD卡）";
        list[7] = "恢复（外置SD卡）";
        list[8] = "高级恢复（外置SD卡）";
        list[9] = "删除（外置SD卡）";
    }
#ifdef RECOVERY_EXTEND_NANDROID_MENU
    extend_nandroid_menu(list, 10, sizeof(list) / sizeof(char*));
#endif

    for (;;) {
        int chosen_item = get_filtered_menu_selection(headers, list, 0, 0, sizeof(list) / sizeof(char*));
        if (chosen_item == GO_BACK)
            break;
        switch (chosen_item)
        {
            case 0:
                {
                    char backup_path[PATH_MAX];
                    time_t t = time(NULL);
                    struct tm *tmp = localtime(&t);
                    if (tmp == NULL)
                    {
                        struct timeval tp;
                        gettimeofday(&tp, NULL);
                        sprintf(backup_path, "/sdcard/clockworkmod/backup/%d", tp.tv_sec);
                    }
                    else
                    {
                        strftime(backup_path, sizeof(backup_path), "/sdcard/clockworkmod/backup/%F.%H.%M.%S", tmp);
                    }
                    nandroid_backup(backup_path);
                    write_recovery_version();
                }
                break;
            case 1:
                show_nandroid_restore_menu("/sdcard");
                write_recovery_version();
                break;
            case 2:
                show_nandroid_delete_menu("/sdcard");
                write_recovery_version();
                break;
            case 3:
                show_nandroid_advanced_restore_menu("/sdcard");
                write_recovery_version();
                break;
            case 4:
                run_dedupe_gc(other_sd);
                break;
            case 5:
                choose_default_backup_format();
                break;
            case 6:
                {
                    char backup_path[PATH_MAX];
                    time_t t = time(NULL);
                    struct tm *timeptr = localtime(&t);
                    if (timeptr == NULL)
                    {
                        struct timeval tp;
                        gettimeofday(&tp, NULL);
                        if (other_sd != NULL) {
                            sprintf(backup_path, "%s/clockworkmod/backup/%d", other_sd, tp.tv_sec);
                        }
                        else {
                            break;
                        }
                    }
                    else
                    {
                        if (other_sd != NULL) {
                            char tmp[PATH_MAX];
                            strftime(tmp, sizeof(tmp), "clockworkmod/backup/%F.%H.%M.%S", timeptr);
                            // this sprintf results in:
                            // /emmc/clockworkmod/backup/%F.%H.%M.%S (time values are populated too)
                            sprintf(backup_path, "%s/%s", other_sd, tmp);
                        }
                        else {
                            break;
                        }
                    }
                    nandroid_backup(backup_path);
                }
                break;
            case 7:
                if (other_sd != NULL) {
                    show_nandroid_restore_menu(other_sd);
                }
                break;
            case 8:
                if (other_sd != NULL) {
                    show_nandroid_advanced_restore_menu(other_sd);
                }
                break;
            case 9:
                if (other_sd != NULL) {
                    show_nandroid_delete_menu(other_sd);
                }
                break;
            default:
#ifdef RECOVERY_EXTEND_NANDROID_MENU
                handle_nandroid_menu(10, chosen_item);
#endif
                break;
        }
    }
}

static void partition_sdcard(const char* volume) {
    if (!can_partition(volume)) {
        ui_print("无法分区设备: %s\n", volume);
        return;
    }

    static char* ext_sizes[] = { "128M",
                                 "256M",
                                 "512M",
                                 "1024M",
                                 "2048M",
                                 "4096M",
                                 NULL };

    static char* swap_sizes[] = { "0M",
                                  "32M",
                                  "64M",
                                  "128M",
                                  "256M",
                                  NULL };

    static char* ext_headers[] = { "Ext Size", "", NULL };
    static char* swap_headers[] = { "Swap Size", "", NULL };

    int ext_size = get_menu_selection(ext_headers, ext_sizes, 0, 0);
    if (ext_size == GO_BACK)
        return;

    int swap_size = get_menu_selection(swap_headers, swap_sizes, 0, 0);
    if (swap_size == GO_BACK)
        return;

    char sddevice[256];
    Volume *vol = volume_for_path(volume);
    strcpy(sddevice, vol->device);
    // we only want the mmcblk, not the partition
    sddevice[strlen("/dev/block/mmcblkX")] = NULL;
    char cmd[PATH_MAX];
    setenv("SDPATH", sddevice, 1);
    sprintf(cmd, "sdparted -es %s -ss %s -efs ext3 -s", ext_sizes[ext_size], swap_sizes[swap_size]);
    ui_print("正在对SD卡进行分区... 请等待...\n");
    if (0 == __system(cmd))
        ui_print("分区完成！\n");
    else
        ui_print("进行SD卡分区时发生一个未知错误。请查看 /tmp/recovery.log 获取详细信息\n");
}

int can_partition(const char* volume) {
    Volume *vol = volume_for_path(volume);
    if (vol == NULL) {
        LOGI("未能格式化存储器: %s\n", volume);
        return 0;
    }

    int vol_len = strlen(vol->device);
    // do not allow partitioning of a device that isn't mmcblkX or mmcblkXp1
    if (vol->device[vol_len - 2] == 'p' && vol->device[vol_len - 1] != '1') {
        LOGI("不安全设备未能分区: %s\n", vol->device);
        return 0;
    }
    
    if (strcmp(vol->fs_type, "vfat") != 0) {
        LOGI("Can't partition non-vfat: %s\n", vol->fs_type);
        return 0;
    }

    return 1;
}

void show_advanced_menu()
{
    static char* headers[] = {  "高级菜单",
                                "",
                                NULL
    };

    static char* list[] = { "重启到recovery模式",
                            "清空 dalvik cache",
                            "发送错误报告",
                            "按键测试",
                            "显示日志",
                            "修复权限",
                            "重新对sdcard分区",
                            "重新对外部SD卡分区",
                            "重新对内置SD卡分区",
                            NULL
    };

        list[6] = NULL;
        list[7] = NULL;
        list[8] = NULL;

    for (;;)
    {
        int chosen_item = get_filtered_menu_selection(headers, list, 0, 0, sizeof(list) / sizeof(char*));
        if (chosen_item == GO_BACK)
            break;
        switch (chosen_item)
        {
            case 0:
                android_reboot(ANDROID_RB_RESTART2, 0, "recovery");
                break;
            case 1:
                if (0 != ensure_path_mounted("/data"))
                    break;
                ensure_path_mounted("/sd-ext");
                ensure_path_mounted("/cache");
                if (confirm_selection( "确定清空？", "是 - 清空Dalvik Cache")) {
                    __system("rm -r /data/dalvik-cache");
                    __system("rm -r /cache/dalvik-cache");
                    __system("rm -r /sd-ext/dalvik-cache");
                    ui_print("Dalvik Cache 已经清空！\n");
                }
                ensure_path_unmounted("/data");
                break;
            case 2:
                handle_failure(1);
                break;
            case 3:
            {
                ui_print("正在进行键位测试\n");
                ui_print("按返回结束测试.\n");
                int key;
                int action;
                do
                {
                    key = ui_wait_key();
                    action = device_handle_key(key, 1);
                    ui_print("键值: %d\n", key);
                }
                while (action != GO_BACK);
                break;
            }
            case 4:
                ui_printlogtail(12);
                break;
            case 5:
                ensure_path_mounted("/system");
                ensure_path_mounted("/data");
                ui_print("正在修复系统权限...\n");
                __system("fix_permissions");
                ui_print("权限修复完成！\n");
                break;
            case 6:
                partition_sdcard("/sdcard");
                break;
            case 7:
                partition_sdcard("/external_sd");
                break;
            case 8:
                partition_sdcard("/emmc");
                break;
        }
    }
}

void write_fstab_root(char *path, FILE *file)
{
    Volume *vol = volume_for_path(path);
    if (vol == NULL) {
        LOGW("Unable to get recovery.fstab info for %s during fstab generation!\n", path);
        return;
    }

    char device[200];
    if (vol->device[0] != '/')
        get_partition_device(vol->device, device);
    else
        strcpy(device, vol->device);

    fprintf(file, "%s ", device);
    fprintf(file, "%s ", path);
    // special case rfs cause auto will mount it as vfat on samsung.
    fprintf(file, "%s rw\n", vol->fs_type2 != NULL && strcmp(vol->fs_type, "rfs") != 0 ? "auto" : vol->fs_type);
}

void create_fstab()
{
    struct stat info;
    __system("touch /etc/mtab");
    FILE *file = fopen("/etc/fstab", "w");
    if (file == NULL) {
        LOGW("Unable to create /etc/fstab!\n");
        return;
    }
    Volume *vol = volume_for_path("/boot");
    if (NULL != vol && strcmp(vol->fs_type, "mtd") != 0 && strcmp(vol->fs_type, "emmc") != 0 && strcmp(vol->fs_type, "bml") != 0)
         write_fstab_root("/boot", file);
    write_fstab_root("/cache", file);
    write_fstab_root("/data", file);
    write_fstab_root("/datadata", file);
    write_fstab_root("/emmc", file);
    write_fstab_root("/system", file);
    write_fstab_root("/sdcard", file);
    write_fstab_root("/sd-ext", file);
    write_fstab_root("/external_sd", file);
    fclose(file);
    LOGI("Completed outputting fstab.\n");
}

int bml_check_volume(const char *path) {
    ui_print("正在校验 %s...\n", path);
    ensure_path_unmounted(path);
    if (0 == ensure_path_mounted(path)) {
        ensure_path_unmounted(path);
        return 0;
    }
    
    Volume *vol = volume_for_path(path);
    if (vol == NULL) {
        LOGE("Unable process volume! Skipping...\n");
        return 0;
    }
    
    ui_print("%s 可能是 rfs 文件系统. 正在检验...\n", path);
    char tmp[PATH_MAX];
    sprintf(tmp, "mount -t rfs %s %s", vol->device, path);
    int ret = __system(tmp);
    printf("%d\n", ret);
    return ret == 0 ? 1 : 0;
}

void process_volumes() {
    create_fstab();

    if (is_data_media()) {
        setup_data_media();
    }

    return;

    // dead code.
    if (device_flash_type() != BML)
        return;

    ui_print("正在校验 EXT4 分区...\n");
    int ret = 0;
    ret = bml_check_volume("/system");
    ret |= bml_check_volume("/data");
    if (has_datadata())
        ret |= bml_check_volume("/datadata");
    ret |= bml_check_volume("/cache");
    
    if (ret == 0) {
        ui_print("校验完成!\n");
        return;
    }
    
    char backup_path[PATH_MAX];
    time_t t = time(NULL);
    char backup_name[PATH_MAX];
    struct timeval tp;
    gettimeofday(&tp, NULL);
    sprintf(backup_name, "before-ext4-convert-%d", tp.tv_sec);
    sprintf(backup_path, "/sdcard/clockworkmod/backup/%s", backup_name);

    ui_set_show_text(1);
    ui_print("文件系统将会被转换为 EXT4格式.\n");
    ui_print("将会进行备份和恢复操作.\n");
    ui_print("如果出现错误你可以恢复\n");
    ui_print("文件名为%s的备份\n", backup_name);
    ui_print("\n");

    nandroid_backup(backup_path);
    nandroid_restore(backup_path, 1, 1, 1, 1, 1, 0);
    ui_set_show_text(0);
}

void handle_failure(int ret)
{
    if (ret == 0)
        return;
    if (0 != ensure_path_mounted("/sdcard"))
        return;
    mkdir("/sdcard/clockworkmod", S_IRWXU | S_IRWXG | S_IRWXO);
    __system("cp /tmp/recovery.log /sdcard/clockworkmod/recovery.log");
     ui_print("/tmp/recovery.log 已经复制到 /sdcard/clockworkmod/recovery.log. 请打开 ROM Manager 来报告一个错误。\n");
}

int is_path_mounted(const char* path) {
    Volume* v = volume_for_path(path);
    if (v == NULL) {
        return 0;
    }
    if (strcmp(v->fs_type, "ramdisk") == 0) {
        // the ramdisk is always mounted.
        return 1;
    }

    int result;
    result = scan_mounted_volumes();
    if (result < 0) {
        LOGE("failed to scan mounted volumes\n");
        return 0;
    }

    const MountedVolume* mv =
        find_mounted_volume_by_mount_point(v->mount_point);
    if (mv) {
        // volume is already mounted
        return 1;
    }
    return 0;
}

int has_datadata() {
    Volume *vol = volume_for_path("/datadata");
    return vol != NULL;
}

int volume_main(int argc, char **argv) {
    load_volume_table();
    return 0;
}

int verify_root_and_recovery() {
    if (ensure_path_mounted("/system") != 0)
        return 0;

    int ret = 0;
    struct stat st;
    if (0 == lstat("/system/etc/install-recovery.sh", &st)) {
        if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
            ui_show_text(1);
            ret = 1;
            if (confirm_selection("确保恢复系统?", "是 - 禁止恢复到官方恢复系统")) {
                __system("chmod -x /system/etc/install-recovery.sh");
            }
        }
    }

    int exists = 0;
    if (0 == lstat("/system/bin/su", &st)) {
        exists = 1;
        if (S_ISREG(st.st_mode)) {
            if ((st.st_mode & (S_ISUID | S_ISGID)) != (S_ISUID | S_ISGID)) {
                ui_show_text(1);
                ret = 1;
                if (confirm_selection("修复ROOT权限？", "是 - 修复ROOT权限 (/system/bin/su)")) {
                    __system("chmod 6755 /system/bin/su");
                }
            }
        }
    }

    if (0 == lstat("/system/xbin/su", &st)) {
        exists = 1;
        if (S_ISREG(st.st_mode)) {
            if ((st.st_mode & (S_ISUID | S_ISGID)) != (S_ISUID | S_ISGID)) {
                ui_show_text(1);
                ret = 1;
                if (confirm_selection("修复ROOT权限？", "是 - 修复ROOT权限 (/system/xbin/su)")) {
                    __system("chmod 6755 /system/xbin/su");
                }
            }
        }
    }

    if (!exists) {
        ui_show_text(1);
        ret = 1;
        if (confirm_selection("可能您没有ROOT，是否ROOT?", "-是 (/system/xbin/su)")) {
            __system("cp /sbin/su.recovery /system/xbin/su");
            __system("chmod 6755 /system/xbin/su");
            __system("ln -sf /system/xbin/su /system/bin/su");
        }
    }

    ensure_path_unmounted("/system");
    return ret;
}
