#pragma once

#include <stdbool.h>


enum wddx_type {
    WDDX_BOOLEAN,
    WDDX_NUMBER,
    WDDX_STRING,
    WDDX_ARRAY,
    WDDX_STRUCT
};

typedef void WDDX;
typedef void WDDX_NODE;

#define WDDX_defer(var) WDDX* var __attribute__((cleanup(wddx_cleanup))) = NULL

WDDX *wddx_create();
bool wddx_put_bool(WDDX *dest, const char *path, bool value);
bool wddx_put_number(WDDX *dest, const char *path, double value);
bool wddx_put_string(WDDX *dest, const char *path, const char *value);
const char *wddx_to_xml(WDDX *src);

WDDX *wddx_from_xml(const char *xml);

const WDDX_NODE *wddx_header(const WDDX *src);
const WDDX_NODE *wddx_data(const WDDX *src);

int wddx_node_type(const WDDX_NODE *value);
bool wddx_node_bool(const WDDX_NODE *value);
double wddx_node_number(const WDDX_NODE *value);
const char *wddx_node_string(const WDDX_NODE *value);
int wddx_node_array_size(const WDDX_NODE *value);
const WDDX_NODE *wddx_node_array_at(const WDDX_NODE *value, int cnt);
int wddx_node_struct_size(const WDDX_NODE *value);
const WDDX_NODE *wddx_node_struct_at(const WDDX_NODE *value, int cnt, const char **name);

bool wddx_get_bool(const WDDX *src, const char *path, bool *ok);
double wddx_get_number(const WDDX *src, const char *path, bool *ok);
const char *wddx_get_string(const WDDX *src, const char *path);
const WDDX_NODE *wddx_get_var(const WDDX *src, const char *path);

char *wddx_node_to_xml(const WDDX_NODE *node);

void wddx_cleanup(WDDX **value);
