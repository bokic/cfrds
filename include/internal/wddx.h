#pragma once

#include <stdbool.h>
#include <stddef.h>


/**
 * @enum wddx_type
 * @brief Defines supported WDDX data types.
 *
 * These types correspond to the XML elements in the WDDX specification:
 * - `WDDX_NULL` → `<null/>`
 * - `WDDX_BOOLEAN` → `<boolean value="true|false"/>`
 * - `WDDX_NUMBER` → `<number>...</number>`
 * - `WDDX_STRING` → `<string>...</string>`
 * - `WDDX_ARRAY` → `<array length="N">...</array>`
 * - `WDDX_STRUCT` → `<struct type="java.util.HashMap">...</struct>`
 */
enum wddx_type {
    WDDX_NULL,
    WDDX_BOOLEAN,
    WDDX_NUMBER,
    WDDX_STRING,
    WDDX_ARRAY,
    WDDX_STRUCT
};

/**
 * @typedef WDDX
 * @brief Opaque handle to a WDDX packet (container for header + data).
 * @see wddx_create(), wddx_cleanup(), wddx_to_xml(), wddx_from_xml()
 */
typedef struct WDDX WDDX;

/**
 * @typedef WDDX_NODE
 * @brief Opaque handle to a single WDDX node (value of any type).
 * @see wddx_data(), wddx_header(), wddx_node_type(), wddx_get_var()
 */
typedef struct WDDX_NODE WDDX_NODE;

/**
 * @def WDDX_defer(var)
 * @brief RAII macro for automatic cleanup of WDDX pointers using GCC/Clang `__attribute__((cleanup))`.
 * @param var The variable name (e.g., `WDDX_defer(packet);` declares `WDDX *packet = NULL;`)
 * @note Requires `-fvisibility=hidden` or similar to avoid linker issues in shared libraries.
 * @see wddx_cleanup()
 */
#define WDDX_defer(var) WDDX* var __attribute__((cleanup(wddx_cleanup))) = NULL

/**
 * @brief Creates a new, empty WDDX packet.
 * @return A newly allocated `WDDX*`, or `NULL` on allocation failure.
 * @note The returned packet has `header = NULL` and `data = NULL`.
 *       Call `wddx_put_*()` functions to populate the data.
 * @note On error, returns `NULL` — no cleanup needed (nothing allocated).
 */
WDDX *wddx_create(void);

/**
 * @brief Inserts a boolean value into the WDDX data structure at the given path.
 * @param dest The target WDDX packet (must be non-`NULL`).
 * @param path Dot-separated *comma-separated* path string (e.g., `"0,foo,2"` or `"user,name"`).
 *             - Numeric segments (e.g., `"0"`, `"1"`) are treated as **0-based array indices**.
 *             - Non-numeric segments (e.g., `"foo"`) are treated as struct keys.
 *             - Paths like `""` (empty) are **not supported**; use `wddx_put_*` without path for root.
 * @param value The boolean value to insert.
 * @return `true` on success, `false` on allocation failure or invalid path.
 * @note Internally uses `atoi()` for numeric indices; values > `INT_MAX` cause undefined behavior.
 * @warning If a path segment is a large numeric string (e.g., `"9999999999"`), `atoi()` overflow may occur.
 */
bool wddx_put_bool(WDDX *dest, const char *path, bool value);

/**
 * @brief Inserts a numeric (double-precision floating-point) value at the given path.
 * @param dest Target WDDX packet (non-`NULL`).
 * @param path Path string (see @ref wddx_put_bool for format).
 * @param value The double value to insert.
 * @return `true` on success, `false` on error.
 * @note The value is converted to a string with `snprintf(..., "%.16g", value)`.
 * @warning If `path` contains numeric segments > `INT_MAX`, behavior is undefined.
 */
bool wddx_put_number(WDDX *dest, const char *path, double value);

/**
 * @brief Inserts a string value at the given path.
 * @param dest Target WDDX packet (non-`NULL`).
 * @param path Path string (see @ref wddx_put_bool for format).
 * @param value The string to insert (must be null-terminated; copied internally).
 * @return `true` on success, `false` on error.
 * @note The string is copied; caller retains ownership of `value`.
 * @warning Empty paths (`""`) are rejected by internal logic.
 * @warning If `path` contains numeric segments > `INT_MAX`, undefined behavior occurs.
 */
bool wddx_put_string(WDDX *dest, const char *path, const char *value);

