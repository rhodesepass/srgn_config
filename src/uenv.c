#include "uenv.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void set_err(char *err, size_t err_len, const char *msg) {
    if (err && err_len) {
        snprintf(err, err_len, "%s", msg ? msg : "Unknown error");
    }
}

static void set_err_errno(char *err, size_t err_len, const char *prefix) {
    if (err && err_len) {
        if (prefix) {
            snprintf(err, err_len, "%s: %s", prefix, strerror(errno));
        } else {
            snprintf(err, err_len, "%s", strerror(errno));
        }
    }
}

static void free_strv(char **v, size_t n) {
    if (!v) return;
    for (size_t i = 0; i < n; i++) {
        free(v[i]);
    }
    free(v);
}

static int strv_push(char ***v, size_t *n, const char *s) {
    char **nv = realloc(*v, (*n + 1) * sizeof(char *));
    if (!nv) return -1;
    nv[*n] = strdup(s ? s : "");
    if (!nv[*n]) {
        /* keep realloc result */
        *v = nv;
        return -1;
    }
    *v = nv;
    *n = *n + 1;
    return 0;
}

static char *ltrim(char *s) {
    while (s && *s && isspace((unsigned char)*s)) s++;
    return s;
}

static int starts_with_key(const char *line, const char *key) {
    if (!line || !key) return 0;
    const char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    return strncmp(p, key, strlen(key)) == 0;
}

/* Parse whitespace-separated tokens after key=. Returns strdup'ed token list. */
static int parse_tokens_from_line(const char *line, const char *key, char ***tokens_out, size_t *count_out) {
    *tokens_out = NULL;
    *count_out = 0;
    if (!line || !key) return 0;

    const char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    const size_t klen = strlen(key);
    if (strncmp(p, key, klen) != 0) return 0;
    p += klen;

    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p || *p == '\n' || *p == '\r') break;
        const char *start = p;
        while (*p && !isspace((unsigned char)*p) && *p != '\n' && *p != '\r') p++;
        const size_t len = (size_t)(p - start);
        char tmp[256];
        if (len >= sizeof(tmp)) {
            /* Truncate extremely long tokens to avoid pathological inputs */
            memcpy(tmp, start, sizeof(tmp) - 1);
            tmp[sizeof(tmp) - 1] = 0;
        } else {
            memcpy(tmp, start, len);
            tmp[len] = 0;
        }
        if (strv_push(tokens_out, count_out, tmp) != 0) {
            free_strv(*tokens_out, *count_out);
            *tokens_out = NULL;
            *count_out = 0;
            return -1;
        }
    }

    return 0;
}

static int ensure_final_nl(FILE *out, const uenv_file_t *u) {
    if (!u || u->line_count == 0) return 0;
    const char *last = u->lines[u->line_count - 1];
    if (!last) return 0;
    const size_t len = strlen(last);
    if (len == 0) return 0;
    if (last[len - 1] == '\n') return 0;
    if (fputc('\n', out) == EOF) return -1;
    return 0;
}

int uenv_load(const char *path, uenv_file_t *out, char *err, size_t err_len) {
    if (!out || !path) {
        set_err(err, err_len, "Invalid argument");
        return -1;
    }
    memset(out, 0, sizeof(*out));
    out->interface_idx = -1;
    out->ext_idx = -1;

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        set_err_errno(err, err_len, "Failed to open uEnv.txt");
        return -1;
    }

    char *line = NULL;
    size_t cap = 0;
    ssize_t nread;
    while ((nread = getline(&line, &cap, fp)) != -1) {
        (void)nread;
        if (strv_push(&out->lines, &out->line_count, line) != 0) {
            free(line);
            fclose(fp);
            uenv_free(out);
            set_err(err, err_len, "Out of memory");
            return -1;
        }
    }

    free(line);
    fclose(fp);

    /* Find the first interface=/ext= lines (only the first occurrence is treated as authoritative) */
    for (size_t i = 0; i < out->line_count; i++) {
        const char *cur = out->lines[i];
        if (out->interface_idx < 0 && starts_with_key(cur, "interface=")) {
            out->interface_idx = (long)i;
            if (parse_tokens_from_line(cur, "interface=", &out->interface_tokens, &out->interface_token_count) != 0) {
                uenv_free(out);
                set_err(err, err_len, "Failed to parse interface=");
                return -1;
            }
        } else if (out->ext_idx < 0 && starts_with_key(cur, "ext=")) {
            out->ext_idx = (long)i;
            if (parse_tokens_from_line(cur, "ext=", &out->ext_tokens, &out->ext_token_count) != 0) {
                uenv_free(out);
                set_err(err, err_len, "Failed to parse ext=");
                return -1;
            }
        }
        if (out->interface_idx >= 0 && out->ext_idx >= 0) break;
    }

    return 0;
}

