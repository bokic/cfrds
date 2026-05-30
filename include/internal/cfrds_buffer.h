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
    int size;
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

struct cfrds_browse_dir *cfrds_buffer_to_browse_dir(cfrds_buffer *buffer);
struct cfrds_file_content *cfrds_buffer_to_file_content(cfrds_buffer *buffer);
struct cfrds_sql_dsninfo *cfrds_buffer_to_sql_dsninfo(cfrds_buffer *buffer);
struct cfrds_sql_tableinfo *cfrds_buffer_to_sql_tableinfo(cfrds_buffer *buffer);
struct cfrds_sql_columninfo *cfrds_buffer_to_sql_columninfo(cfrds_buffer *buffer);
struct cfrds_sql_primarykeys *cfrds_buffer_to_sql_primarykeys(cfrds_buffer *buffer);
struct cfrds_sql_foreignkeys *cfrds_buffer_to_sql_foreignkeys(cfrds_buffer *buffer);
struct cfrds_sql_importedkeys *cfrds_buffer_to_sql_importedkeys(cfrds_buffer *buffer);
struct cfrds_sql_exportedkeys *cfrds_buffer_to_sql_exportedkeys(cfrds_buffer *buffer);
struct cfrds_sql_resultset *cfrds_buffer_to_sql_sqlstmnt(cfrds_buffer *buffer);
struct cfrds_sql_metadata *cfrds_buffer_to_sql_metadata(cfrds_buffer *buffer);
struct cfrds_sql_supportedcommands *cfrds_buffer_to_sql_supportedcommands(cfrds_buffer *buffer);
char *cfrds_buffer_to_sql_dbdescription(cfrds_buffer *buffer);

char *cfrds_buffer_to_debugger_start(cfrds_buffer *buffer);
bool cfrds_buffer_to_debugger_stop(cfrds_buffer *buffer);
int cfrds_buffer_to_debugger_info(cfrds_buffer *buffer);
cfrds_debugger_event *cfrds_buffer_to_debugger_event(cfrds_buffer *buffer);

#ifdef __cplusplus
}
#endif
