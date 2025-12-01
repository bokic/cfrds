#pragma once

#include <stdbool.h>


#ifdef libcfrds_EXPORTS
#if defined(_MSC_VER)
#define EXPORT_WDDX __declspec(dllexport)
#else
#define EXPORT_WDDX __attribute__((visibility("default")))
#endif
#else
#define EXPORT_WDDX
#endif

typedef void WDDX;
typedef void WDDX_NODE;

#define WDDX_defer(var) WDDX* var __attribute__((cleanup(wddx_cleanup))) = nullptr

#ifdef __cplusplus
extern "C"
{
#endif

EXPORT_WDDX WDDX *wddx_create();
EXPORT_WDDX bool wddx_put_bool(WDDX *dest, const char *path, bool value);
EXPORT_WDDX bool wddx_put_number(WDDX *dest, const char *path, double value);
EXPORT_WDDX bool wddx_put_string(WDDX *dest, const char *path, const char *value);
EXPORT_WDDX const char *wddx_to_xml(WDDX *src);

EXPORT_WDDX WDDX *wddx_from_xml(const char *xml);
EXPORT_WDDX bool wddx_get_bool(const WDDX *src, const char *path, bool *ok);
EXPORT_WDDX double wddx_get_number(const WDDX *src, const char *path, bool *ok);
EXPORT_WDDX const char *wddx_get_string(const WDDX *src, const char *path);
EXPORT_WDDX const WDDX_NODE *wddx_get_var(const WDDX *src, const char *path);

EXPORT_WDDX char *wddx_node_to_xml(const WDDX_NODE *node);

EXPORT_WDDX void wddx_cleanup(WDDX **value);

#ifdef __cplusplus
}
#endif