void uenv_free(uenv_file_t *u) {
    if (!u) return;
    free_strv(u->lines, u->line_count);
    free_strv(u->interface_tokens, u->interface_token_count);
    free_strv(u->ext_tokens, u->ext_token_count);
    memset(u, 0, sizeof(*u));
    u->interface_idx = -1;
    u->ext_idx = -1;
}

static int write_line(FILE *out, const char *s) {
    if (!s) return 0;
    if (fputs(s, out) == EOF) return -1;
    return 0;
}

static int write_kv_line(FILE *out, const char *key, const char *value_line) {
    /* value_line can be a full line (with newline) or just the value part.
     * If it already starts with key, write it as-is (ensuring trailing newline).
     * Otherwise write key + value_line + newline. */
    if (!key) return -1;
    if (!value_line) value_line = "";

    const char *trimmed = ltrim((char *)value_line);
    if (starts_with_key(trimmed, key)) {
        /* Preserve caller-provided line (ensure trailing newline) */
        if (fputs(trimmed, out) == EOF) return -1;
        const size_t len = strlen(trimmed);
        if (len == 0 || trimmed[len - 1] != '\n') {
            if (fputc('\n', out) == EOF) return -1;
        }
        return 0;
    }

    if (fputs(key, out) == EOF) return -1;
    if (fputs(value_line, out) == EOF) return -1;
    /* Ensure newline */
    const size_t len = strlen(value_line);
    if (len == 0 || value_line[len - 1] != '\n') {
        if (fputc('\n', out) == EOF) return -1;
    }
    return 0;
}

int uenv_write_preserve(const char *path,
                        const uenv_file_t *u,
                        const char *interface_line,
                        const char *ext_line,
                        char *err,
                        size_t err_len) {
    if (!path || !u) {
        set_err(err, err_len, "Invalid argument");
        return -1;
    }

    /* Temp file in same directory to keep rename() atomic */
    char tmp_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s.srgn_config.tmp", path);

    FILE *out = fopen(tmp_path, "wb");
    if (!out) {
        set_err_errno(err, err_len, "Failed to create temp file");
        return -1;
    }

    const long if_idx = u->interface_idx;
    const long ex_idx = u->ext_idx;

    for (size_t i = 0; i < u->line_count; i++) {
        if ((long)i == if_idx) {
            if (write_kv_line(out, "interface=", interface_line) != 0) {
                fclose(out);
                unlink(tmp_path);
                set_err_errno(err, err_len, "Failed to write interface=");
                return -1;
            }
            continue;
        }
        if ((long)i == ex_idx) {
            if (write_kv_line(out, "ext=", ext_line) != 0) {
                fclose(out);
                unlink(tmp_path);
                set_err_errno(err, err_len, "Failed to write ext=");
                return -1;
            }
            continue;
        }
        if (write_line(out, u->lines[i]) != 0) {
            fclose(out);
            unlink(tmp_path);
            set_err_errno(err, err_len, "Failed to write file");
            return -1;
        }
    }

    /* Append missing keys at end (ensure newline boundary first) */
    if (if_idx < 0) {
        if (ensure_final_nl(out, u) != 0) {
            fclose(out);
            unlink(tmp_path);
            set_err_errno(err, err_len, "Failed to write newline");
            return -1;
        }
        if (write_kv_line(out, "interface=", interface_line) != 0) {
            fclose(out);
            unlink(tmp_path);
            set_err_errno(err, err_len, "Failed to append interface=");
            return -1;
        }
    }
    if (ex_idx < 0) {
        if (ensure_final_nl(out, u) != 0) {
            fclose(out);
            unlink(tmp_path);
            set_err_errno(err, err_len, "Failed to write newline");
            return -1;
        }
        if (write_kv_line(out, "ext=", ext_line) != 0) {
            fclose(out);
            unlink(tmp_path);
            set_err_errno(err, err_len, "Failed to append ext=");
            return -1;
        }
    }

    if (fflush(out) != 0) {
        fclose(out);
        unlink(tmp_path);
        set_err_errno(err, err_len, "Failed to flush temp file");
        return -1;
    }
    /* Best-effort flush to disk */
    int fd = fileno(out);
    if (fd >= 0) {
        (void)fsync(fd);
    }
    fclose(out);

    if (rename(tmp_path, path) != 0) {
        unlink(tmp_path);
        set_err_errno(err, err_len, "Failed to replace uEnv.txt");
        return -1;
    }

    return 0;
}

