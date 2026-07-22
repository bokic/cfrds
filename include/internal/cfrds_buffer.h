#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#include <cfrds.h>


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
typedef SIZE_T size_t;
typedef SSIZE_T ssize_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

struct cfrds_server {
    char *host;
    uint16_t port;
    char *username;
    char *orig_password;
    char *password;
    int _errno;
    int64_t error_code;
    char *error;
};

struct cfrds_file_content {
    char *data;
    size_t size;
    char *modified;
    char *permission;
};

typedef struct {
    char kind;
    char *name;
    uint8_t permissions;
    size_t size;
    uint64_t modified;
} cfrds_browse_dir_item;

struct cfrds_browse_dir {
    size_t cnt;
    cfrds_browse_dir_item items[];
};

struct cfrds_sql_dsninfo {
    size_t cnt;
    char *names[];
};

typedef struct {
    char *unknown;
    char *schema;
    char *name;
    char *type;
} cfrds_sql_tableinfoitem;

struct cfrds_sql_tableinfo {
    size_t cnt;
    cfrds_sql_tableinfoitem items[];
};

typedef struct {
    char *schema;
    char *owner;
    char *table;
    char *name;
    int type;
    char *typeStr;
    int precision;
    int length;
    int scale;
    int radix;
    int nullable;
} cfrds_sql_columninfoitem;

struct cfrds_sql_columninfo {
    size_t cnt;
    cfrds_sql_columninfoitem items[];
};

typedef struct {
    char *tableCatalog;
    char *tableOwner;
    char *tableName;
    char *colName;
    int keySequence;
} cfrds_sql_primarykeysitem;

struct cfrds_sql_primarykeys {
    size_t cnt;
    cfrds_sql_primarykeysitem items[];
};

typedef struct {
    char *pkTableCatalog;
    char *pkTableOwner;
    char *pkTableName;
    char *pkColName;
    char *fkTableCatalog;
    char *fkTableOwner;
    char *fkTableName;
    char *fkColName;
    int keySequence;
    int updateRule;
    int deleteRule;
} cfrds_sql_foreignkeysitem;

struct cfrds_sql_foreignkeys {
    size_t cnt;
    cfrds_sql_foreignkeysitem items[];
};

typedef struct {
    char *pkTableCatalog;
    char *pkTableOwner;
    char *pkTableName;
    char *pkColName;
    char *fkTableCatalog;
    char *fkTableOwner;
    char *fkTableName;
    char *fkColName;
    int keySequence;
    int updateRule;
    int deleteRule;
} cfrds_sql_importedkeysitem;

struct cfrds_sql_importedkeys {
    size_t cnt;
    cfrds_sql_importedkeysitem items[];
};

typedef struct {
    char *pkTableCatalog;
    char *pkTableOwner;
    char *pkTableName;
    char *pkColName;
    char *fkTableCatalog;
    char *fkTableOwner;
    char *fkTableName;
    char *fkColName;
    int keySequence;
    int updateRule;
    int deleteRule;
} cfrds_sql_exportedkeysitem;

struct cfrds_sql_exportedkeys {
    size_t cnt;
    cfrds_sql_exportedkeysitem items[];
};

struct cfrds_sql_resultset {
    size_t columns;
    size_t rows;
    char *values[];
};

typedef struct {
    char *name;
    char *type;
    char *jtype;
} cfrds_sql_metadataitem;

struct cfrds_sql_metadata {
    size_t cnt;
    cfrds_sql_metadataitem items[];
};

struct cfrds_sql_supportedcommands {
    size_t cnt;
    char *commands[];
};

struct cfrds_security_analyzer_result {
    int id;
    int total_files;
    int files_visited_count;
    void *errors_description;
    void *files_scanned;
    void *files_not_scanned;
    char *executorservice;
    int percentage;
    char *files;
    int64_t last_updated;
    int files_scanned_count;
    int files_not_scanned_count;
    void *errors;
};

/**
 * @brief Allocates and initializes a new, empty cfrds_buffer.
 * 
 * Sets capacity to 0, data size to 0, and internal pointer to NULL.
 * 
 * @param buffer Output pointer where a pointer to the created cfrds_buffer is stored.
 * @return true on successful allocation, false if buffer is NULL or malloc fails.
 */
bool cfrds_buffer_create(cfrds_buffer **buffer);

