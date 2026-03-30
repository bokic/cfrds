/*
 * test_buffer.c — Unit tests for cfrds_buffer internals.
 *
 * No external framework required: each test function returns 1 on failure,
 * 0 on success. main() tallies failures and exits non-zero if any fail.
 * CTest treats non-zero exit as a test failure.
 */

#include <internal/cfrds_buffer.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* ── Minimal assert helper ─────────────────────────────────────────────── */

#define PASS 0
#define FAIL 1

static int _failures = 0;

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "FAIL  %s:%d  %s\n", __func__, __LINE__, #expr); \
            return FAIL; \
        } \
    } while (0)

#define RUN(fn) \
    do { \
        int _r = fn(); \
        if (_r == PASS) { \
            printf("PASS  %s\n", #fn); \
        } else { \
            printf("FAIL  %s\n", #fn); \
            _failures++; \
        } \
    } while (0)

/* ── Tests: create / free ──────────────────────────────────────────────── */

static int test_create_null_out(void)
{
    CHECK(cfrds_buffer_create(NULL) == false);
    return PASS;
}

static int test_create_and_free(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf) == true);
    CHECK(buf != NULL);
    CHECK(cfrds_buffer_data_size(buf) == 0);
    cfrds_buffer_free(buf);
    return PASS;
}

static int test_free_null(void)
{
    /* Must not crash */
    cfrds_buffer_free(NULL);
    return PASS;
}

/* ── Tests: data / size on empty buffer ───────────────────────────────── */

static int test_data_null_buffer(void)
{
    CHECK(cfrds_buffer_data(NULL) == NULL);
    CHECK(cfrds_buffer_data_size(NULL) == 0);
    return PASS;
}

/* ── Tests: append (string) ────────────────────────────────────────────── */

static int test_append_basic(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));

    CHECK(cfrds_buffer_append(buf, "hello"));
    CHECK(cfrds_buffer_data_size(buf) == 5);
    CHECK(memcmp(cfrds_buffer_data(buf), "hello", 5) == 0);

    CHECK(cfrds_buffer_append(buf, " world"));
    CHECK(cfrds_buffer_data_size(buf) == 11);
    CHECK(memcmp(cfrds_buffer_data(buf), "hello world", 11) == 0);

    cfrds_buffer_free(buf);
    return PASS;
}

static int test_append_null_buffer(void)
{
    CHECK(cfrds_buffer_append(NULL, "x") == false);
    return PASS;
}

static int test_append_null_str(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));
    CHECK(cfrds_buffer_append(buf, NULL) == false);
    cfrds_buffer_free(buf);
    return PASS;
}

static int test_append_empty_string(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));
    /* Appending an empty string is a noop but must succeed */
    CHECK(cfrds_buffer_append(buf, "") == true);
    CHECK(cfrds_buffer_data_size(buf) == 0);
    cfrds_buffer_free(buf);
    return PASS;
}

/* ── Tests: append_char ────────────────────────────────────────────────── */

static int test_append_char(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));
    CHECK(cfrds_buffer_append_char(buf, 'A'));
    CHECK(cfrds_buffer_append_char(buf, 'B'));
    CHECK(cfrds_buffer_append_char(buf, 'C'));
    CHECK(cfrds_buffer_data_size(buf) == 3);
    CHECK(memcmp(cfrds_buffer_data(buf), "ABC", 3) == 0);
    cfrds_buffer_free(buf);
    return PASS;
}

static int test_append_char_null(void)
{
    CHECK(cfrds_buffer_append_char(NULL, 'X') == false);
    return PASS;
}

/* ── Tests: append_int ─────────────────────────────────────────────────── */

static int test_append_int_positive(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));
    CHECK(cfrds_buffer_append_int(buf, 42));
    CHECK(cfrds_buffer_data_size(buf) == 2);
    CHECK(memcmp(cfrds_buffer_data(buf), "42", 2) == 0);
    cfrds_buffer_free(buf);
    return PASS;
}

