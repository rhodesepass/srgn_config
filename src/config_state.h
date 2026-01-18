#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "config_registry.h"
#include "uenv.h"

typedef struct {
    const cfg_registry_t *reg;
    bool *enabled; /* len = reg->count */

    /* tokens present in uEnv but not recognized by the registry (must be preserved on save) */
    char **unknown_interface;
    size_t unknown_interface_count;
    char **unknown_ext;
    size_t unknown_ext_count;
} cfg_state_t;

int  cfg_state_init_from_uenv(cfg_state_t *st,
                              const cfg_registry_t *reg,
                              const uenv_file_t *u,
                              device_rev_t dev_rev);
void cfg_state_free(cfg_state_t *st);

bool cfg_item_is_available(const cfg_item_t *it, device_rev_t dev_rev);

/* confirm callback: return 0 for Yes, non-zero for No */
int cfg_state_toggle(cfg_state_t *st,
                     size_t item_index,
                     device_rev_t dev_rev,
                     int (*confirm)(const char *title, const char *prompt));

int cfg_state_build_tokens(const cfg_state_t *st,
                           cfg_category_t cat,
                           char ***tokens_out,
                           size_t *count_out);

