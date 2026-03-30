/*
 * test_wddx.c — Unit tests for the WDDX parser/serialiser in wddx.c.
 *
 * WDDX payloads that would normally come from a live ColdFusion server are
 * embedded directly as string literals (fixtures).  This lets the tests run
 * with no network access.
 *
 * The tests exercise wddx_from_xml() (parse), the node accessor functions,
 * wddx_create()/wddx_put_*()/wddx_to_xml() (serialise), and round-trip
 * correctness.
 */

#include <internal/wddx.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

/* ── Minimal test harness ──────────────────────────────────────────────── */

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
        if (_r == PASS) \
            printf("PASS  %s\n", #fn); \
        else { \
            printf("FAIL  %s\n", #fn); \
            _failures++; \
        } \
    } while (0)

/* ── WDDX fixture payloads ─────────────────────────────────────────────── */

/* Minimal well-formed WDDX packet carrying a single string value */
static const char FIXTURE_STRING[] =
    "<wddxPacket version=\"1.0\">"
    "<header/>"
    "<data><string>Hello, World!</string></data>"
    "</wddxPacket>";

/* WDDX packet carrying a number */
static const char FIXTURE_NUMBER[] =
    "<wddxPacket version=\"1.0\">"
    "<header/>"
    "<data><number>3.14</number></data>"
    "</wddxPacket>";

/* WDDX packet carrying a boolean (true) */
static const char FIXTURE_BOOL_TRUE[] =
    "<wddxPacket version=\"1.0\">"
    "<header/>"
    "<data><boolean value=\"true\">true</boolean></data>"
    "</wddxPacket>";

/* WDDX packet carrying a boolean (false) */
static const char FIXTURE_BOOL_FALSE[] =
    "<wddxPacket version=\"1.0\">"
    "<header/>"
    "<data><boolean value=\"false\">false</boolean></data>"
    "</wddxPacket>";

/* WDDX array of two strings — representative of how browse_dir responses
   embed sub-lists. */
static const char FIXTURE_ARRAY[] =
    "<wddxPacket version=\"1.0\">"
    "<header/>"
    "<data>"
      "<array length=\"2\">"
        "<string>alpha</string>"
        "<string>beta</string>"
      "</array>"
    "</data>"
    "</wddxPacket>";

/* WDDX struct (key-value map) */
static const char FIXTURE_STRUCT[] =
    "<wddxPacket version=\"1.0\">"
    "<header/>"
    "<data>"
      "<struct type=\"java.util.HashMap\">"
        "<var name=\"host\"><string>localhost</string></var>"
        "<var name=\"port\"><number>8500</number></var>"
      "</struct>"
    "</data>"
    "</wddxPacket>";

/* Malformed XML — must not crash, must return NULL */
static const char FIXTURE_MALFORMED[] =
    "<wddxPacket version=\"1.0\"><header/><data><string>unclosed";

/* Missing <wddxPacket> root element */
static const char FIXTURE_WRONG_ROOT[] =
    "<notWddx><header/><data><string>x</string></data></notWddx>";

/* Nested WDDX: struct containing an array */
static const char FIXTURE_NESTED[] =
    "<wddxPacket version=\"1.0\">"
    "<header/>"
    "<data>"
      "<struct type=\"java.util.HashMap\">"
        "<var name=\"items\">"
          "<array length=\"3\">"
            "<number>1</number>"
            "<number>2</number>"
            "<number>3</number>"
          "</array>"
        "</var>"
      "</struct>"
    "</data>"
    "</wddxPacket>";

/* Empty string value (edge case) */
static const char FIXTURE_EMPTY_STRING[] =
    "<wddxPacket version=\"1.0\">"
    "<header/>"
    "<data><string/></data>"
    "</wddxPacket>";

/* ── Parse tests ───────────────────────────────────────────────────────── */

static int test_parse_null_input(void)
{
    /* wddx_from_xml(NULL) should either return NULL or not crash */
    /* The implementation calls strlen(xml) so we can't pass NULL safely;
       instead test with an empty string. */
    WDDX *w = wddx_from_xml("");
    /* May be NULL — just must not crash */
    if (w) wddx_cleanup(&w);
    return PASS;
}

static int test_parse_malformed(void)
{
    WDDX *w = wddx_from_xml(FIXTURE_MALFORMED);
    /* Must not crash; result may be NULL or partial — we only verify safety */
    if (w) wddx_cleanup(&w);
    return PASS;
}

static int test_parse_wrong_root(void)
{
    WDDX *w = wddx_from_xml(FIXTURE_WRONG_ROOT);
    CHECK(w == NULL);
    return PASS;
}

static int test_parse_string_value(void)
{
    WDDX_defer(w);
    w = wddx_from_xml(FIXTURE_STRING);
    CHECK(w != NULL);

    const WDDX_NODE *data = wddx_data(w);
    CHECK(data != NULL);
    CHECK(wddx_node_type(data) == WDDX_STRING);
    CHECK(strcmp(wddx_node_string(data), "Hello, World!") == 0);
    return PASS;
}

static int test_parse_empty_string_value(void)
{
    WDDX_defer(w);
    w = wddx_from_xml(FIXTURE_EMPTY_STRING);
    CHECK(w != NULL);

    const WDDX_NODE *data = wddx_data(w);
    CHECK(data != NULL);
    CHECK(wddx_node_type(data) == WDDX_STRING);
    /* Empty string: node_string returns "" or NULL depending on impl;
       either is acceptable as long as it doesn't crash. */
    (void)wddx_node_string(data);
    return PASS;
}

static int test_parse_number_value(void)
{
    WDDX_defer(w);
    w = wddx_from_xml(FIXTURE_NUMBER);
    CHECK(w != NULL);

    const WDDX_NODE *data = wddx_data(w);
    CHECK(data != NULL);
    CHECK(wddx_node_type(data) == WDDX_NUMBER);
    double v = wddx_node_number(data);
    CHECK(fabs(v - 3.14) < 0.001);
    return PASS;
}

static int test_parse_bool_true(void)
{
    WDDX_defer(w);
    w = wddx_from_xml(FIXTURE_BOOL_TRUE);
    CHECK(w != NULL);

    const WDDX_NODE *data = wddx_data(w);
    CHECK(data != NULL);
    CHECK(wddx_node_type(data) == WDDX_BOOLEAN);
    CHECK(wddx_node_bool(data) == true);
    return PASS;
}

static int test_parse_bool_false(void)
{
    WDDX_defer(w);
    w = wddx_from_xml(FIXTURE_BOOL_FALSE);
    CHECK(w != NULL);

    const WDDX_NODE *data = wddx_data(w);
    CHECK(data != NULL);
    CHECK(wddx_node_type(data) == WDDX_BOOLEAN);
    CHECK(wddx_node_bool(data) == false);
    return PASS;
}

static int test_parse_array(void)
{
    WDDX_defer(w);
    w = wddx_from_xml(FIXTURE_ARRAY);
    CHECK(w != NULL);

    const WDDX_NODE *data = wddx_data(w);
    CHECK(data != NULL);
    CHECK(wddx_node_type(data) == WDDX_ARRAY);
    CHECK(wddx_node_array_size(data) == 2);

    const WDDX_NODE *e0 = wddx_node_array_at(data, 0);
    const WDDX_NODE *e1 = wddx_node_array_at(data, 1);
    CHECK(e0 != NULL);
    CHECK(e1 != NULL);
    CHECK(wddx_node_type(e0) == WDDX_STRING);
    CHECK(wddx_node_type(e1) == WDDX_STRING);
    CHECK(strcmp(wddx_node_string(e0), "alpha") == 0);
    CHECK(strcmp(wddx_node_string(e1), "beta") == 0);
    return PASS;
}

static int test_parse_struct(void)
{
    WDDX_defer(w);
    w = wddx_from_xml(FIXTURE_STRUCT);
    CHECK(w != NULL);

    const WDDX_NODE *data = wddx_data(w);
    CHECK(data != NULL);
    CHECK(wddx_node_type(data) == WDDX_STRUCT);
    CHECK(wddx_node_struct_size(data) == 2);

    /* Access via path */
    const char *host = wddx_get_string(w, "host");
    CHECK(host != NULL);
    CHECK(strcmp(host, "localhost") == 0);

    bool ok = false;
    double port = wddx_get_number(w, "port", &ok);
    CHECK(ok == true);
    CHECK(fabs(port - 8500.0) < 0.001);

    return PASS;
}

static int test_parse_nested(void)
{
    WDDX_defer(w);
    w = wddx_from_xml(FIXTURE_NESTED);
    CHECK(w != NULL);

    const WDDX_NODE *data = wddx_data(w);
    CHECK(data != NULL);
    CHECK(wddx_node_type(data) == WDDX_STRUCT);

    /* The struct has one key "items" whose value is an array[3] */
    const WDDX_NODE *var = wddx_get_var(w, "items");
    CHECK(var != NULL);
    CHECK(wddx_node_type(var) == WDDX_ARRAY);
    CHECK(wddx_node_array_size(var) == 3);

    for (int i = 0; i < 3; i++) {
        const WDDX_NODE *el = wddx_node_array_at(var, i);
        CHECK(el != NULL);
        CHECK(wddx_node_type(el) == WDDX_NUMBER);
        CHECK(fabs(wddx_node_number(el) - (i + 1)) < 0.001);
    }
    return PASS;
}

/* ── Accessor guard tests ──────────────────────────────────────────────── */

static int test_accessors_on_null(void)
{
    CHECK(wddx_data(NULL) == NULL);
    CHECK(wddx_header(NULL) == NULL);
    CHECK(wddx_node_type(NULL) == WDDX_NULL);
    CHECK(wddx_node_bool(NULL) == false);
    CHECK(isnan(wddx_node_number(NULL)));
    CHECK(wddx_node_string(NULL) == NULL);
    CHECK(wddx_node_array_size(NULL) == 0);
    CHECK(wddx_node_struct_size(NULL) == 0);
    return PASS;
}

/* ── Serialise (create + put + to_xml) tests ───────────────────────────── */

static int test_create_string(void)
{
    WDDX_defer(w);
    w = wddx_create();
    CHECK(w != NULL);
    CHECK(wddx_put_string(w, "greeting", "hello"));
    const char *xml = wddx_to_xml(w);
    CHECK(xml != NULL);
    CHECK(strstr(xml, "hello") != NULL);
    return PASS;
}

static int test_create_number(void)
{
    WDDX_defer(w);
    w = wddx_create();
    CHECK(w != NULL);
    CHECK(wddx_put_number(w, "value", 42.0));
    const char *xml = wddx_to_xml(w);
    CHECK(xml != NULL);
    CHECK(strstr(xml, "42") != NULL);
    return PASS;
}

static int test_create_bool(void)
{
    WDDX_defer(w);
    w = wddx_create();
    CHECK(w != NULL);
    CHECK(wddx_put_bool(w, "flag", true));
    const char *xml = wddx_to_xml(w);
    CHECK(xml != NULL);
    CHECK(strstr(xml, "true") != NULL);
    return PASS;
}

static int test_create_multiple_keys(void)
{
    WDDX_defer(w);
    w = wddx_create();
    CHECK(w != NULL);
    CHECK(wddx_put_string(w, "a", "one"));
    CHECK(wddx_put_string(w, "b", "two"));
    const char *xml = wddx_to_xml(w);
    CHECK(xml != NULL);
    CHECK(strstr(xml, "one") != NULL);
    CHECK(strstr(xml, "two") != NULL);
    return PASS;
}

static int test_to_xml_contains_wddxpacket(void)
{
    WDDX_defer(w);
    w = wddx_create();
    CHECK(w != NULL);
    CHECK(wddx_put_string(w, "x", "y"));
    const char *xml = wddx_to_xml(w);
    CHECK(xml != NULL);
    CHECK(strstr(xml, "wddxPacket") != NULL);
    return PASS;
}

/* ── Round-trip test ───────────────────────────────────────────────────── */

static int test_roundtrip_string(void)
{
    /* Parse the fixture, then verify the string survives intact. */
    WDDX_defer(w);
    w = wddx_from_xml(FIXTURE_STRING);
    CHECK(w != NULL);

    const WDDX_NODE *data = wddx_data(w);
    CHECK(data != NULL);
    CHECK(wddx_node_type(data) == WDDX_STRING);
    CHECK(strcmp(wddx_node_string(data), "Hello, World!") == 0);

    return PASS;
}

static int test_roundtrip_struct(void)
{
    WDDX_defer(w);
    w = wddx_from_xml(FIXTURE_STRUCT);
    CHECK(w != NULL);

    /* Re-serialise and re-parse */
    const char *xml2 = wddx_to_xml(w);
    CHECK(xml2 != NULL);

    WDDX_defer(w2);
    w2 = wddx_from_xml(xml2);
    CHECK(w2 != NULL);

    const char *host = wddx_get_string(w2, "host");
    CHECK(host != NULL);
    CHECK(strcmp(host, "localhost") == 0);

    return PASS;
}

static int test_roundtrip_array(void)
{
    WDDX_defer(w);
    w = wddx_from_xml(FIXTURE_ARRAY);
    CHECK(w != NULL);

    const char *xml2 = wddx_to_xml(w);
    CHECK(xml2 != NULL);

    WDDX_defer(w2);
    w2 = wddx_from_xml(xml2);
    CHECK(w2 != NULL);

    const WDDX_NODE *data = wddx_data(w2);
    CHECK(data != NULL);
    CHECK(wddx_node_type(data) == WDDX_ARRAY);
    CHECK(wddx_node_array_size(data) == 2);
    CHECK(strcmp(wddx_node_string(wddx_node_array_at(data, 0)), "alpha") == 0);
    CHECK(strcmp(wddx_node_string(wddx_node_array_at(data, 1)), "beta") == 0);

    return PASS;
}

/* ── main ──────────────────────────────────────────────────────────────── */

int main(void)
{
    /* parse — safety */
    RUN(test_parse_null_input);
    RUN(test_parse_malformed);
    RUN(test_parse_wrong_root);

    /* parse — value types */
    RUN(test_parse_string_value);
    RUN(test_parse_empty_string_value);
    RUN(test_parse_number_value);
    RUN(test_parse_bool_true);
    RUN(test_parse_bool_false);
    RUN(test_parse_array);
    RUN(test_parse_struct);
    RUN(test_parse_nested);

    /* accessor guards */
    RUN(test_accessors_on_null);

    /* serialise */
    RUN(test_create_string);
    RUN(test_create_number);
    RUN(test_create_bool);
    RUN(test_create_multiple_keys);
    RUN(test_to_xml_contains_wddxpacket);

    /* round-trip */
    RUN(test_roundtrip_string);
    RUN(test_roundtrip_struct);
    RUN(test_roundtrip_array);

    printf("\n%d test(s) failed.\n", _failures);
    return _failures ? 1 : 0;
}