static int test_append_int_zero(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));
    CHECK(cfrds_buffer_append_int(buf, 0));
    CHECK(cfrds_buffer_data_size(buf) == 1);
    CHECK(memcmp(cfrds_buffer_data(buf), "0", 1) == 0);
    cfrds_buffer_free(buf);
    return PASS;
}

static int test_append_int_negative(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));
    CHECK(cfrds_buffer_append_int(buf, -7));
    CHECK(cfrds_buffer_data_size(buf) == 2);
    CHECK(memcmp(cfrds_buffer_data(buf), "-7", 2) == 0);
    cfrds_buffer_free(buf);
    return PASS;
}

/* ── Tests: append_bytes ───────────────────────────────────────────────── */

static int test_append_bytes(void)
{
    cfrds_buffer *buf = NULL;
    const uint8_t data[] = {0x00, 0x01, 0xFF, 0xFE};
    CHECK(cfrds_buffer_create(&buf));
    CHECK(cfrds_buffer_append_bytes(buf, data, sizeof(data)));
    CHECK(cfrds_buffer_data_size(buf) == 4);
    CHECK(memcmp(cfrds_buffer_data(buf), data, 4) == 0);
    cfrds_buffer_free(buf);
    return PASS;
}

static int test_append_bytes_null_data(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));
    CHECK(cfrds_buffer_append_bytes(buf, NULL, 4) == false);
    cfrds_buffer_free(buf);
    return PASS;
}

static int test_append_bytes_zero_length(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));
    /* Zero-length is treated as invalid — returns false but no crash */
    (void)cfrds_buffer_append_bytes(buf, "x", 0);
    cfrds_buffer_free(buf);
    return PASS;
}

/* ── Tests: append_buffer ──────────────────────────────────────────────── */

static int test_append_buffer(void)
{
    cfrds_buffer *a = NULL, *b = NULL;
    CHECK(cfrds_buffer_create(&a));
    CHECK(cfrds_buffer_create(&b));

    CHECK(cfrds_buffer_append(a, "foo"));
    CHECK(cfrds_buffer_append(b, "bar"));
    CHECK(cfrds_buffer_append_buffer(a, b));
    CHECK(cfrds_buffer_data_size(a) == 6);
    CHECK(memcmp(cfrds_buffer_data(a), "foobar", 6) == 0);

    cfrds_buffer_free(a);
    cfrds_buffer_free(b);
    return PASS;
}

static int test_append_buffer_null(void)
{
    cfrds_buffer *a = NULL;
    CHECK(cfrds_buffer_create(&a));
    CHECK(cfrds_buffer_append_buffer(NULL, a) == false);
    CHECK(cfrds_buffer_append_buffer(a, NULL) == false);
    cfrds_buffer_free(a);
    return PASS;
}

/* ── Tests: RDS wire-format helpers ────────────────────────────────────── */

static int test_append_rds_count(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));
    CHECK(cfrds_buffer_append_rds_count(buf, 3));
    /* Expected: "3:" */
    CHECK(cfrds_buffer_data_size(buf) == 2);
    CHECK(memcmp(cfrds_buffer_data(buf), "3:", 2) == 0);
    cfrds_buffer_free(buf);
    return PASS;
}

static int test_append_rds_string(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));
    CHECK(cfrds_buffer_append_rds_string(buf, "hello"));
    /* Expected: "STR:5:hello" */
    const char *expected = "STR:5:hello";
    CHECK(cfrds_buffer_data_size(buf) == strlen(expected));
    CHECK(memcmp(cfrds_buffer_data(buf), expected, strlen(expected)) == 0);
    cfrds_buffer_free(buf);
    return PASS;
}