/**
 * @brief Serializes the WDDX packet to XML (WDDX 1.0 format).
 * @param src The WDDX packet to serialize (non-`NULL`).
 * @return A null-terminated string containing the XML, or `NULL` on error.
 * @note The returned string is owned by `src` and is **invalidated** on:
 *       - Next call to `wddx_to_xml()` on the same `src`,
 *       - Calling `wddx_cleanup()`,
 *       - Modifying `src` (e.g., via `wddx_put_*`).
 * @note The XML includes a `<wddxPacket version="1.0">` root element, with `<header>` and `<data>` children.
 * @note The `<struct>` elements are serialized with `type="java.util.HashMap"` attribute.
 * @warning This function reuses an internal buffer; copy the result immediately if persistence is needed.
 */
const char *wddx_to_xml(WDDX *src);

/**
 * @brief Parses an XML string into a WDDX packet.
 * @param xml A null-terminated string containing WDDX XML (must start with `<wddxPacket version="1.0">`).
 * @return A newly allocated `WDDX*`, or `NULL` on parse failure or invalid XML.
 * @note The XML must contain a `<header>` and `<data>` section. Empty header is allowed.
 * @note XML elements are parsed as per `wddx_type`: `<null>`, `<boolean>`, `<number>`, `<string>`, `<array>`, `<struct>`.
 * @note Arrays must specify a `length` attribute; structs use `<var>` elements with `name` attributes.
 * @warning This function disables libxml2 error output (in non-`NDEBUG` builds) via `xmlSetGenericErrorFunc`.
 *          Not thread-safe if multiple threads call this or `xmlParseMemory()` concurrently.
 * @warning Does **not** support DTDs, external entities, or XInclude — but also does not enable them for security.
 */
WDDX *wddx_from_xml(const char *xml);

/**
 * @brief Retrieves the header node of a WDDX packet.
 * @param src The WDDX packet (non-`NULL`).
 * @return A pointer to the header `WDDX_NODE*`, or `NULL` if the packet has no header (i.e., header == NULL).
 * @note The header is read-only; modifications require creating a new header node.
 * @note Header content is ignored by `wddx_to_xml()` (but present in XML as `<header>`).
 */
const WDDX_NODE *wddx_header(const WDDX *src);

/**
 * @brief Retrieves the main data node of a WDDX packet.
 * @param src The WDDX packet (non-`NULL`).
 * @return A pointer to the data `WDDX_NODE*`, or `NULL` if no data has been inserted.
 */
const WDDX_NODE *wddx_data(const void *src);

/**
 * @brief Returns the type of a WDDX node.
 * @param value The node to query (may be `NULL`; returns `WDDX_NULL` for safety).
 * @return One of `wddx_type` values.
 * @note This is the primary way to determine how to interpret the node's content.
 */
int wddx_node_type(const void *value);

/**
 * @brief Extracts the boolean value from a `WDDX_BOOLEAN` node.
 * @param value The node to query (must be of type `WDDX_BOOLEAN` or `NULL`).
 * @return `true` if the node's boolean value is `true`, `false` otherwise (including on type mismatch or `NULL`).
 * @note Returns `false` on failure — use `wddx_node_type()` first to confirm type.
 */
bool wddx_node_bool(const WDDX_NODE *value);

/**
 * @brief Extracts the numeric value from a `WDDX_NUMBER` node.
 * @param value The node to query (must be of type `WDDX_NUMBER` or `NULL`).
 * @return The stored `double` value, or `0.0` (on `NULL` or type mismatch).
 * @note Use `isfinite()`, `isnan()` on the result to distinguish NaN/Inf.
 * @warning Returns `0.0` both when value is `0.0` and on error — always check type first.
 */
double wddx_node_number(const WDDX_NODE *value);

/**
 * @brief Extracts the string value from a `WDDX_STRING` node.
 * @param value The node to query (must be of type `WDDX_STRING` or `NULL`).
 * @return A null-terminated string, or `NULL` if node is `NULL` or not a string.
 * @note The returned string is owned by the node and must not be freed by the caller.
 * @warning Returns `NULL` for all non-string types — verify with `wddx_node_type()`.
 */
const char *wddx_node_string(const WDDX_NODE *value);

/**
 * @brief Returns the size (number of elements) of a `WDDX_ARRAY` node.
 * @param value The node to query (must be of type `WDDX_ARRAY` or `NULL`).
 * @return The array length (≥ 0), or `0` if node is `NULL` or not an array.
 * @note Does not distinguish between empty array and type mismatch — use `wddx_node_type()` first.
 */