/**
 * @brief Returns the internal data pointer of a cfrds_buffer.
 * 
 * @param buffer Pointer to the buffer.
 * @return A pointer to the character array inside the buffer, or NULL if buffer is NULL.
 *         Note that the data is guaranteed to have a null terminator past the end of the size boundary.
 */
char *cfrds_buffer_data(cfrds_buffer *buffer);

/**
 * @brief Returns the size of valid data in the cfrds_buffer.
 * 
 * @param buffer Pointer to the buffer.
 * @return The count of bytes currently active in the buffer. Returns 0 if buffer is NULL.
 */
size_t cfrds_buffer_data_size(cfrds_buffer *buffer);

/**
 * @brief Appends a null-terminated string to the cfrds_buffer.
 * 
 * Automatically reallocates internal storage using an exponential growth strategy if the 
 * append exceeds the current capacity.
 * 
 * @param buffer Destination buffer.
 * @param str Null-terminated string to append.
 * @return true on success, false if buffer/str is NULL or reallocation fails.
 */
bool cfrds_buffer_append(cfrds_buffer *buffer, const char *str);



/**
 * @brief Appends raw bytes of a specified length to the buffer.
 * 
 * Reallocates the buffer if necessary and copies the bytes.
 * 
 * @param buffer Destination buffer.
 * @param data Pointer to the bytes to copy.
 * @param length Number of bytes to copy.
 * @return true on success, false if buffer/data is NULL, length is 0, or reallocation fails.
 */
bool cfrds_buffer_append_bytes(cfrds_buffer *buffer, const void *data, size_t length);

/**
 * @brief Appends the content of another cfrds_buffer to the destination buffer.
 * 
 * Reallocates the destination buffer if necessary.
 * 
 * @param buffer Destination buffer.
 * @param new Source buffer whose data will be appended.
 * @return true on success, false if buffer/new is NULL or reallocation fails.
 */
bool cfrds_buffer_append_buffer(cfrds_buffer *buffer, cfrds_buffer *new);



/**
 * @brief Appends a count followed by a colon for RDS protocol list sizes.
 * 
 * Formats and appends `<cnt>:` to the buffer.
 * 
 * @param buffer Destination buffer.
 * @param cnt Size or element count value.
 * @return true on success, false if buffer is NULL or formatting/appending fails.
 */
bool cfrds_buffer_append_rds_count(cfrds_buffer *buffer, size_t cnt);

/**
 * @brief Appends a string formatted in the RDS protocol string representation.
 * 
 * Formats and appends `"STR:<len>:<string>"` to the buffer.
 * 
 * @param buffer Destination buffer.
 * @param str Null-terminated string to append.
 * @return true on success, false if buffer/str is NULL or appending fails.
 */
bool cfrds_buffer_append_rds_string(cfrds_buffer *buffer, const char *str);

/**
 * @brief Appends a byte array formatted in the RDS protocol representation.
 * 
 * Formats and appends `"STR:<length>:<data>"` to the buffer.
 * 
 * @param buffer Destination buffer.
 * @param data Pointer to raw bytes.
 * @param length Count of bytes.
 * @return true on success, false if buffer/data is NULL or appending fails.
 */
bool cfrds_buffer_append_rds_bytes(cfrds_buffer *buffer, const void *data, size_t length);

/**
 * @brief Reserves a specified amount of additional free space in the buffer.
 * 
 * If remaining capacity is less than size, it reallocates storage to `buffer->size + size + 1`
 * bytes and uses explicit_bzero to clear the newly allocated block.
 * 
 * @param buffer Target buffer.
 * @param size Minimum bytes of free space requested.
 * @return true on success, false if buffer is NULL or realloc fails.
 */
bool cfrds_buffer_reserve_above_size(cfrds_buffer *buffer, size_t size);

/**
 * @brief Expands the active data size of the buffer by `size` bytes.
 * 
 * Checks capacity and reserves space first, then advances the active size indicator.
 * 
 * @param buffer Target buffer.
 * @param size Number of bytes to expand the active range by.
 * @return true on success, false if buffer is NULL or reservation fails.
 */
bool cfrds_buffer_expand(cfrds_buffer *buffer, size_t size);

/**
 * @brief Frees all memory associated with the buffer.
 * 
 * Frees internal byte array and the cfrds_buffer container. Safe if buffer is NULL.
 * 
 * @param buffer Pointer to the buffer to free.
 */
void cfrds_buffer_free(cfrds_buffer *buffer);