static int test_append_rds_string_empty(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));
    CHECK(cfrds_buffer_append_rds_string(buf, ""));
    /* Expected: "STR:0:" */
    const char *expected = "STR:0:";
    CHECK(cfrds_buffer_data_size(buf) == strlen(expected));
    CHECK(memcmp(cfrds_buffer_data(buf), expected, strlen(expected)) == 0);
    cfrds_buffer_free(buf);
    return PASS;
}

static int test_append_rds_bytes(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));
    const char payload[] = {0x01, 0x02, 0x03};
    CHECK(cfrds_buffer_append_rds_bytes(buf, payload, sizeof(payload)));
    /* Expected: "STR:3:<3 raw bytes>" */
    const char *d = cfrds_buffer_data(buf);
    CHECK(cfrds_buffer_data_size(buf) == 9); /* "STR:3:" (6) + 3 bytes */
    CHECK(memcmp(d, "STR:3:", 6) == 0);
    CHECK(memcmp(d + 6, payload, 3) == 0);
    cfrds_buffer_free(buf);
    return PASS;
}

/* ── Tests: parse_number ───────────────────────────────────────────────── */

static int test_parse_number_basic(void)
{
    const char *data = "42:rest";
    size_t remaining = 7;
    int64_t out = 0;
    CHECK(cfrds_buffer_parse_number(&data, &remaining, &out));
    CHECK(out == 42);
    CHECK(remaining == 4);           /* "rest" */
    CHECK(strcmp(data, "rest") == 0);
    return PASS;
}

static int test_parse_number_zero(void)
{
    const char *data = "0:";
    size_t remaining = 2;
    int64_t out = -1;
    CHECK(cfrds_buffer_parse_number(&data, &remaining, &out));
    CHECK(out == 0);
    CHECK(remaining == 0);
    return PASS;
}

static int test_parse_number_negative(void)
{
    const char *data = "-5:";
    size_t remaining = 3;
    int64_t out = 0;
    CHECK(cfrds_buffer_parse_number(&data, &remaining, &out));
    CHECK(out == -5);
    return PASS;
}

static int test_parse_number_no_colon(void)
{
    const char *data = "42";
    size_t remaining = 2;
    int64_t out = 0;
    CHECK(cfrds_buffer_parse_number(&data, &remaining, &out) == false);
    return PASS;
}

static int test_parse_number_null(void)
{
    size_t remaining = 0;
    int64_t out = 0;
    CHECK(cfrds_buffer_parse_number(NULL, &remaining, &out) == false);
    return PASS;
}

/* ── Tests: parse_string ───────────────────────────────────────────────── */

static int test_parse_string_basic(void)
{
    /* Format: "<len>:<payload>" — the length prefix determines bytes read */
    const char *data = "5:hello";
    size_t remaining = 7;
    char *out = NULL;
    CHECK(cfrds_buffer_parse_string(&data, &remaining, &out));
    CHECK(out != NULL);
    CHECK(strcmp(out, "hello") == 0);
    CHECK(remaining == 0);
    free(out);
    return PASS;
}

static int test_parse_string_empty(void)
{
    const char *data = "0:";
    size_t remaining = 2;
    char *out = NULL;
    CHECK(cfrds_buffer_parse_string(&data, &remaining, &out));
    CHECK(out != NULL);
    CHECK(out[0] == '\0');
    CHECK(remaining == 0);
    free(out);
    return PASS;
}

static int test_parse_string_null_out(void)
{
    const char *data = "3:abc";
    size_t remaining = 5;
    CHECK(cfrds_buffer_parse_string(&data, &remaining, NULL) == false);
    return PASS;
}

static int test_parse_string_truncated(void)
{
    /* remaining says there are only 2 bytes left, but length says 5 */
    const char *data = "5:hi";
    size_t remaining = 4;
    char *out = NULL;
    CHECK(cfrds_buffer_parse_string(&data, &remaining, &out) == false);
    return PASS;
}

/* ── Tests: parse_bytearray ────────────────────────────────────────────── */

