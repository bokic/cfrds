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
/**
 * @brief Creates a new, empty WDDX packet.
 * 
 * Allocates memory for a `WDDX` struct (containing pointer fields for `header`, `data`, and 
 * `str` of type `xmlBufferPtr`) and clears it using explicit_bzero.
 * 
 * @return A newly allocated `WDDX*`, or `NULL` on allocation failure.
 */
WDDX *wddx_create(void);

/**
 * @brief Inserts a boolean value into the WDDX data structure at the given path.
 * 
 * Traverses or constructs a hierarchy of nested array and struct nodes recursively based on the 
 * comma-separated path.
 * - If a segment is numeric (determined by checking up to 20 digits via is_string_numeric), it is 
 *   treated as a 0-based array index. It allocates or reallocates the child items list array size
 *   as needed to accommodate the index, zeroing the new elements.
 * - Otherwise, the segment is treated as a struct variable name, and search is done on the existing
 *   structure variables. If not found, a new struct element is allocated and added to the end of the items list.
 * Values are stored under a newly allocated `WDDX_NODE` with type `WDDX_BOOLEAN` and string representation `"true"` or `"false"`.
 * 
 * @param dest The target WDDX packet.
 * @param path Comma-separated path string specifying structural position (e.g. "0,foo,2").
 * @param value The boolean value to insert.
 * @return true on success, false on allocation failure, index out of bounds, or invalid type mismatch.
 */
bool wddx_put_bool(WDDX *dest, const char *path, bool value);

/**
 * @brief Inserts a numeric (double) value into the WDDX data structure at the given path.
 * 
 * Formats the number as a string via `snprintf(valueStr, sizeof(valueStr), "%.16g", value)` and 
 * invokes path traversal to insert a `WDDX_NODE` with type `WDDX_NUMBER` containing the string.
 * 
 * @param dest The target WDDX packet.
 * @param path Comma-separated path string.
 * @param value The double value to insert.
 * @return true on success, false on traversal or allocation failure.
 */
bool wddx_put_number(WDDX *dest, const char *path, double value);

/**
 * @brief Inserts a string value into the WDDX data structure at the given path.
 * 
 * Invokes path traversal to insert a `WDDX_NODE` of type `WDDX_STRING` with the string copied 
 * inside the struct's flexible string array.
 * 
 * @param dest The target WDDX packet.
 * @param path Comma-separated path string.
 * @param value Null-terminated string value to insert.
 * @return true on success, false on traversal or allocation failure.
 */
bool wddx_put_string(WDDX *dest, const char *path, const char *value);

/**
 * @brief Serializes the WDDX packet to XML format.
 * 
 * Frees any existing serialized buffer in `src->str`. Then uses libxml2 to build a new XML document:
 * - `<wddxPacket version="1.0">` is created as root.
 * - `<header>` child element is created and populated recursively from `src->header`.
 * - `<data>` child element is created and populated recursively from `src->data`.
 * XML node translation rules:
 * - `WDDX_NULL` -> `<null/>`
 * - `WDDX_BOOLEAN` -> `<boolean value="true|false"/>`
 * - `WDDX_NUMBER` -> `<number>...</number>`
 * - `WDDX_STRING` -> `<string>...</string>`
 * - `WDDX_ARRAY` -> `<array length="N">...</array>`
 * - `WDDX_STRUCT` -> `<struct type="java.util.HashMap">` containing `<var name="key">` children.
 * Dumps the document into `src->str` and returns its contents pointer.
 * 
 * @param src The WDDX packet.
 * @return Null-terminated string containing the serialized XML. Pointer is owned by `src`.
 */
const char *wddx_to_xml(WDDX *src);

/**
 * @brief Parses an XML string into a WDDX packet.
 * 
 * Uses libxml2's `xmlParseMemory` to parse the string. Silences error output in non-NDEBUG builds
 * via a custom generic error handler. Traverses the document structure to verify the root 
 * `<wddxPacket>` and its `<header>` and `<data>` children. Parses all nodes recursively mapping
 * elements to their respective internal types (`null`, `boolean`, `number`, `string`, `array`, `struct`).
 * - Arrays read their `length` property and parse child items up to that length.
 * - Structs scan for `<var>` elements, extracting `name` property and parsing their inner child.
 * 
 * @param xml Null-terminated string containing WDDX XML data.
 * @return A newly allocated WDDX structure containing the parsed trees, or NULL on parsing failure.
 */
