#include "device.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

char* get_device_rev_str(device_rev_t rev) {
    switch (rev) {
        case DEVICE_REV_EPASS_0_2:
            return "EPass 0.2";
        case DEVICE_REV_EPASS_0_3_0_4:
            return "EPass 0.3/0.4";
        case DEVICE_REV_EPASS_0_5:
            return "EPass 0.5";
        case DEVICE_REV_EPASS_0_6:
            return "EPass 0.6";
    }
    return "Unknown";
}

char* get_device_screen_str(device_screen_t screen) {
    switch (screen) {
        case DEVICE_SCREEN_360640_HSD:
            return "Generic 360x640(Hannstar)";
        case DEVICE_SCREEN_360640_BOE:
            return "Generic 360x640(BOE)";
        case DEVICE_SCREEN_360640_LAOWU:
            return "Laowu 360x640(Hannstar)";
    }
    return "Unknown";
}

int get_device_info(device_info_t *info){
    int fd = open("/dev/mtdblock0", O_RDONLY);
    char buf[1024];
    if (fd == -1) {
        return -1;
    }

    lseek(fd, DEVCFG_OFFSET, SEEK_SET);
    read(fd, buf, 1024);

    char *device_rev = strstr(buf, "device_rev=");
    char *screen = strstr(buf, "screen=");
    if (device_rev) {
        device_rev += strlen("device_rev=");
        if (strncmp(device_rev, "0.2", 3) == 0) {
            info->rev = DEVICE_REV_EPASS_0_2;
        } else if (strncmp(device_rev, "0.3", 3) == 0) {
            info->rev = DEVICE_REV_EPASS_0_3_0_4;
        } else if (strncmp(device_rev, "0.5", 3) == 0) {
            info->rev = DEVICE_REV_EPASS_0_5;
        } else if (strncmp(device_rev, "0.6", 3) == 0) {
            info->rev = DEVICE_REV_EPASS_0_6;
        }
    }
    else{
        printf("device_rev not found\n");
        close(fd);
        return -1;
    }

    if (screen) {
        screen += strlen("screen=");
        if (strncmp(screen, "hsd", 3) == 0) {
            info->screen = DEVICE_SCREEN_360640_HSD;
        } else if (strncmp(screen, "boe", 3) == 0) {
            info->screen = DEVICE_SCREEN_360640_BOE;
        } else if (strncmp(screen, "laowu", 5) == 0) {
            info->screen = DEVICE_SCREEN_360640_LAOWU;
        }
    }
    else{
        printf("screen not found\n");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}
