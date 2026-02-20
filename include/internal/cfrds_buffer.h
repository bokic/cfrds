#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <cfrds.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


typedef struct cfrds_buffer cfrds_buffer;

#ifdef _WIN32
typedef SOCKET cfrds_socket;
#define CFRDS_INVALID_SOCKET INVALID_SOCKET
#else
typedef int cfrds_socket;
#define CFRDS_INVALID_SOCKET 0
#endif

typedef struct {
    char *host;
    uint16_t port;
    char *username;
    char *orig_password;
    char *password;
    cfrds_socket socket;
    int _errno;
    int64_t error_code;
    char *error;
} cfrds_server_int;

typedef struct {
    char *data;
    int size;
    char *modified;
    char *permission;
} cfrds_file_content_int;

typedef struct {
    char kind;
    char *name;
    uint8_t permissions;
    size_t size;
    uint64_t modified;
} cfrds_browse_dir_item_int;

typedef struct {
    size_t cnt;
    cfrds_browse_dir_item_int items[];
} cfrds_browse_dir_int;

typedef struct {
    size_t cnt;
    char *names[];
} cfrds_sql_dsninfo_int;

typedef struct {
    char *unknown;
    char *schema;
    char *name;
    char *type;
} cfrds_sql_tableinfoitem_int;

typedef struct {
    size_t cnt;
    cfrds_sql_tableinfoitem_int items[];
} cfrds_sql_tableinfo_int;

typedef struct {
    char *schema;
    char *owner;
    char *table;
    char *name;
    int type;
    char *typeStr;
    int percision;
    int length;
    int scale;
    int radix;
    int nullable;
} cfrds_sql_columninfoitem_int;

typedef struct {
    size_t cnt;
    cfrds_sql_columninfoitem_int items[];
} cfrds_sql_columninfo_int;

typedef struct {
    char *tableCatalog;
    char *tableOwner;
    char *tableName;
    char *colName;
    int keySequence;
} cfrds_sql_primarykeysitem_int;

typedef struct {
    size_t cnt;
    cfrds_sql_primarykeysitem_int items[];
} cfrds_sql_primarykeys_int;

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
} cfrds_sql_foreignkeysitem_int;

typedef struct {
    size_t cnt;
    cfrds_sql_foreignkeysitem_int items[];
} cfrds_sql_foreignkeys_int;

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
} cfrds_sql_importedkeysitem_int;

typedef struct {
    size_t cnt;
    cfrds_sql_importedkeysitem_int items[];
} cfrds_sql_importedkeys_int;

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
} cfrds_sql_exportedkeysitem_int;

typedef struct {
    size_t cnt;
    cfrds_sql_exportedkeysitem_int items[];
} cfrds_sql_exportedkeys_int;

typedef struct {
    size_t columns;
    size_t rows;
    char *values[];
} cfrds_sql_resultset_int;

typedef struct {
    char *name;
    char *type;
    char *jtype;
} cfrds_sql_metadataitem_int;

typedef struct {
    size_t cnt;
    cfrds_sql_metadataitem_int items[];
} cfrds_sql_metadata_int;

typedef struct {
    size_t cnt;
    char *commands[];
} cfrds_sql_supportedcommands_int;

typedef struct {
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
} cfrds_security_analyzer_result_int;

#ifdef __cplusplus
extern "C"
{
#endif

bool cfrds_buffer_create(cfrds_buffer **buffer);

char *cfrds_buffer_data(cfrds_buffer *buffer);
size_t cfrds_buffer_data_size(cfrds_buffer *buffer);

bool cfrds_buffer_append(cfrds_buffer *buffer, const char *str);
bool cfrds_buffer_append_int(cfrds_buffer *buffer, int number);
bool cfrds_buffer_append_bytes(cfrds_buffer *buffer, const void *data, size_t length);
bool cfrds_buffer_append_buffer(cfrds_buffer *buffer, cfrds_buffer *new);
bool cfrds_buffer_append_char(cfrds_buffer *buffer, const char ch);

bool cfrds_buffer_append_rds_count(cfrds_buffer *buffer, size_t cnt);
bool cfrds_buffer_append_rds_string(cfrds_buffer *buffer, const char *str);
bool cfrds_buffer_append_rds_bytes(cfrds_buffer *buffer, const void *data, size_t length);

bool cfrds_buffer_reserve_above_size(cfrds_buffer *buffer, size_t size);
bool cfrds_buffer_expand(cfrds_buffer *buffer, size_t size);
void cfrds_buffer_free(cfrds_buffer *buffer);

bool cfrds_buffer_parse_number(const char **data, size_t *remaining, int64_t *out);
bool cfrds_buffer_parse_bytearray(const char **data, size_t *remaining, char **out, int *out_size);
bool cfrds_buffer_parse_string(const char **data, size_t *remaining, char **out);
bool cfrds_buffer_parse_string_list_item(const char **data, size_t *remaining, char **out);

cfrds_browse_dir_int *cfrds_buffer_to_browse_dir(cfrds_buffer *buffer);
cfrds_file_content_int *cfrds_buffer_to_file_content(cfrds_buffer *buffer);
cfrds_sql_dsninfo_int *cfrds_buffer_to_sql_dsninfo(cfrds_buffer *buffer);
cfrds_sql_tableinfo_int *cfrds_buffer_to_sql_tableinfo(cfrds_buffer *buffer);
cfrds_sql_columninfo_int *cfrds_buffer_to_sql_columninfo(cfrds_buffer *buffer);
cfrds_sql_primarykeys_int *cfrds_buffer_to_sql_primarykeys(cfrds_buffer *buffer);
cfrds_sql_foreignkeys_int *cfrds_buffer_to_sql_foreignkeys(cfrds_buffer *buffer);
cfrds_sql_importedkeys_int *cfrds_buffer_to_sql_importedkeys(cfrds_buffer *buffer);
cfrds_sql_exportedkeys_int *cfrds_buffer_to_sql_exportedkeys(cfrds_buffer *buffer);
cfrds_sql_resultset_int *cfrds_buffer_to_sql_sqlstmnt(cfrds_buffer *buffer);
cfrds_sql_metadata_int *cfrds_buffer_to_sql_metadata(cfrds_buffer *buffer);
cfrds_sql_supportedcommands_int *cfrds_buffer_to_sql_supportedcommands(cfrds_buffer *buffer);
char *cfrds_buffer_to_sql_dbdescription(cfrds_buffer *buffer);

char *cfrds_buffer_to_debugger_start(cfrds_buffer *buffer);
bool cfrds_buffer_to_debugger_stop(cfrds_buffer *buffer);
int cfrds_buffer_to_debugger_info(cfrds_buffer *buffer);
cfrds_debugger_event *cfrds_buffer_to_debugger_event(cfrds_buffer *buffer);

#ifdef __cplusplus
}
#endif
