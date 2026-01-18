#pragma once

#define DEVCFG_OFFSET 0xFA000

typedef enum {
    DEVICE_REV_EPASS_0_2 = 0,
    DEVICE_REV_EPASS_0_3_0_4 = 1,
    DEVICE_REV_EPASS_0_5 = 2,
    DEVICE_REV_EPASS_0_6 = 3,
} device_rev_t;

typedef enum {
    DEVICE_SCREEN_360640_HSD = 0,
    DEVICE_SCREEN_360640_BOE = 1,
    DEVICE_SCREEN_360640_LAOWU = 2,
} device_screen_t;

typedef struct {
    device_rev_t rev;
    device_screen_t screen;
} device_info_t;

char* get_device_rev_str(device_rev_t rev);
char* get_device_screen_str(device_screen_t screen);

int get_device_info(device_info_t *info);