/**
 * @brief Parses an RDS protocol base-10 number terminated by a colon.
 * 
 * Parses a decimal number, advances `*data` past the colon, and updates `*remaining` bytes.
 * 
 * @param data Input/output pointer to the cursor in the data block.
 * @param remaining Input/output pointer tracking remaining bytes.
 * @param out Output pointer where the parsed number is written.
 * @return true on success, false if parsing fails, colon is not found, or value is out of bounds.
 */
bool cfrds_buffer_parse_number(const char **data, size_t *remaining, int64_t *out);



/**
 * @brief Parses an RDS protocol string prefixed by its size.
 * 
 * Reads size, allocates `size + 1` bytes, copies characters, adds a null-terminator, 
 * and updates data parsing cursors.
 * 
 * @param data Input/output parsing cursor.
 * @param remaining Input/output remaining byte tracker.
 * @param out Output pointer populated with the allocated string. Must be freed by the caller.
 * @return true on success, false on parsing or allocation failure.
 */
bool cfrds_buffer_parse_string(const char **data, size_t *remaining, char **out);



/**
 * @brief Parses folder/file directory browsing results from the RDS server response.
 * 
 * Expects a field count divisible by 5. Parses item type ('F' or 'D'), filename, permissions, 
 * size, and modified date (translating ColdFusion ticks to Unix time). 
 * 
 * @param buffer Server response buffer.
 * @return Pointer to allocated `cfrds_browse_dir` containing parsed list. Must be freed by the caller.
 *         Returns NULL on parser errors or if count exceeds maximum limit (10000).
 */
struct cfrds_browse_dir *cfrds_buffer_to_browse_dir(cfrds_buffer *buffer);

/**
 * @brief Parses file contents response returned from RDS.
 * 
 * Expects exactly 3 fields (file contents, modification timestamp, permissions).
 * 
 * @param buffer Server response buffer.
 * @return Allocated `cfrds_file_content` struct containing parsed data and meta. Must be freed by caller.
 *         Returns NULL on parsing error.
 */
struct cfrds_file_content *cfrds_buffer_to_file_content(cfrds_buffer *buffer);

/**
 * @brief Parses SQL Data Source Names (DSN) from the RDS server response.
 * 
 * Parses DSN list count and DSN names (stripping quotes).
 * 
 * @param buffer Server response buffer.
 * @return Allocated `cfrds_sql_dsninfo` structure with DSN name list. Must be freed by the caller.
 *         Returns NULL on parse error.
 */
struct cfrds_sql_dsninfo *cfrds_buffer_to_sql_dsninfo(cfrds_buffer *buffer);

/**
 * @brief Parses database table metadata list from the RDS server response.
 * 
 * Parses database tables (schema, name, type, and an unknown first field) from comma-separated quoted lists.
 * 
 * @param buffer Server response buffer.
 * @return Allocated `cfrds_sql_tableinfo` containing table structures. Must be freed by the caller.
 *         Returns NULL on parsing error.
 */
struct cfrds_sql_tableinfo *cfrds_buffer_to_sql_tableinfo(cfrds_buffer *buffer);

/**
 * @brief Parses database columns metadata from the RDS server response.
 * 
 * Parses up to 12 attributes per column (schema, owner, table, name, type, typeStr, precision, 
 * length, scale, radix, nullable, etc.).
 * 
 * @param buffer Server response buffer.
 * @return Allocated `cfrds_sql_columninfo` structure. Must be freed by the caller.
 *         Returns NULL on parsing error.
 */
struct cfrds_sql_columninfo *cfrds_buffer_to_sql_columninfo(cfrds_buffer *buffer);

/**
 * @brief Parses primary keys metadata from the RDS server response.
 * 
 * Parses primary key attributes (tableCatalog, tableOwner, tableName, colName, keySequence).
 * 
 * @param buffer Server response buffer.
 * @return Allocated `cfrds_sql_primarykeys` structure. Must be freed by the caller.
 *         Returns NULL on parsing error.
 */
struct cfrds_sql_primarykeys *cfrds_buffer_to_sql_primarykeys(cfrds_buffer *buffer);

/**
 * @brief Parses foreign keys metadata from the RDS server response.
 * 
 * Parses pkTableCatalog, pkTableOwner, pkTableName, pkColName, fkTableCatalog, fkTableOwner, 
 * fkTableName, fkColName, keySequence, updateRule, deleteRule.
 * 
 * @param buffer Server response buffer.
 * @return Allocated `cfrds_sql_foreignkeys` structure. Must be freed by the caller.
 *         Returns NULL on parsing error.
 */
