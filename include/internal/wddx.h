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

#define WDDX_defer(var) WDDX* var __attribute__((cleanup(wddx_cleanup))) = nullptr

EXPORT_WDDX WDDX *wddx_create();
EXPORT_WDDX bool wddx_put_bool(WDDX *dest, const char *path, bool value);
EXPORT_WDDX bool wddx_put_number(WDDX *dest, const char *path, double value);
EXPORT_WDDX bool wddx_put_string(WDDX *dest, const char *path, const char *value);
EXPORT_WDDX const char *wddx_to_string(WDDX *src);
EXPORT_WDDX void wddx_cleanup(WDDX **value);
