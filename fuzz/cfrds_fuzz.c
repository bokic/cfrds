#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdalign.h>

// Include only what's needed (no Qt/UI deps)
#include "../include/internal/cfrds_buffer.h"
#include "../include/internal/wddx.h"

// Silence unused warnings in clean build
#pragma GCC diagnostic ignored "-Wunused-function"

// Forward-declare cleanup helpers if not exposed in headers
void wddx_cleanup(WDDX **p);

// Helper: ensure input is null-terminated (required by many parsers)
static char *null_terminate(const uint8_t *Data, size_t Size) {
    if (Size == 0) return NULL;
    char *s = malloc(Size + 1);
    if (!s) return NULL;
    memcpy(s, Data, Size);
    s[Size] = '\0';
    return s;
}

// ===== Fuzz targets =====

// fuzz_rds_number.c
int LLVMFuzzerTestOneInput_rds_number(const uint8_t *Data, size_t Size) {
    if (Size < 1) return 0;

    char *input = null_terminate(Data, Size);
    if (!input) return 0;

    const char *p = input;
    size_t remaining = strlen(input);
    int64_t out;

    cfrds_buffer_parse_number(&p, &remaining, &out);

    free(input);
    return 0;
}

// fuzz_rds_string.c
int LLVMFuzzerTestOneInput_rds_string(const uint8_t *Data, size_t Size) {
    if (Size < 1) return 0;

    char *input = null_terminate(Data, Size);
    if (!input) return 0;

    const char *p = input;
    size_t remaining = strlen(input);
    char *out = NULL;

    bool ok = cfrds_buffer_parse_string(&p, &remaining, &out);

    if (ok && out) {
        free(out);  // caller owns memory
    }

    free(input);
    return 0;
}

// fuzz_rds_string_list_item.c
int LLVMFuzzerTestOneInput_rds_string_list_item(const uint8_t *Data, size_t Size) {
    if (Size < 2) return 0;  // at least 2 chars (e.g., `a,`)

    char *input = null_terminate(Data, Size);
    if (!input) return 0;

    const char *p = input;
    size_t remaining = strlen(input);
    char *out = NULL;

    bool ok = cfrds_buffer_parse_string_list_item(&p, &remaining, &out);

    if (ok && out) {
        free(out);
    }

    free(input);
    return 0;
}

// fuzz_rds_bytearray.c
int LLVMFuzzerTestOneInput_rds_bytearray(const uint8_t *Data, size_t Size) {
    if (Size < 1) return 0;

    char *input = null_terminate(Data, Size);
    if (!input) return 0;

    const char *p = input;
    size_t remaining = strlen(input);
    char *out = NULL;
    int size = -1;

    bool ok = cfrds_buffer_parse_bytearray(&p, &remaining, &out, &size);

    if (ok && out) {
        free(out);
    }

    free(input);
    return 0;
}

// fuzz_rds_browse_dir.c
int LLVMFuzzerTestOneInput_rds_browse_dir(const uint8_t *Data, size_t Size) {
    cfrds_buffer *buf = NULL;
    if (!cfrds_buffer_create(&buf)) return 0;

    // Ensure we don't exceed size_t limit (prevents overflow in realloc)
    size_t safe_size = Size;
    if (safe_size > 1024 * 1024) safe_size = 1024 * 1024;  // 1MB cap

    if (!cfrds_buffer_append_bytes(buf, Data, safe_size)) {
        cfrds_buffer_free(buf);
        return 0;
    }

    cfrds_browse_dir_int *res = cfrds_buffer_to_browse_dir(buf);
    if (res) {
        // Free items
        for (size_t i = 0; i < res->cnt; i++) {
            free(res->items[i].name);
        }
        free(res);
    }

    cfrds_buffer_free(buf);
    return 0;
}

// fuzz_rds_sql_sqlstmnt.c
int LLVMFuzzerTestOneInput_rds_sql_sqlstmnt(const uint8_t *Data, size_t Size) {
    cfrds_buffer *buf = NULL;
    if (!cfrds_buffer_create(&buf)) return 0;

    size_t safe_size = Size > 1024 * 1024 ? 1024 * 1024 : Size;
    if (!cfrds_buffer_append_bytes(buf, Data, safe_size)) {
        cfrds_buffer_free(buf);
        return 0;
    }

    cfrds_sql_resultset_int *res = cfrds_buffer_to_sql_sqlstmnt(buf);
    if (res) {
        // Free all values
        size_t total = res->columns * (res->rows + 1);
        for (size_t i = 0; i < total; i++) {
            free(res->values[i]);
        }
        free(res);
    }

    cfrds_buffer_free(buf);
    return 0;
}

// fuzz_wddx_xml.c — fuzzes WDDX XML parser (debugger events)
int LLVMFuzzerTestOneInput_wddx_xml(const uint8_t *Data, size_t Size) {
    if (Size < 1) return 0;

    char *xml = null_terminate(Data, Size);
    if (!xml) return 0;

    // Prevent DoS: cap XML size and skip large inputs
    if (strlen(xml) > 1024) {
        free(xml);
        return 0;
    }

    WDDX *wddx = wddx_from_xml(xml);
    if (wddx) {
        const WDDX_NODE *data = wddx_data(wddx);
        if (data) {
            int type = wddx_node_type(data);
            if (type == WDDX_STRING) {
                const char *s = wddx_node_string(data);
                (void)s; // use to avoid warning
            }
        }
        wddx_cleanup(&wddx);
    }

    free(xml);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size < 2) return 0;

    switch (Data[0]) {
        case 'S': // "STR:" + data → test parse_string
            {
                char *input = malloc(Size);
                memcpy(input, Data, Size);
                input[Size-1] = '\0';  // ensure null-termination
                const char *p = input;
                size_t rem = strlen(p);
                char *out = NULL;
                cfrds_buffer_parse_string(&p, &rem, &out);
                if (out) free(out);
                free(input);
            }
            break;

        case 'B': // "BROWSE:" + data → simulate browse_dir
            {
                cfrds_buffer *buf = NULL;
                cfrds_buffer_create(&buf);
                cfrds_buffer_append_bytes(buf, Data + 1, Size - 1);
                cfrds_browse_dir_int *res = cfrds_buffer_to_browse_dir(buf);
                if (res) {
                    for (size_t i = 0; i < res->cnt; i++) free(res->items[i].name);
                    free(res);
                }
                cfrds_buffer_free(buf);
            }
            break;

        case 'W': // WDDX XML
            {
                char *xml = malloc(Size);
                memcpy(xml, Data, Size);
                xml[Size-1] = '\0';
                WDDX *wddx = wddx_from_xml(xml);
                if (wddx) wddx_cleanup(&wddx);
                free(xml);
            }
            break;

        default:
            break;
    }

    return 0;
}
