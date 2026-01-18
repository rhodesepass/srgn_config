#pragma once

#include "device.h"

/* Return 0 on normal exit; non-zero on error (UI already displayed the message). */
int ui_run(const device_info_t *dev_info, const char *uenv_path);

