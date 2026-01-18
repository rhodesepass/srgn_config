#include "config_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_strv(char **v, size_t n) {
    if (!v) return;
    for (size_t i = 0; i < n; i++) free(v[i]);
    free(v);
}

static int strv_push(char ***v, size_t *n, const char *s) {
    char **nv = realloc(*v, (*n + 1) * sizeof(char *));
    if (!nv) return -1;
    nv[*n] = strdup(s ? s : "");
    if (!nv[*n]) {
        *v = nv;
        return -1;
    }
    *v = nv;
    *n = *n + 1;
    return 0;
}

static int strv_contains(char *const *v, size_t n, const char *s) {
    if (!s) return 0;
    for (size_t i = 0; i < n; i++) {
        if (v[i] && strcmp(v[i], s) == 0) return 1;
    }
    return 0;
}

static int find_item_index_by_id(const cfg_registry_t *reg, const char *id) {
    if (!reg || !id) return -1;
    for (size_t i = 0; i < reg->count; i++) {
        if (reg->items[i].id && strcmp(reg->items[i].id, id) == 0) return (int)i;
    }
    return -1;
}

bool cfg_item_is_available(const cfg_item_t *it, device_rev_t dev_rev) {
    if (!it) return false;
    const int r = device_rev_rank(dev_rev);
    if (r < 0) return false;
    if (it->min_rev_rank >= 0 && r < it->min_rev_rank) return false;
    if (it->max_rev_rank >= 0 && r > it->max_rev_rank) return false;
    return true;
}

int cfg_state_init_from_uenv(cfg_state_t *st,
                             const cfg_registry_t *reg,
                             const uenv_file_t *u,
                             device_rev_t dev_rev) {
    (void)dev_rev;
    if (!st || !reg || !u) return -1;
    memset(st, 0, sizeof(*st));
    st->reg = reg;
    st->enabled = calloc(reg->count, sizeof(bool));
    if (!st->enabled) return -1;

    /* interface tokens */
    for (size_t i = 0; i < u->interface_token_count; i++) {
        const char *tok = u->interface_tokens[i];
        const int idx = find_item_index_by_id(reg, tok);
        if (idx >= 0 && reg->items[idx].cat == CFG_CAT_INTERFACE) {
            st->enabled[idx] = true;
        } else {
            if (!strv_contains(st->unknown_interface, st->unknown_interface_count, tok)) {
                if (strv_push(&st->unknown_interface, &st->unknown_interface_count, tok) != 0) {
                    cfg_state_free(st);
                    return -1;
                }
            }
        }
    }

    /* ext tokens */
    for (size_t i = 0; i < u->ext_token_count; i++) {
        const char *tok = u->ext_tokens[i];
        const int idx = find_item_index_by_id(reg, tok);
        if (idx >= 0 && reg->items[idx].cat == CFG_CAT_EXT) {
            st->enabled[idx] = true;
        } else {
            if (!strv_contains(st->unknown_ext, st->unknown_ext_count, tok)) {
                if (strv_push(&st->unknown_ext, &st->unknown_ext_count, tok) != 0) {
                    cfg_state_free(st);
                    return -1;
                }
            }
        }
    }

    return 0;
}

void cfg_state_free(cfg_state_t *st) {
    if (!st) return;
    free(st->enabled);
    free_strv(st->unknown_interface, st->unknown_interface_count);
    free_strv(st->unknown_ext, st->unknown_ext_count);
    memset(st, 0, sizeof(*st));
}

static int enable_item_recursive(cfg_state_t *st, size_t idx, device_rev_t dev_rev, int depth) {
    if (!st || !st->reg) return -1;
    if (idx >= st->reg->count) return -1;
    if (depth > 16) return -1; /* prevent cycles from exploding */

    const cfg_item_t *it = &st->reg->items[idx];
    if (!cfg_item_is_available(it, dev_rev)) return -2;

    /* enable dependencies first */
    for (size_t r = 0; r < it->requires_count; r++) {
        const char *rid = it->requires[r];
        const int rix = find_item_index_by_id(st->reg, rid);
        if (rix < 0) return -1;
        if (enable_item_recursive(st, (size_t)rix, dev_rev, depth + 1) < 0) return -1;
    }

    st->enabled[idx] = true;
    return 0;
}

int cfg_state_toggle(cfg_state_t *st,
                     size_t item_index,
                     device_rev_t dev_rev,
                     int (*confirm)(const char *title, const char *prompt)) {
    if (!st || !st->reg || !st->enabled) return -1;
    if (item_index >= st->reg->count) return -1;

    const cfg_item_t *it = &st->reg->items[item_index];

    if (!st->enabled[item_index]) {
        /* enable */
        if (!cfg_item_is_available(it, dev_rev)) return -2;

        if (confirm) {
            char help[256];
            snprintf(help, sizeof(help), "Option %s:\nDescription: \n%s", it->id, it->help ? it->help : "");
            const int ans = confirm("Confirm", help);
            if (ans != 0) {
                return -4; /* user rejected */
            }
        }

        /* conflicts: if a conflict is enabled, ask to disable it */
        for (size_t c = 0; c < it->conflicts_count; c++) {
            const char *cid = it->conflicts[c];
            const int cix = find_item_index_by_id(st->reg, cid);
            if (cix < 0) continue;
            if (st->enabled[cix]) {
                if (!confirm) return -3;
                char prompt[256];
                snprintf(prompt, sizeof(prompt), "Enabling %s \nconflicts with\n %s.\nDisable %s?", it->id, cid, cid);
                const int ans = confirm("Conflict", prompt);
                if (ans != 0) {
                    return -4; /* user rejected */
                }
                st->enabled[cix] = false;
            }
        }

        /* enable (including recursive dependencies) */
        return enable_item_recursive(st, item_index, dev_rev, 0);
    }

    /* disable: only disable itself (do not auto-disable reverse dependencies) */
    st->enabled[item_index] = false;
    return 0;
}

int cfg_state_build_tokens(const cfg_state_t *st,
                           cfg_category_t cat,
                           char ***tokens_out,
                           size_t *count_out) {
    if (!tokens_out || !count_out || !st || !st->reg || !st->enabled) return -1;
    *tokens_out = NULL;
    *count_out = 0;

    /* known tokens: stable output order = registry order */
    for (size_t i = 0; i < st->reg->count; i++) {
        const cfg_item_t *it = &st->reg->items[i];
        if (it->cat != cat) continue;
        if (!st->enabled[i]) continue;
        if (strv_push(tokens_out, count_out, it->id) != 0) {
            free_strv(*tokens_out, *count_out);
            *tokens_out = NULL;
            *count_out = 0;
            return -1;
        }
    }

    /* unknown tokens: keep original order from uEnv */
    if (cat == CFG_CAT_INTERFACE) {
        for (size_t i = 0; i < st->unknown_interface_count; i++) {
            if (strv_push(tokens_out, count_out, st->unknown_interface[i]) != 0) {
                free_strv(*tokens_out, *count_out);
                *tokens_out = NULL;
                *count_out = 0;
                return -1;
            }
        }
    } else {
        for (size_t i = 0; i < st->unknown_ext_count; i++) {
            if (strv_push(tokens_out, count_out, st->unknown_ext[i]) != 0) {
                free_strv(*tokens_out, *count_out);
                *tokens_out = NULL;
                *count_out = 0;
                return -1;
            }
        }
    }

    return 0;
}

