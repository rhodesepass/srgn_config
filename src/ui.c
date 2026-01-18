#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "anbui.h"

#include "config_registry.h"
#include "config_state.h"
#include "uenv.h"

static int confirm_yesno(const char *title, const char *prompt) {
    const int32_t r = ad_yesNoBox(title ? title : "Confirm", true, "%s", prompt ? prompt : "");
    return (r == AD_YESNO_YES) ? 0 : 1;
}

static void free_strv(char **v, size_t n) {
    if (!v) return;
    for (size_t i = 0; i < n; i++) free(v[i]);
    free(v);
}

static char *join_tokens(char *const *tokens, size_t n) {
    /* join with single spaces, no trailing space */
    size_t len = 0;
    for (size_t i = 0; i < n; i++) {
        if (!tokens[i] || tokens[i][0] == 0) continue;
        len += strlen(tokens[i]) + 1;
    }
    if (len == 0) return strdup("");

    char *buf = calloc(1, len);
    if (!buf) return NULL;
    size_t off = 0;
    for (size_t i = 0; i < n; i++) {
        if (!tokens[i] || tokens[i][0] == 0) continue;
        const size_t tlen = strlen(tokens[i]);
        if (off != 0) buf[off++] = ' ';
        memcpy(buf + off, tokens[i], tlen);
        off += tlen;
        buf[off] = 0;
    }
    return buf;
}

static int run_category_menu(cfg_state_t *st,
                             device_rev_t dev_rev,
                             cfg_category_t cat,
                             const char *title) {
    if (!st || !st->reg) return -1;

    while (1) {
        ad_Menu *menu = ad_menuCreate(title, "Press ENTER/3 to toggle; \nESC/4 to go back.", true);
        if (!menu) return -1;

        /* build a mapping from visible menu index -> registry index */
        size_t *map = NULL;
        size_t map_count = 0;

        for (size_t i = 0; i < st->reg->count; i++) {
            const cfg_item_t *it = &st->reg->items[i];
            if (it->cat != cat) continue;

            const bool enabled = st->enabled[i];
            const bool avail = cfg_item_is_available(it, dev_rev);

            char line[256];
            if (!avail) {
                snprintf(line, sizeof(line), "[%c] %s (unavailable)", enabled ? 'x' : ' ', it->title);
            } else {
                snprintf(line, sizeof(line), "[%c] %s", enabled ? 'x' : ' ', it->title);
            }
            ad_menuAddItemFormatted(menu, "%s", line);

            size_t *nmap = realloc(map, (map_count + 1) * sizeof(size_t));
            if (!nmap) {
                free(map);
                ad_menuDestroy(menu);
                return -1;
            }
            map = nmap;
            map[map_count++] = i;
        }

        const int32_t sel = ad_menuExecute(menu);
        ad_menuDestroy(menu);
        if (sel == AD_CANCELED) {
            free(map);
            return 0;
        }
        if (sel < 0 || (size_t)sel >= map_count) {
            free(map);
            continue;
        }

        const size_t idx = map[(size_t)sel];
        free(map);

        const cfg_item_t *it = &st->reg->items[idx];
        if (!cfg_item_is_available(it, dev_rev)) {
            ad_okBox("Info", true, "This option is unavailable \non the current device revision.");
            continue;
        }

        const int r = cfg_state_toggle(st, idx, dev_rev, confirm_yesno);
        if (r == -2) {
            ad_okBox("Info", true, "This option is unavailable \non the current device revision.");
        } else if (r == -4) {
            /* user rejected disabling conflicts */
            ad_okBox("Info", true, "Operation cancelled.");
        } else if (r != 0) {
            ad_okBox("Error", true, "Toggle failed (internal error).");
        }
    }
}

int ui_run(const device_info_t *dev_info, const char *uenv_path) {
    if (!dev_info || !uenv_path) return -1;

    char err[256];
    uenv_file_t u = {0};
    if (uenv_load(uenv_path, &u, err, sizeof(err)) != 0) {
        ad_okBox("Error", true, "Failed to read config: %s", err);
        return -1;
    }

    const cfg_registry_t *reg = cfg_registry_get();
    cfg_state_t st;
    if (cfg_state_init_from_uenv(&st, reg, &u, dev_info->rev) != 0) {
        uenv_free(&u);
        ad_okBox("Error", true, "Failed to initialize config state \n(OOM or internal error).");
        return -1;
    }

    char prompt[512];
    snprintf(prompt, sizeof(prompt),
             "Device: %s\nScreen: %s\nuEnv: %s\n\nSelect an action:",
             get_device_rev_str(dev_info->rev),
             get_device_screen_str(dev_info->screen),
             uenv_path);

    while (1) {
        ad_Menu *menu = ad_menuCreate("srgn_config V0.1", prompt, true);
        if (!menu) break;
        ad_menuAddItemFormatted(menu, "Configure interfaces (interface)");
        ad_menuAddItemFormatted(menu, "Configure extensions (ext)");
        ad_menuAddItemFormatted(menu, "View uEnv.txt");
        ad_menuAddItemFormatted(menu, "Save changes");
        ad_menuAddItemFormatted(menu, "Reboot");
        ad_menuAddItemFormatted(menu, "Exit");

        const int32_t sel = ad_menuExecute(menu);
        ad_menuDestroy(menu);

        if (sel == AD_CANCELED || sel == 5) {
            break;
        }

        if (sel == 0) {
            run_category_menu(&st, dev_info->rev, CFG_CAT_INTERFACE, "Interfaces (interface)");
        } else if (sel == 1) {
            run_category_menu(&st, dev_info->rev, CFG_CAT_EXT, "Extensions (ext)");
        } else if (sel == 2) {
            ad_textFileBox("uEnv.txt", uenv_path);
        } else if (sel == 3) {
            char **if_tokens = NULL;
            size_t if_count = 0;
            char **ex_tokens = NULL;
            size_t ex_count = 0;

            if (cfg_state_build_tokens(&st, CFG_CAT_INTERFACE, &if_tokens, &if_count) != 0 ||
                cfg_state_build_tokens(&st, CFG_CAT_EXT, &ex_tokens, &ex_count) != 0) {
                free_strv(if_tokens, if_count);
                free_strv(ex_tokens, ex_count);
                ad_okBox("Error", true, "Failed to build output \n(OOM or internal error).");
                continue;
            }

            char *if_joined = join_tokens(if_tokens, if_count);
            char *ex_joined = join_tokens(ex_tokens, ex_count);
            free_strv(if_tokens, if_count);
            free_strv(ex_tokens, ex_count);

            if (!if_joined || !ex_joined) {
                free(if_joined);
                free(ex_joined);
                ad_okBox("Error", true, "Out of memory.");
                continue;
            }

            if (uenv_write_preserve(uenv_path, &u, if_joined, ex_joined, err, sizeof(err)) != 0) {
                ad_okBox("Error", true, "Write failed: %s", err);
            } else {
                ad_okBox("Done", true, "Written to %s.\nReboot is required \nfor changes to take effect.", uenv_path);
                /* Reload to refresh baseline and parsed tokens */
                uenv_free(&u);
                if (uenv_load(uenv_path, &u, err, sizeof(err)) != 0) {
                    ad_okBox("Warning", true, "Write succeeded, but reload failed: %s", err);
                }
            }

            free(if_joined);
            free(ex_joined);
        }
        if (sel == 4) {
            system("reboot");
            break;
        }
    }

    cfg_state_free(&st);
    uenv_free(&u);
    return 0;
}

