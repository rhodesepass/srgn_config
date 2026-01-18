/*
    AnbUI Miniature Text UI Lib for Burger Enjoyers(tm)

    ad_test: AnbUI Example Test Application

    Tip of the day: Cheese burgers and bacon burgers are like ying and yang
    except you get to enjoy the ying and yang with cheese and bacon without
    having to compromise on the other.

    (C) 2024 E. Voirin (oerg866) */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ad_priv.h"
#include "anbui.h"

#include "device.h"
#include "ui.h"

extern ad_ConsoleConfig ad_s_con;

static const char *get_uenv_path_from_args_or_env(int argc, char **argv) {
    const char *env = getenv("SRGN_UENV_PATH");
    const char *path = (env && env[0]) ? env : "/boot/uEnv.txt";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--uenv") == 0 && (i + 1) < argc) {
            path = argv[i + 1];
            i++;
        }
    }
    return path;
}

int main(int argc, char *argv[]) {
    ad_init("Shirogane EPass Device Config Tool V0.1");
    // fix srgnvs8pix
    if (strcmp(ttyname(fileno(stdin)), "/dev/tty0") == 0) {
        printf("current tty is /dev/tty0\n");
        ad_s_con.width -= 3;
    }

    char dev_status_buf[512];
    device_info_t dev_info;
    if (get_device_info(&dev_info) != 0) {
        printf("get device info failed\n");
        return -1;
    }
    snprintf(dev_status_buf, sizeof(dev_status_buf), "Dev:%s\nScreen:%s",
           get_device_rev_str(dev_info.rev),
           get_device_screen_str(dev_info.screen));
    puts(dev_status_buf);

    const char *uenv_path = get_uenv_path_from_args_or_env(argc, argv);
    (void)ui_run(&dev_info, uenv_path);

    ad_deinit();

    return 0;
}