int wddx_node_array_size(const void *value);

/**
 * @brief Retrieves an element from a `WDDX_ARRAY` node by index.
 * @param value The array node (must be of type `WDDX_ARRAY`).
 * @param cnt The 0-based index (must satisfy `0 <= cnt < wddx_node_array_size(value)`).
 * @return A pointer to the element node, or `NULL` on error.
 * @note Returns `NULL` for invalid index, `NULL` node, or non-array type.
 * @warning Does **not** validate `cnt` — caller must check bounds (or use `wddx_node_array_size()`).
 */
const WDDX_NODE *wddx_node_array_at(const void *value, size_t cnt);

/**
 * @brief Returns the number of fields in a `WDDX_STRUCT` node.
 * @param value The struct node (must be of type `WDDX_STRUCT` or `NULL`).
 * @return The number of struct fields (≥ 0), or `0` for non-struct or `NULL`.
 * @note Does not distinguish between empty struct and type mismatch.
 */
int wddx_node_struct_size(const void *value);

/**
 * @brief Retrieves a field from a `WDDX_STRUCT` node by index.
 * @param value The struct node (must be of type `WDDX_STRUCT`).
 * @param cnt The 0-based index (must satisfy `0 <= cnt < wddx_node_struct_size(value)`).
 * @param name Pointer to store the field's key name (output, may be `NULL`).
 * @return A pointer to the value node, or `NULL` on error.
 * @note If `name` is non-`NULL`, `*name` is set to the field's key (null-terminated, internal storage).
 * @warning Does not validate `cnt`; must be called with valid `cnt` < struct size.
 */
const WDDX_NODE *wddx_node_struct_at(const void *value, size_t cnt, const char **name);

/**
 * @brief Gets a boolean value from the data section at a given path.
 * @param src The WDDX packet (non-`NULL`).
 * @param path Path string (see @ref wddx_put_bool).
 * @param ok If non-`NULL`, set to `true` on success, `false` on failure (type mismatch, not found, etc.).
 * @return The boolean value if successful, `false` otherwise.
 * @note Always sets `*ok` to `false` on error — use `ok` parameter to disambiguate `false` vs. error.
 */
bool wddx_get_bool(const void *src, const char *path, bool *ok);

/**
 * @brief Gets a numeric value from the data section at a given path.
 * @param src The WDDX packet (non-`NULL`).
 * @param path Path string (see @ref wddx_put_bool).
 * @param ok If non-`NULL`, set to `true` on success, `false` on failure.
 * @return The `double` value if successful, `0.0` otherwise.
 * @note Always sets `*ok = false` on error — use to distinguish `0.0` vs. missing/invalid.
 */
double wddx_get_number(const void *src, const char *path, bool *ok);

/**
 * @brief Gets a string value from the data section at a given path.
 * @param src The WDDX packet (non-`NULL`).
 * @param path Path string (see @ref wddx_put_bool).
 * @return The string if found and of type `WDDX_STRING`, `NULL` otherwise.
 * @note Returns `NULL` for missing, wrong type, or allocation errors — use `wddx_get_var()` + `wddx_node_type()` for robustness.
 */
const char *wddx_get_string(const void *src, const char *path);

/**
 * @brief Retrieves the node at a given path in the data section, or `NULL`.
 * @param src The WDDX packet (non-`NULL`).
 * @param path Path string (see @ref wddx_put_bool).
 * @return A pointer to the node, or `NULL` if not found.
 * @note Allows inspection of the node (e.g., to check its type and access sub-values).
 */
const WDDX_NODE *wddx_get_var(const void *src, const char *path);

/**
 * @brief Serializes a single WDDX node (not the full packet) into XML.
 * @param node The node to serialize (non-`NULL`).
 * @return A newly allocated string containing XML (caller must `free()`), or `NULL` on error.
 * @note Generates `<var>` root element (e.g., `<var><string>hello</string></var>`).
 * @warning Does **not** produce WDDX-compliant packets — only for debugging or substructure serialization.
 * @note The `null` type is **not supported** — `WDDX_NULL` nodes are silently ignored.
 */
char *wddx_node_to_xml(const WDDX_NODE *node);

/**
 * @brief Cleanup function for WDDX pointers (used by `WDDX_defer` macro).
 * @param value Pointer to the WDDX pointer to clean up (may be `NULL`).
 * @note Frees the packet and all associated nodes (recursively), and XML buffer.
 * @note Sets `*value = NULL` after cleanup.
 */
void wddx_cleanup(void *value);
