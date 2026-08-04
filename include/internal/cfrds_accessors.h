#pragma once

#include <cfrds.h>
#include <internal/cfrds_buffer.h>

#define CFRDS_CHECK_BOUNDS(value, ndx, default_ret) \
    do { \
        if ((value) == NULL || (ndx) >= (value)->cnt) \
            return (default_ret); \
    } while (0)

#define DEFINE_STRING_ACCESSOR(func_name, struct_type, field) \
    const char *func_name(const struct_type *value, size_t ndx) { \
        CFRDS_CHECK_BOUNDS(value, ndx, NULL); \
        return value->items[ndx].field; \
    }

#define DEFINE_INT_ACCESSOR(func_name, struct_type, field, default_val) \
    int func_name(const struct_type *value, size_t ndx) { \
        CFRDS_CHECK_BOUNDS(value, ndx, default_val); \
        return value->items[ndx].field; \
    }

#define DEFINE_CHAR_ACCESSOR(func_name, struct_type, field, default_val) \
    char func_name(const struct_type *value, size_t ndx) { \
        CFRDS_CHECK_BOUNDS(value, ndx, default_val); \
        return value->items[ndx].field; \
    }

#define DEFINE_UINT8_ACCESSOR(func_name, struct_type, field, default_val) \
    uint8_t func_name(const struct_type *value, size_t ndx) { \
        CFRDS_CHECK_BOUNDS(value, ndx, default_val); \
        return value->items[ndx].field; \
    }

#define DEFINE_SIZE_ACCESSOR(func_name, struct_type, field, default_val) \
    size_t func_name(const struct_type *value, size_t ndx) { \
        CFRDS_CHECK_BOUNDS(value, ndx, default_val); \
        return value->items[ndx].field; \
    }

#define DEFINE_UINT64_ACCESSOR(func_name, struct_type, field, default_val) \
    uint64_t func_name(const struct_type *value, size_t ndx) { \
        CFRDS_CHECK_BOUNDS(value, ndx, default_val); \
        return value->items[ndx].field; \
    }
