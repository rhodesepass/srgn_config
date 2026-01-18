#pragma once

#include <stddef.h>

#include "device.h"

typedef enum {
    CFG_CAT_INTERFACE = 0,
    CFG_CAT_EXT       = 1,
} cfg_category_t;

typedef struct {
    const char *id;     /* token name (written into uEnv token list) */
    cfg_category_t cat; /* interface / ext */

    const char *title;  /* UI label */
    const char *help;   /* help text, optional */

    /* Device revision constraints by rank; -1 means no lower/upper bound */
    int min_rev_rank;
    int max_rev_rank;

    const char *const *requires;
    size_t requires_count;

    const char *const *conflicts;
    size_t conflicts_count;
} cfg_item_t;

typedef struct {
    const cfg_item_t *items;
    size_t count;
} cfg_registry_t;

const cfg_registry_t *cfg_registry_get(void);

/* Device revision rank (for min/max constraints) */
int device_rev_rank(device_rev_t rev);