struct cfrds_sql_foreignkeys *cfrds_buffer_to_sql_foreignkeys(cfrds_buffer *buffer);

/**
 * @brief Parses imported keys metadata from the RDS server response.
 * 
 * Same structural format as foreign keys.
 * 
 * @param buffer Server response buffer.
 * @return Allocated `cfrds_sql_importedkeys` structure. Must be freed by the caller.
 *         Returns NULL on parsing error.
 */
struct cfrds_sql_importedkeys *cfrds_buffer_to_sql_importedkeys(cfrds_buffer *buffer);

/**
 * @brief Parses exported keys metadata from the RDS server response.
 * 
 * Same structural format as foreign keys.
 * 
 * @param buffer Server response buffer.
 * @return Allocated `cfrds_sql_exportedkeys` structure. Must be freed by the caller.
 *         Returns NULL on parsing error.
 */
struct cfrds_sql_exportedkeys *cfrds_buffer_to_sql_exportedkeys(cfrds_buffer *buffer);

/**
 * @brief Parses query sql statement resultset from the RDS server response.
 * 
 * Expects tabular list. The first row contains column headers. Determines the number of columns 
 * and rows, then builds a tabular grid of value strings.
 * 
 * @param buffer Server response buffer.
 * @return Allocated `cfrds_sql_resultset` representing rows and columns. Must be freed by the caller.
 *         Returns NULL on parse error.
 */
struct cfrds_sql_resultset *cfrds_buffer_to_sql_sqlstmnt(cfrds_buffer *buffer);

/**
 * @brief Parses query column metadata from the RDS server response.
 * 
 * Parses names, types, and Java types (jtype) of columns.
 * 
 * @param buffer Server response buffer.
 * @return Allocated `cfrds_sql_metadata` structure. Must be freed by the caller.
 *         Returns NULL on parse error.
 */
struct cfrds_sql_metadata *cfrds_buffer_to_sql_metadata(cfrds_buffer *buffer);

/**
 * @brief Parses supported SQL database commands list from the RDS server response.
 * 
 * Verifies that row count is 1, extracts all command names, and populates the list.
 * 
 * @param buffer Server response buffer.
 * @return Allocated `cfrds_sql_supportedcommands` structure. Must be freed by the caller.
 *         Returns NULL on parsing error.
 */
struct cfrds_sql_supportedcommands *cfrds_buffer_to_sql_supportedcommands(cfrds_buffer *buffer);

/**
 * @brief Parses database description from the RDS server response.
 * 
 * Verifies single-row response, extracts description string from comma-separated fields.
 * 
 * @param buffer Server response buffer.
 * @return Allocated string copy of description. Must be freed by the caller. Returns NULL on error.
 */
char *cfrds_buffer_to_sql_dbdescription(cfrds_buffer *buffer);

/**
 * @brief Parses debugger start response.
 * 
 * Expects exactly 2 rows. Extracts and returns the second row (usually containing some unique ID/XML status).
 * 
 * @param buffer Server response buffer.
 * @return Allocated string with response value. Must be freed by caller. Returns NULL on error.
 */
char *cfrds_buffer_to_debugger_start(cfrds_buffer *buffer);

/**
 * @brief Parses debugger stop response.
 * 
 * Expects exactly 1 row containing WDDX XML. Parses WDDX and verifies status equals "RDS_OK".
 * 
 * @param buffer Server response buffer.
 * @return true if successful and status is RDS_OK, false otherwise.
 */
bool cfrds_buffer_to_debugger_stop(cfrds_buffer *buffer);

/**
 * @brief Parses debugger info response to extract debug port.
 * 
 * Expects exactly 1 row containing WDDX XML. Verifies status is "RDS_OK" and returns 
 * the double value "DEBUG_SERVER_PORT" converted to int.
 * 
 * @param buffer Server response buffer.
 * @return The port number on success, -1 on parsing/status error.
 */
int cfrds_buffer_to_debugger_info(cfrds_buffer *buffer);

/**
 * @brief Parses debugger events.
 * 
 * Expects exactly 1 row containing WDDX XML. Parses WDDX XML and casts root node to event structure.
 * 
 * @param buffer Server response buffer.
 * @return A parsed `cfrds_debugger_event` pointer (which maps to a WDDX structure), or NULL on failure.
 */
cfrds_debugger_event *cfrds_buffer_to_debugger_event(cfrds_buffer *buffer);