WDDX *wddx_from_xml(const char *xml);



/**
 * @brief Retrieves the data root node of a WDDX packet.
 * 
 * @param src Pointer to WDDX structure (cast internally to WDDX*).
 * @return Pointer to WDDX_NODE representing data root, or NULL if src is NULL or has no data.
 */
const WDDX_NODE *wddx_data(const void *src);

/**
 * @brief Returns the type of a WDDX node.
 * 
 * @param value Pointer to WDDX_NODE (cast internally).
 * @return Node type enum value (e.g. WDDX_NULL, WDDX_BOOLEAN, etc.). Returns WDDX_NULL if value is NULL.
 */
int wddx_node_type(const void *value);





/**
 * @brief Extracts the string pointer from a WDDX_STRING node.
 * 
 * @param value Pointer to the node.
 * @return Const pointer to the string bytes. Pointer is owned by the node. Returns NULL if node is NULL.
 */
const char *wddx_node_string(const WDDX_NODE *value);

/**
 * @brief Returns the size of a WDDX_ARRAY node.
 * 
 * @param value Pointer to the array node.
 * @return The array capacity (cnt). Returns 0 if node is NULL.
 */
int wddx_node_array_size(const void *value);

/**
 * @brief Retrieves an element of a WDDX_ARRAY node by index.
 * 
 * Performs boundary check (`cnt < value->cnt`) and returns the child node.
 * 
 * @param value Pointer to the array node.
 * @param cnt 0-based index.
 * @return Pointer to the child node, or NULL if index is out of bounds or node is NULL.
 */
const WDDX_NODE *wddx_node_array_at(const void *value, size_t cnt);

/**
 * @brief Returns the number of variable fields in a WDDX_STRUCT node.
 * 
 * @param value Pointer to the struct node.
 * @return The count of fields. Returns 0 if node is NULL.
 */
int wddx_node_struct_size(const void *value);

/**
 * @brief Retrieves a structural key-value field of a WDDX_STRUCT node by list index.
 * 
 * Performs boundary checks, sets the output variable name string pointer, and returns the node value.
 * 
 * @param value Pointer to the struct node.
 * @param cnt 0-based index.
 * @param name Output pointer for the key string. Ignored if NULL.
 * @return Pointer to the value node, or NULL if index is out of bounds or node is NULL.
 */
const WDDX_NODE *wddx_node_struct_at(const void *value, size_t cnt, const char **name);



/**
 * @brief Query helper to retrieve a double value at a specific path.
 * 
 * Performs recursive traversal to locate the node at `path` and verifies its type is `WDDX_NUMBER`.
 * 
 * @param src Pointer to the WDDX structure.
 * @param path Comma-separated path.
 * @param ok Output parameter set to true on success, false on traversal or type mismatch.
 * @return The double value if successful, 0.0 otherwise.
 */
double wddx_get_number(const void *src, const char *path, bool *ok);

/**
 * @brief Query helper to retrieve a string at a specific path.
 * 
 * Performs recursive traversal to locate the node at `path` and verifies its type is `WDDX_STRING`.
 * 
 * @param src Pointer to the WDDX structure.
 * @param path Comma-separated path.
 * @return Pointer to the null-terminated string, or NULL if not found or type mismatch.
 */
const char *wddx_get_string(const void *src, const char *path);

/**
 * @brief Query helper to retrieve a node at a specific path.
 * 
 * Traverses structural nodes recursively matching indexes for array items and keys for struct fields.
 * 
 * @param src Pointer to the WDDX structure.
 * @param path Comma-separated path.
 * @return Const pointer to the located WDDX_NODE, or NULL if not found.
 */
const WDDX_NODE *wddx_get_var(const void *src, const char *path);



/**
 * @brief Recursively frees a WDDX packet structure and associated memory.
 * 
 * Destroys all structural nodes, internal variable names, values, and XML serializer buffer.
 * Finally, frees the WDDX container and sets the referenced pointer to NULL.
 * 
 * @param value Pointer to the WDDX* pointer to clean up.
 */
void wddx_cleanup(void *value);
