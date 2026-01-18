#pragma once

#include <stddef.h>

/*
 * uEnv.txt read/write helper
 *
 * Rules:
 * - Preserve all lines exactly as read
 * - Only modify/add the "interface=" and "ext=" lines on save
 * - If "interface="/ "ext=" is missing, append it at end of file
 * - All other keys/comments/order must remain byte-for-byte identical
 */

typedef struct {
    char   **lines;
    size_t   line_count;

    /* index in lines[], -1 if not found */
    long     interface_idx;
    long     ext_idx;

    char   **interface_tokens;
    size_t   interface_token_count;

    char   **ext_tokens;
    size_t   ext_token_count;
} uenv_file_t;

int  uenv_load(const char *path, uenv_file_t *out, char *err, size_t err_len);
void uenv_free(uenv_file_t *u);

/* Save: preserve everything, only replace/append interface/ext lines */
int uenv_write_preserve(const char *path,
                        const uenv_file_t *u,
                        const char *interface_line,
                        const char *ext_line,
                        char *err,
                        size_t err_len);