static int test_parse_bytearray_basic(void)
{
    const uint8_t raw[] = {0xDE, 0xAD, 0xBE, 0xEF};
    /* Build "4:<raw bytes>" */
    char input[6];
    input[0] = '4'; input[1] = ':';
    memcpy(input + 2, raw, 4);

    const char *data = input;
    size_t remaining = sizeof(input);
    char *out = NULL;
    int out_size = 0;

    CHECK(cfrds_buffer_parse_bytearray(&data, &remaining, &out, &out_size));
    CHECK(out != NULL);
    CHECK(out_size == 4);
    CHECK(memcmp(out, raw, 4) == 0);
    CHECK(remaining == 0);
    free(out);
    return PASS;
}

static int test_parse_bytearray_null_out(void)
{
    const char *data = "2:ab";
    size_t remaining = 4;
    int out_size = 0;
    CHECK(cfrds_buffer_parse_bytearray(&data, &remaining, NULL, &out_size) == false);
    return PASS;
}

/* ── Tests: large data / growth ────────────────────────────────────────── */

static int test_large_append(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));

    /* Append 10 000 'x' characters one at a time to exercise realloc */
    for (int i = 0; i < 10000; i++) {
        CHECK(cfrds_buffer_append_char(buf, 'x'));
    }
    CHECK(cfrds_buffer_data_size(buf) == 10000);

    const char *d = cfrds_buffer_data(buf);
    for (int i = 0; i < 10000; i++) {
        CHECK(d[i] == 'x');
    }

    cfrds_buffer_free(buf);
    return PASS;
}

/* ── Tests: null-sentinel beyond data ──────────────────────────────────── */

static int test_null_sentinel(void)
{
    cfrds_buffer *buf = NULL;
    CHECK(cfrds_buffer_create(&buf));
    CHECK(cfrds_buffer_append(buf, "abc"));
    /* The byte just past the data must be '\0' (null sentinel) */
    const char *d = cfrds_buffer_data(buf);
    CHECK(d[3] == '\0');
    cfrds_buffer_free(buf);
    return PASS;
}

/* ── main ──────────────────────────────────────────────────────────────── */

int main(void)
{
    /* create / free */
    RUN(test_create_null_out);
    RUN(test_create_and_free);
    RUN(test_free_null);
    RUN(test_data_null_buffer);

    /* append string */
    RUN(test_append_basic);
    RUN(test_append_null_buffer);
    RUN(test_append_null_str);
    RUN(test_append_empty_string);

    /* append_char */
    RUN(test_append_char);
    RUN(test_append_char_null);

    /* append_int */
    RUN(test_append_int_positive);
    RUN(test_append_int_zero);
    RUN(test_append_int_negative);

    /* append_bytes */
    RUN(test_append_bytes);
    RUN(test_append_bytes_null_data);
    RUN(test_append_bytes_zero_length);

    /* append_buffer */
    RUN(test_append_buffer);
    RUN(test_append_buffer_null);

    /* RDS helpers */
    RUN(test_append_rds_count);
    RUN(test_append_rds_string);
    RUN(test_append_rds_string_empty);
    RUN(test_append_rds_bytes);

    /* parse_number */
    RUN(test_parse_number_basic);
    RUN(test_parse_number_zero);
    RUN(test_parse_number_negative);
    RUN(test_parse_number_no_colon);
    RUN(test_parse_number_null);

    /* parse_string */
    RUN(test_parse_string_basic);
    RUN(test_parse_string_empty);
    RUN(test_parse_string_null_out);
    RUN(test_parse_string_truncated);

    /* parse_bytearray */
    RUN(test_parse_bytearray_basic);
    RUN(test_parse_bytearray_null_out);

    /* growth / sentinel */
    RUN(test_large_append);
    RUN(test_null_sentinel);

    printf("\n%d test(s) failed.\n", _failures);
    return _failures ? 1 : 0;
}
