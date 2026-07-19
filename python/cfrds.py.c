#include <cfrds.h>

#include <Python.h>
#include <structmember.h>
#include <stddef.h>

static inline void dict_set_string_steal(PyObject *dict, const char *key, const char *str) {
    if (str != NULL) {
        PyObject *val = PyUnicode_FromString(str);
        if (val) {
            PyDict_SetItemString(dict, key, val);
            Py_DECREF(val);
        }
    } else {
        PyDict_SetItemString(dict, key, Py_None);
    }
}

static inline void dict_set_long_steal(PyObject *dict, const char *key, long v) {
    PyObject *val = PyLong_FromLong(v);
    if (val) {
        PyDict_SetItemString(dict, key, val);
        Py_DECREF(val);
    }
}

static inline void dict_set_size_t_steal(PyObject *dict, const char *key, size_t v) {
    PyObject *val = PyLong_FromSize_t(v);
    if (val) {
        PyDict_SetItemString(dict, key, val);
        Py_DECREF(val);
    }
}

static inline void dict_set_ulonglong_steal(PyObject *dict, const char *key, unsigned long long v) {
    PyObject *val = PyLong_FromUnsignedLongLong(v);
    if (val) {
        PyDict_SetItemString(dict, key, val);
        Py_DECREF(val);
    }
}

#define CHECK_FOR_ERRORS(function_call)                                                          \
    {                                                                                            \
        cfrds_status res = function_call;                                                       \
        if (res != CFRDS_STATUS_OK)                                                              \
        {                                                                                        \
            switch(res)                                                                          \
            {                                                                                    \
            case CFRDS_STATUS_PARAM_IS_NULL:                                                  \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_PARAM_IS_NULL");            \
                break;                                                                           \
            case CFRDS_STATUS_SERVER_IS_NULL:                                                 \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_SERVER_IS_NULL");           \
                break;                                                                           \
            case CFRDS_STATUS_INDEX_OUT_OF_BOUNDS:                                               \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_INDEX_OUT_OF_BOUNDS");         \
                break;                                                                           \
            case CFRDS_STATUS_COMMAND_FAILED:                                                    \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_COMMAND_FAILED");              \
                break;                                                                           \
            case CFRDS_STATUS_RESPONSE_ERROR:                                                    \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_RESPONSE_ERROR");              \
                break;                                                                           \
            case CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND:                                           \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND");     \
                break;                                                                           \
            case CFRDS_STATUS_DIR_ALREADY_EXISTS:                                                \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_DIR_ALREADY_EXISTS");          \
                break;                                                                           \
            case CFRDS_STATUS_SOCKET_CREATION_FAILED:                                            \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_SOCKET_CREATION_FAILED");      \
                break;                                                                           \
            case CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED:                                       \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED"); \
                break;                                                                           \
            case CFRDS_STATUS_WRITING_TO_SOCKET_FAILED:                                          \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_WRITING_TO_SOCKET_FAILED");    \
                break;                                                                           \
            case CFRDS_STATUS_PARTIALLY_WRITE_TO_SOCKET:                                         \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_PARTIALLY_WRITE_TO_SOCKET");   \
                break;                                                                           \
            case CFRDS_STATUS_READING_FROM_SOCKET_FAILED:                                        \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_READING_FROM_SOCKET_FAILED");  \
                break;                                                                           \
            default:                                                                             \
                PyErr_SetString(PyExc_RuntimeError, "Unknown CFRDS error.");                     \
                break;                                                                           \
            }                                                                                    \
                                                                                                 \
            goto exit;                                                                           \
        }                                                                                        \
    }

typedef struct {
    PyObject_HEAD
    cfrds_server *server;
} cfrds_server_Object;

static PyObject *
cfrds_server_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    cfrds_server_Object *self = NULL;

    static char *kwlist[] = {"hostname", "port", "username", "password", NULL};

    cfrds_server *server = NULL;
    const char *hostname = "127.0.0.1";
    uint16_t port = 8500;
    const char *username = "admin";
    const char *password = "";

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sHss", kwlist, &hostname, &port, &username, &password))
    {
        PyErr_SetString(PyExc_RuntimeError, "Invalid arguments!");
        goto error;
    }

    if (!cfrds_server_init(&server, hostname, port, username, password))
    {
        PyErr_SetString(PyExc_RuntimeError, "cfrds_server_init function failed!");
        goto error;
    }
    if (server == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "failed to create server struct!");
        goto error;
    }

    self = (cfrds_server_Object *) type->tp_alloc(type, 0);
    if (!self)
    {
        PyErr_SetString(PyExc_RuntimeError, "Can't create self!");
        goto error;
    }

    self->server = server;

    return (PyObject *) self;

error:
    if (server)
    {
        cfrds_server_free(server);
    }

    return NULL;
}

static int
cfrds_server_dealloc(cfrds_server_Object *self)
{
    if (self->server)
    {
        cfrds_server_free(self->server);
        self->server = NULL;
    }

    return 0;
}

static PyObject *
cfrds_server_browse_dir(cfrds_server_Object *self, PyObject *args)
{
    cfrds_browse_dir *dir = NULL;
    PyObject *ret = NULL;
    char *path = NULL;

    if (!PyArg_ParseTuple(args, "s", &path))
    {
        PyErr_SetString(PyExc_RuntimeError, "path not set");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_browse_dir(self->server, path, &dir));

    ret = PyList_New(0);
    if (!ret)
        goto exit;

    if (dir)
    {
        for(size_t c = 0; c < cfrds_browse_dir_count(dir); c++)
        {
            PyObject *item = PyDict_New();
            if (!item)
            {
                Py_DECREF(ret);
                ret = NULL;
                goto exit;
            }

            char kind = cfrds_browse_dir_item_get_kind(dir, c);
            char kind_str[2] = {kind, '\0'};
            dict_set_string_steal(item, "kind", kind_str);
            dict_set_string_steal(item, "name", cfrds_browse_dir_item_get_name(dir, c));

            uint8_t permissions = cfrds_browse_dir_item_get_permissions(dir, c);
            char permissions_str[] = "-----";
            if (kind == 'D') permissions_str[0] = 'D';
            if (permissions & 0x01) permissions_str[1] = 'R';
            if (permissions & 0x02) permissions_str[2] = 'H';
            if (permissions & 0x10) permissions_str[3] = 'A';
            if (permissions & 0x80) permissions_str[4] = 'N';
            dict_set_string_steal(item, "permissions", permissions_str);

            dict_set_size_t_steal(item, "size", cfrds_browse_dir_item_get_size(dir, c));
            dict_set_ulonglong_steal(item, "modified", cfrds_browse_dir_item_get_modified(dir, c));

            PyList_Append(ret, item);
            Py_DECREF(item);
        }
    }

    cfrds_browse_dir_free(dir);
    return ret;

exit:
    cfrds_browse_dir_free(dir);
    Py_XDECREF(ret);
    return NULL;
} 

static PyObject *
cfrds_server_file_read(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    char *filepath = NULL;
    cfrds_file_content *file_content = NULL;

    if (!PyArg_ParseTuple(args, "s", &filepath))
    {
        PyErr_SetString(PyExc_RuntimeError, "filepath not set");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_file_read(self->server, filepath, &file_content));

    ret = PyByteArray_FromStringAndSize(cfrds_file_content_get_data(file_content), (Py_ssize_t)cfrds_file_content_get_size(file_content));

    cfrds_file_content_free(file_content);
    return ret;

exit:
    cfrds_file_content_free(file_content);
    return NULL;
}

static PyObject *
cfrds_server_file_write(cfrds_server_Object *self, PyObject *args)
{
    char *filepath = NULL;
    PyObject *file_content = NULL;

    if (!PyArg_ParseTuple(args, "sY", &filepath, &file_content))
    {
        PyErr_SetString(PyExc_RuntimeError, "filepath or content parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_file_write(self->server, filepath, PyByteArray_AsString(file_content), PyByteArray_Size(file_content)));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyObject *
cfrds_server_file_rename(cfrds_server_Object *self, PyObject *args)
{
    char *filepath_from = NULL;
    char *filepath_to = NULL;

    if (!PyArg_ParseTuple(args, "ss", &filepath_from, &filepath_to))
    {
        PyErr_SetString(PyExc_RuntimeError, "filename from/to parameter(s) not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_file_rename(self->server, filepath_from, filepath_to));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyObject *
cfrds_server_file_remove(cfrds_server_Object *self, PyObject *args)
{
    char *filepath = NULL;

    if (!PyArg_ParseTuple(args, "s", &filepath))
    {
        PyErr_SetString(PyExc_RuntimeError, "filename parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_file_remove_file(self->server, filepath));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyObject *
cfrds_server_dir_remove(cfrds_server_Object *self, PyObject *args)
{
    char *dirpath = NULL;

    if (!PyArg_ParseTuple(args, "s", &dirpath))
    {
        PyErr_SetString(PyExc_RuntimeError, "dirpath parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_file_remove_dir(self->server, dirpath));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyObject *
cfrds_server_file_exists(cfrds_server_Object *self, PyObject *args)
{
    char *pathname = NULL;
    bool exists = false;

    if (!PyArg_ParseTuple(args, "s", &pathname))
    {
        PyErr_SetString(PyExc_RuntimeError, "pathname parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_file_exists(self->server, pathname, &exists));

    return PyBool_FromLong(exists);

exit:
    return NULL;
}

static PyObject *
cfrds_server_dir_create(cfrds_server_Object *self, PyObject *args)
{
    char *dirpath = NULL;

    if (!PyArg_ParseTuple(args, "s", &dirpath))
    {
        PyErr_SetString(PyExc_RuntimeError, "dirpath parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_file_create_dir(self->server, dirpath));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyObject *
cfrds_server_cf_root_dir(cfrds_server_Object *self)
{
    PyObject *ret = NULL;
    char *dirpath = NULL;

    CHECK_FOR_ERRORS(cfrds_command_file_get_root_dir(self->server, &dirpath));

    if (!dirpath)
        return NULL;

    ret = PyUnicode_DecodeUTF8(dirpath, strlen(dirpath), NULL);
    free(dirpath);
    return ret;

exit:
    return NULL;
}

static PyObject *
cfrds_server_sql_dsninfo(cfrds_server_Object *self)
{
    PyObject *ret = NULL;
    cfrds_sql_dsninfo *dsninfo = NULL;
    size_t cnt = 0;

    CHECK_FOR_ERRORS(cfrds_command_sql_dsninfo(self->server, &dsninfo));

    if (!dsninfo)
        return NULL;

    cnt = cfrds_sql_dsninfo_count(dsninfo);
    ret = PyList_New(cnt);
    if (!ret)
    {
        cfrds_sql_dsninfo_free(dsninfo);
        return NULL;
    }

    for(size_t c = 0; c < cnt; c++)
    {
        PyObject *val = PyUnicode_FromString(cfrds_sql_dsninfo_item_get_name(dsninfo, c));
        PyList_SetItem(ret, c, val);
    }

    cfrds_sql_dsninfo_free(dsninfo);
    return ret;

exit:
    return NULL;
}

static PyObject *
cfrds_server_sql_tableinfo(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    cfrds_sql_tableinfo *tableinfo = NULL;
    size_t cnt = 0;
    char *tablename = NULL;

    if (!PyArg_ParseTuple(args, "s", &tablename))
    {
        PyErr_SetString(PyExc_RuntimeError, "tablename parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_sql_tableinfo(self->server, tablename, &tableinfo));

    cnt = cfrds_sql_tableinfo_count(tableinfo);
    ret = PyList_New(cnt);
    if (!ret)
    {
        cfrds_sql_tableinfo_free(tableinfo);
        return NULL;
    }

    for(size_t c = 0; c < cnt; c++)
    {
        PyObject *item = PyDict_New();
        if (!item)
        {
            Py_DECREF(ret);
            cfrds_sql_tableinfo_free(tableinfo);
            return NULL;
        }

        dict_set_string_steal(item, "unknown", cfrds_sql_tableinfo_get_column_unknown(tableinfo, c));
        dict_set_string_steal(item, "schema", cfrds_sql_tableinfo_get_column_schema(tableinfo, c));
        dict_set_string_steal(item, "name", cfrds_sql_tableinfo_get_column_name(tableinfo, c));
        dict_set_string_steal(item, "type", cfrds_sql_tableinfo_get_column_type(tableinfo, c));

        PyList_SetItem(ret, c, item);
    }

    cfrds_sql_tableinfo_free(tableinfo);
    return ret;

exit:
    return NULL;
}

static PyObject *
cfrds_server_sql_columninfo(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    cfrds_sql_columninfo *columninfo = NULL;
    size_t cnt = 0;

    char *tablename = NULL;
    char *columnname = NULL;

    if (!PyArg_ParseTuple(args, "ss", &tablename, &columnname))
    {
        PyErr_SetString(PyExc_RuntimeError, "tablename and/or columnname parameter(s) not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_sql_columninfo(self->server, tablename, columnname, &columninfo));

    cnt = cfrds_sql_columninfo_count(columninfo);
    ret = PyList_New(cnt);
    if (!ret)
    {
        cfrds_sql_columninfo_free(columninfo);
        return NULL;
    }

    for(size_t c = 0; c < cnt; c++)
    {
        PyObject *item = PyDict_New();
        if (!item)
        {
            Py_DECREF(ret);
            cfrds_sql_columninfo_free(columninfo);
            return NULL;
        }

        dict_set_string_steal(item, "schema", cfrds_sql_columninfo_get_schema(columninfo, c));
        dict_set_string_steal(item, "owner", cfrds_sql_columninfo_get_owner(columninfo, c));
        dict_set_string_steal(item, "table", cfrds_sql_columninfo_get_table(columninfo, c));
        dict_set_string_steal(item, "name", cfrds_sql_columninfo_get_name(columninfo, c));
        dict_set_long_steal(item, "type", cfrds_sql_columninfo_get_type(columninfo, c));
        dict_set_string_steal(item, "typeStr", cfrds_sql_columninfo_get_typeStr(columninfo, c));
        dict_set_long_steal(item, "precision", cfrds_sql_columninfo_get_precision(columninfo, c));
        dict_set_long_steal(item, "length", cfrds_sql_columninfo_get_length(columninfo, c));
        dict_set_long_steal(item, "scale", cfrds_sql_columninfo_get_scale(columninfo, c));
        dict_set_long_steal(item, "radix", cfrds_sql_columninfo_get_radix(columninfo, c));
        dict_set_long_steal(item, "nullable", cfrds_sql_columninfo_get_nullable(columninfo, c));

        PyList_SetItem(ret, c, item);
    }

    cfrds_sql_columninfo_free(columninfo);
    return ret;

exit:
    return NULL;
}

static PyObject *
cfrds_server_sql_primarykeys(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    cfrds_sql_primarykeys *primarykeys = NULL;
    size_t cnt = 0;

    const char *tablename = NULL;
    const char *columnname = NULL;

    if (!PyArg_ParseTuple(args, "ss", &tablename, &columnname))
    {
        PyErr_SetString(PyExc_RuntimeError, "tablename and/or columnname parameter(s) not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_sql_primarykeys(self->server, tablename, columnname, &primarykeys));

    cnt = cfrds_sql_primarykeys_count(primarykeys);
    ret = PyList_New(cnt);
    if (ret == NULL)
    {
        cfrds_sql_primarykeys_free(primarykeys);
        return NULL;
    }

    for(size_t c = 0; c < cnt; c++)
    {
        PyObject *item = PyDict_New();
        if (!item)
        {
            Py_DECREF(ret);
            cfrds_sql_primarykeys_free(primarykeys);
            return NULL;
        }

        dict_set_string_steal(item, "catalog", cfrds_sql_primarykeys_get_catalog(primarykeys, c));
        dict_set_string_steal(item, "owner", cfrds_sql_primarykeys_get_owner(primarykeys, c));
        dict_set_string_steal(item, "table", cfrds_sql_primarykeys_get_table(primarykeys, c));
        dict_set_string_steal(item, "column", cfrds_sql_primarykeys_get_column(primarykeys, c));
        dict_set_long_steal(item, "key_sequence", cfrds_sql_primarykeys_get_key_sequence(primarykeys, c));

        PyList_SetItem(ret, c, item);
    }

    cfrds_sql_primarykeys_free(primarykeys);
    return ret;

exit:
    return NULL;
}

static PyObject *
cfrds_server_sql_foreignkeys(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    cfrds_sql_foreignkeys *foreignkeys = NULL;
    size_t cnt = 0;

    const char *tablename = NULL;
    const char *columnname = NULL;

    if (!PyArg_ParseTuple(args, "ss", &tablename, &columnname))
    {
        PyErr_SetString(PyExc_RuntimeError, "tablename and/or columnname parameter(s) not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_sql_foreignkeys(self->server, tablename, columnname, &foreignkeys));

    cnt = cfrds_sql_foreignkeys_count(foreignkeys);
    ret = PyList_New(cnt);
    if (ret == NULL)
    {
        cfrds_sql_foreignkeys_free(foreignkeys);
        return NULL;
    }

    for(size_t c = 0; c < cnt; c++)
    {
        PyObject *item = PyDict_New();
        if (!item)
        {
            Py_DECREF(ret);
            cfrds_sql_foreignkeys_free(foreignkeys);
            return NULL;
        }

        dict_set_string_steal(item, "pkcatalog", cfrds_sql_foreignkeys_get_pkcatalog(foreignkeys, c));
        dict_set_string_steal(item, "pkowner", cfrds_sql_foreignkeys_get_pkowner(foreignkeys, c));
        dict_set_string_steal(item, "pktable", cfrds_sql_foreignkeys_get_pktable(foreignkeys, c));
        dict_set_string_steal(item, "pkcolumn", cfrds_sql_foreignkeys_get_pkcolumn(foreignkeys, c));
        dict_set_string_steal(item, "fkcatalog", cfrds_sql_foreignkeys_get_fkcatalog(foreignkeys, c));
        dict_set_string_steal(item, "fkowner", cfrds_sql_foreignkeys_get_fkowner(foreignkeys, c));
        dict_set_string_steal(item, "fktable", cfrds_sql_foreignkeys_get_fktable(foreignkeys, c));
        dict_set_string_steal(item, "fkcolumn", cfrds_sql_foreignkeys_get_fkcolumn(foreignkeys, c));
        dict_set_long_steal(item, "key_sequence", cfrds_sql_foreignkeys_get_key_sequence(foreignkeys, c));
        dict_set_long_steal(item, "updaterule", cfrds_sql_foreignkeys_get_updaterule(foreignkeys, c));
        dict_set_long_steal(item, "deleterule", cfrds_sql_foreignkeys_get_deleterule(foreignkeys, c));

        PyList_SetItem(ret, c, item);
    }

    cfrds_sql_foreignkeys_free(foreignkeys);
    return ret;

exit:
    return NULL;
}

static PyObject *
cfrds_server_sql_importedkeys(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    cfrds_sql_importedkeys *importedkeys = NULL;
    size_t cnt = 0;

    const char *tablename = NULL;
    const char *columnname = NULL;

    if (!PyArg_ParseTuple(args, "ss", &tablename, &columnname))
    {
        PyErr_SetString(PyExc_RuntimeError, "tablename and/or columnname parameter(s) not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_sql_importedkeys(self->server, tablename, columnname, &importedkeys));

    cnt = cfrds_sql_importedkeys_count(importedkeys);
    ret = PyList_New(cnt);
    if (ret == NULL)
    {
        cfrds_sql_importedkeys_free(importedkeys);
        return NULL;
    }

    for(size_t c = 0; c < cnt; c++)
    {
        PyObject *item = PyDict_New();
        if (!item)
        {
            Py_DECREF(ret);
            cfrds_sql_importedkeys_free(importedkeys);
            return NULL;
        }

        dict_set_string_steal(item, "pkcatalog", cfrds_sql_importedkeys_get_pkcatalog(importedkeys, c));
        dict_set_string_steal(item, "pkowner", cfrds_sql_importedkeys_get_pkowner(importedkeys, c));
        dict_set_string_steal(item, "pktable", cfrds_sql_importedkeys_get_pktable(importedkeys, c));
        dict_set_string_steal(item, "pkcolumn", cfrds_sql_importedkeys_get_pkcolumn(importedkeys, c));
        dict_set_string_steal(item, "fkcatalog", cfrds_sql_importedkeys_get_fkcatalog(importedkeys, c));
        dict_set_string_steal(item, "fkowner", cfrds_sql_importedkeys_get_fkowner(importedkeys, c));
        dict_set_string_steal(item, "fktable", cfrds_sql_importedkeys_get_fktable(importedkeys, c));
        dict_set_string_steal(item, "fkcolumn", cfrds_sql_importedkeys_get_fkcolumn(importedkeys, c));
        dict_set_long_steal(item, "key_sequence", cfrds_sql_importedkeys_get_key_sequence(importedkeys, c));
        dict_set_long_steal(item, "updaterule", cfrds_sql_importedkeys_get_updaterule(importedkeys, c));
        dict_set_long_steal(item, "deleterule", cfrds_sql_importedkeys_get_deleterule(importedkeys, c));

        PyList_SetItem(ret, c, item);
    }

    cfrds_sql_importedkeys_free(importedkeys);
    return ret;

exit:
    return NULL;
}

static PyObject *
cfrds_server_sql_exportedkeys(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    cfrds_sql_exportedkeys *exportedkeys = NULL;
    size_t cnt = 0;

    const char *tablename = NULL;
    const char *columnname = NULL;

    if (!PyArg_ParseTuple(args, "ss", &tablename, &columnname))
    {
        PyErr_SetString(PyExc_RuntimeError, "tablename and/or columnname parameter(s) not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_sql_exportedkeys(self->server, tablename, columnname, &exportedkeys));

    cnt = cfrds_sql_exportedkeys_count(exportedkeys);
    ret = PyList_New(cnt);
    if (ret == NULL)
    {
        cfrds_sql_exportedkeys_free(exportedkeys);
        return NULL;
    }

    for(size_t c = 0; c < cnt; c++)
    {
        PyObject *item = PyDict_New();
        if (!item)
        {
            Py_DECREF(ret);
            cfrds_sql_exportedkeys_free(exportedkeys);
            return NULL;
        }

        dict_set_string_steal(item, "pkcatalog", cfrds_sql_exportedkeys_get_pkcatalog(exportedkeys, c));
        dict_set_string_steal(item, "pkowner", cfrds_sql_exportedkeys_get_pkowner(exportedkeys, c));
        dict_set_string_steal(item, "pktable", cfrds_sql_exportedkeys_get_pktable(exportedkeys, c));
        dict_set_string_steal(item, "pkcolumn", cfrds_sql_exportedkeys_get_pkcolumn(exportedkeys, c));
        dict_set_string_steal(item, "fkcatalog", cfrds_sql_exportedkeys_get_fkcatalog(exportedkeys, c));
        dict_set_string_steal(item, "fkowner", cfrds_sql_exportedkeys_get_fkowner(exportedkeys, c));
        dict_set_string_steal(item, "fktable", cfrds_sql_exportedkeys_get_fktable(exportedkeys, c));
        dict_set_string_steal(item, "fkcolumn", cfrds_sql_exportedkeys_get_fkcolumn(exportedkeys, c));
        dict_set_long_steal(item, "key_sequence", cfrds_sql_exportedkeys_get_key_sequence(exportedkeys, c));
        dict_set_long_steal(item, "updaterule", cfrds_sql_exportedkeys_get_updaterule(exportedkeys, c));
        dict_set_long_steal(item, "deleterule", cfrds_sql_exportedkeys_get_deleterule(exportedkeys, c));

        PyList_SetItem(ret, c, item);
    }

    cfrds_sql_exportedkeys_free(exportedkeys);
    return ret;

exit:
    return NULL;
}

static PyObject *
cfrds_server_sql_sqlstmnt(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    cfrds_sql_resultset *resultset = NULL;

    const char *tablename = NULL;
    const char *sql = NULL;

    if (!PyArg_ParseTuple(args, "ss", &tablename, &sql))
    {
        PyErr_SetString(PyExc_RuntimeError, "tablename and/or sql parameter(s) not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_sql_sqlstmnt(self->server, tablename, sql, &resultset));

    ret = PyDict_New();
    if (ret == NULL)
    {
        if (resultset) cfrds_sql_resultset_free(resultset);
        return NULL;
    }

    size_t cols = cfrds_sql_resultset_columns(resultset);
    size_t rows = cfrds_sql_resultset_rows(resultset);

    dict_set_size_t_steal(ret, "columns", cols);
    dict_set_size_t_steal(ret, "rows", rows);

    PyObject *names = PyList_New(cols);
    for(size_t c = 0; c < cols; c++)
    {
        PyObject *name = PyUnicode_FromString(cfrds_sql_resultset_column_name(resultset, c));
        PyList_SetItem(names, c, name ? name : Py_None);
    }
    PyDict_SetItemString(ret, "names", names);
    Py_DECREF(names);

    PyObject *data = PyList_New(rows);
    for(size_t r = 0; r < rows; r++)
    {
        PyObject *row = PyList_New(cols);
        for(size_t c = 0; c < cols; c++)
        {
            PyObject *value = PyUnicode_FromString(cfrds_sql_resultset_value(resultset, r, c));
            PyList_SetItem(row, c, value ? value : Py_None);
        }
        PyList_SetItem(data, r, row);
    }
    PyDict_SetItemString(ret, "data", data);
    Py_DECREF(data);

    if (resultset)
        cfrds_sql_resultset_free(resultset);

    return ret;

exit:
    if (resultset)
        cfrds_sql_resultset_free(resultset);

    return NULL;
}

static PyObject *
cfrds_server_sql_metadata(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    cfrds_sql_metadata *metadata = NULL;

    const char *tablename = NULL;
    const char *sql = NULL;
    size_t cols = 0;

    if (!PyArg_ParseTuple(args, "ss", &tablename, &sql))
    {
        PyErr_SetString(PyExc_RuntimeError, "tablename and/or sql parameter(s) not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_sql_sqlmetadata(self->server, tablename, sql, &metadata));

    cols = cfrds_sql_metadata_count(metadata);

    ret = PyList_New(cols);
    if (ret == NULL)
    {
        if (metadata) cfrds_sql_metadata_free(metadata);
        return NULL;
    }

    for(size_t c = 0; c < cols; c++)
    {
        PyObject *item = PyDict_New();
        if (!item)
        {
            Py_DECREF(ret);
            if (metadata) cfrds_sql_metadata_free(metadata);
            return NULL;
        }

        dict_set_string_steal(item, "name", cfrds_sql_metadata_get_name(metadata, c));
        dict_set_string_steal(item, "type", cfrds_sql_metadata_get_type(metadata, c));
        dict_set_string_steal(item, "jtype", cfrds_sql_metadata_get_jtype(metadata, c));

        PyList_SetItem(ret, c, item);
    }

    if (metadata)
        cfrds_sql_metadata_free(metadata);

    return ret;

exit:
    if (metadata)
        cfrds_sql_metadata_free(metadata);

    return NULL;
}

static PyObject *
cfrds_server_sql_getsupportedcommands(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    cfrds_sql_supportedcommands *supportedcommands = NULL;
    size_t cols = 0;

    CHECK_FOR_ERRORS(cfrds_command_sql_getsupportedcommands(self->server, &supportedcommands));

    cols = cfrds_sql_supportedcommands_count(supportedcommands);

    ret = PyList_New(cols);
    if (ret == NULL)
    {
        if (supportedcommands) cfrds_sql_supportedcommands_free(supportedcommands);
        return NULL;
    }

    for(size_t c = 0; c < cols; c++)
    {
        PyObject *value = PyUnicode_FromString(cfrds_sql_supportedcommands_get(supportedcommands, c));
        PyList_SetItem(ret, c, value ? value : Py_None);
    }

    if (supportedcommands)
        cfrds_sql_supportedcommands_free(supportedcommands);

    return ret;

exit:
    if (supportedcommands)
        cfrds_sql_supportedcommands_free(supportedcommands);

    return NULL;
}

static PyObject *
cfrds_server_sql_dbdescription(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    cfrds_str_defer(dbdescription);
    const char *tablename = NULL;

    if (!PyArg_ParseTuple(args, "s", &tablename))
    {
        PyErr_SetString(PyExc_RuntimeError, "tablename parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_sql_dbdescription(self->server, tablename, &dbdescription));

    ret = PyUnicode_FromString(dbdescription);
    return ret;

exit:
    return NULL;
}

static PyObject *
cfrds_server_debugger_start(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    cfrds_str_defer(session_name);

    CHECK_FOR_ERRORS(cfrds_command_debugger_start(self->server, &session_name));

    ret = PyUnicode_FromString(session_name);
    return ret;

exit:
    return NULL;
}

static PyObject *
cfrds_server_debugger_stop(cfrds_server_Object *self, PyObject *args)
{
    char *session_name = NULL;

    if (!PyArg_ParseTuple(args, "s", &session_name))
    {
        PyErr_SetString(PyExc_RuntimeError, "session_name parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_debugger_stop(self->server, session_name));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyObject *
cfrds_server_debugger_get_server_info(cfrds_server_Object *self, PyObject *args)
{
    cfrds_str_defer(session_name);
    cfrds_status res = CFRDS_STATUS_OK;
    uint16_t port = 0;

    CHECK_FOR_ERRORS(cfrds_command_debugger_start(self->server, &session_name));
    res = cfrds_command_debugger_get_server_info(self->server,  session_name, &port);
    CHECK_FOR_ERRORS(cfrds_command_debugger_stop(self->server, session_name));

    if (res == CFRDS_STATUS_OK)
        return PyLong_FromUnsignedLong(port);

exit:
    return NULL;
}

static PyObject *
cfrds_server_debugger_breakpoint_on_exception(cfrds_server_Object *self, PyObject *args)
{
    char *session_name = NULL;
    int enable;

    if (!PyArg_ParseTuple(args, "sp", &session_name, &enable))
    {
        PyErr_SetString(PyExc_RuntimeError, "session_name or enable parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_debugger_breakpoint_on_exception(self->server, session_name, enable));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyObject *
cfrds_server_debugger_breakpoint(cfrds_server_Object *self, PyObject *args)
{
    char *session_name = NULL;
    char *filepath = NULL;
    int line;
    int enable;

    if (!PyArg_ParseTuple(args, "ssip", &session_name, &filepath, &line, &enable))
    {
        PyErr_SetString(PyExc_RuntimeError, "session_name, filepath, line or enable parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_debugger_breakpoint(self->server, session_name, filepath, line, enable));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyObject *
cfrds_server_debugger_clear_all_breakpoints(cfrds_server_Object *self, PyObject *args)
{
    char *session_name = NULL;

    if (!PyArg_ParseTuple(args, "s", &session_name))
    {
        PyErr_SetString(PyExc_RuntimeError, "session_name parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_debugger_clear_all_breakpoints(self->server, session_name));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyObject *
parse_debug_events_response(const cfrds_debugger_event *event)
{
    cfrds_debugger_type type = cfrds_debugger_event_get_type(event);
    switch (type)
    {
        case CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT_SET:
        {
            PyObject *ret = PyDict_New();
            dict_set_string_steal(ret, "pathname", cfrds_debugger_event_breakpoint_set_get_pathname(event));
            dict_set_long_steal(ret, "req_line", cfrds_debugger_event_breakpoint_set_get_req_line(event));
            dict_set_long_steal(ret, "act_line", cfrds_debugger_event_breakpoint_set_get_act_line(event));
            return ret;
        }
        case CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT:
        case CFRDS_DEBUGGER_EVENT_TYPE_STEP:
        {
            PyObject *ret = PyDict_New();
            dict_set_string_steal(ret, "source", cfrds_debugger_event_breakpoint_get_source(event));
            dict_set_long_steal(ret, "line", cfrds_debugger_event_breakpoint_get_line(event));
            dict_set_string_steal(ret, "thread_name", cfrds_debugger_event_breakpoint_get_thread_name(event));
            return ret;
        }
        default:
            break;
    }

    PyErr_SetString(PyExc_RuntimeError, "Unknown debugger event type received!");
    return NULL;
}

static PyObject *
cfrds_server_debugger_get_debug_events(cfrds_server_Object *self, PyObject *args)
{
    cfrds_debugger_event_defer(event);
    char *session_name = NULL;

    if (!PyArg_ParseTuple(args, "s", &session_name))
    {
        PyErr_SetString(PyExc_RuntimeError, "session_name parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_debugger_get_debug_events(self->server, session_name, &event));

    return parse_debug_events_response(event);

exit:
    return NULL;
}

static PyObject *
cfrds_server_debugger_all_fetch_flags_enabled(cfrds_server_Object *self, PyObject *args)
{
    cfrds_debugger_event_defer(event);
    char *session_name = NULL;
    int threads;
    int watch;
    int scopes;
    int cf_trace;
    int java_trace;

    if (!PyArg_ParseTuple(args, "sppppp", &session_name, &threads, &watch, &scopes, &cf_trace, &java_trace))
    {
        PyErr_SetString(PyExc_RuntimeError, "session_name, threads, watch, scopes, cf_trace or java_trace parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_debugger_all_fetch_flags_enabled(self->server, session_name, threads, watch, scopes, cf_trace, java_trace, &event));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyObject *
cfrds_server_debugger_step_in(cfrds_server_Object *self, PyObject *args)
{
    const char *session_name = NULL;
    const char *thread_name = NULL;

    if (!PyArg_ParseTuple(args, "ss", &session_name, &thread_name))
    {
        PyErr_SetString(PyExc_RuntimeError, "session_name or thread_name parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_debugger_step_in(self->server, session_name, thread_name));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyObject *
cfrds_server_debugger_step_over(cfrds_server_Object *self, PyObject *args)
{
    const char *session_name = NULL;
    const char *thread_name = NULL;

    if (!PyArg_ParseTuple(args, "ss", &session_name, &thread_name))
    {
        PyErr_SetString(PyExc_RuntimeError, "session_name or thread_name parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_debugger_step_over(self->server, session_name, thread_name));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyObject *
cfrds_server_debugger_step_out(cfrds_server_Object *self, PyObject *args)
{
    const char *session_name = NULL;
    const char *thread_name = NULL;

    if (!PyArg_ParseTuple(args, "ss", &session_name, &thread_name))
    {
        PyErr_SetString(PyExc_RuntimeError, "session_name or thread_name parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_debugger_step_out(self->server, session_name, thread_name));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyObject *
cfrds_server_debugger_continue(cfrds_server_Object *self, PyObject *args)
{
    const char *session_name = NULL;
    const char *thread_name = NULL;

    if (!PyArg_ParseTuple(args, "ss", &session_name, &thread_name))
    {
        PyErr_SetString(PyExc_RuntimeError, "session_name or thread_name parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_debugger_continue(self->server, session_name, thread_name));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyObject *
cfrds_server_debugger_watch_expression(cfrds_server_Object *self, PyObject *args)
{
    const char *session_name = NULL;
    const char *thread_name = NULL;
    const char *expression = NULL;

    if (!PyArg_ParseTuple(args, "sss", &session_name, &thread_name, &expression))
    {
        PyErr_SetString(PyExc_RuntimeError, "session_name or thread_name parameter not set!");
        return NULL;
    }

    CHECK_FOR_ERRORS(cfrds_command_debugger_watch_expression(self->server, session_name, thread_name, expression));

    Py_RETURN_NONE;

exit:
    return NULL;
}

static PyMethodDef cfrds_server_methods[] = {
    {"browse_dir",  (PyCFunction) cfrds_server_browse_dir,  METH_VARARGS, "List directory entries"},
    {"file_read",   (PyCFunction) cfrds_server_file_read,   METH_VARARGS, "Read file"},
    {"file_write",  (PyCFunction) cfrds_server_file_write,  METH_VARARGS, "Write file"},
    {"file_rename", (PyCFunction) cfrds_server_file_rename, METH_VARARGS, "Rename file"},
    {"file_remove", (PyCFunction) cfrds_server_file_remove, METH_VARARGS, "Remove file"},
    {"dir_remove",  (PyCFunction) cfrds_server_dir_remove,  METH_VARARGS, "Remove dir"},
    {"file_exists", (PyCFunction) cfrds_server_file_exists, METH_VARARGS, "File exists"},
    {"dir_create",  (PyCFunction) cfrds_server_dir_create,  METH_VARARGS, "Create dir"},
    {"cf_root_dir", (PyCFunction) cfrds_server_cf_root_dir, METH_VARARGS, "Get ColdFusion root dir"},
    {"sql_dsninfo", (PyCFunction) cfrds_server_sql_dsninfo, METH_VARARGS, "Get ColdFusion datasources"},
    {"sql_tableinfo", (PyCFunction) cfrds_server_sql_tableinfo, METH_VARARGS, "Get ColdFusion datasource tables"},
    {"sql_columninfo", (PyCFunction) cfrds_server_sql_columninfo, METH_VARARGS, "Get ColdFusion datasource table columns"},
    {"sql_primarykeys", (PyCFunction) cfrds_server_sql_primarykeys, METH_VARARGS, "Get ColdFusion datasource table primary keys"},
    {"sql_foreignkeys", (PyCFunction) cfrds_server_sql_foreignkeys, METH_VARARGS, "Get ColdFusion datasource table foreign keys"},
    {"sql_importedkeys", (PyCFunction) cfrds_server_sql_importedkeys, METH_VARARGS, "Get ColdFusion datasource table imported keys"},
    {"sql_exportedkeys", (PyCFunction) cfrds_server_sql_exportedkeys, METH_VARARGS, "Get ColdFusion datasource table exported keys"},
    {"sql_sqlstmnt", (PyCFunction) cfrds_server_sql_sqlstmnt, METH_VARARGS, "Execute ColdFusion datasource SQL"},
    {"sql_metadata", (PyCFunction) cfrds_server_sql_metadata, METH_VARARGS, "Get ColdFusion datasource metadata"},
    {"sql_getsupportedcommands", (PyCFunction) cfrds_server_sql_getsupportedcommands, METH_VARARGS, "Get ColdFusion datasource supported commands"},
    {"sql_dbdescription", (PyCFunction) cfrds_server_sql_dbdescription, METH_VARARGS, "Get ColdFusion datasource database description"},
    {"debugger_start", (PyCFunction) cfrds_server_debugger_start, METH_VARARGS, "Start ColdFusion debugger session"},
    {"debugger_stop", (PyCFunction) cfrds_server_debugger_stop, METH_VARARGS, "Stop ColdFusion debugger session"},
    {"debugger_get_server_info", (PyCFunction) cfrds_server_debugger_get_server_info, METH_VARARGS, "Get ColdFusion debugger get server info"},
    {"debugger_breakpoint_on_exception", (PyCFunction) cfrds_server_debugger_breakpoint_on_exception, METH_VARARGS, "ColdFusion debugger breakpoint on exception(set/clear)"},
    {"debugger_breakpoint", (PyCFunction) cfrds_server_debugger_breakpoint, METH_VARARGS, "ColdFusion debugger breakpoint(set/clear)"},
    {"debugger_clear_all_breakpoints", (PyCFunction) cfrds_server_debugger_clear_all_breakpoints, METH_VARARGS, "ColdFusion debugger clear all breakpoints"},
    {"debugger_get_debug_events", (PyCFunction) cfrds_server_debugger_get_debug_events, METH_VARARGS, "ColdFusion debugger get debug events"},
    {"debugger_all_fetch_flags_enabled", (PyCFunction) cfrds_server_debugger_all_fetch_flags_enabled, METH_VARARGS, "ColdFusion debugger wait for flaged events"},
    {"debugger_step_in", (PyCFunction) cfrds_server_debugger_step_in, METH_VARARGS, "ColdFusion debugger step in"},
    {"debugger_step_over", (PyCFunction) cfrds_server_debugger_step_over, METH_VARARGS, "ColdFusion debugger step over"},
    {"debugger_step_out", (PyCFunction) cfrds_server_debugger_step_out, METH_VARARGS, "ColdFusion debugger step out"},
    {"debugger_continue", (PyCFunction) cfrds_server_debugger_continue, METH_VARARGS, "ColdFusion debugger continue"},
    {"debugger_watch_expression", (PyCFunction) cfrds_server_debugger_watch_expression, METH_VARARGS, "ColdFusion debugger watch expression"},
    {NULL}  /* Sentinel */
};

static PyTypeObject cfrds_server_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "cfrds.server",
    .tp_doc = PyDoc_STR("server objects"),
    .tp_basicsize = sizeof(cfrds_server_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = cfrds_server_new,
    .tp_dealloc = (destructor) cfrds_server_dealloc,
    .tp_methods = cfrds_server_methods,
};

static PyModuleDef cfrds_module = {
    PyModuleDef_HEAD_INIT,
    "cfrds",
    "Python module for cfrds library",
    -1,
};

PyMODINIT_FUNC PyInit_cfrds(void) {
    PyObject *m = NULL;

    if (PyType_Ready(&cfrds_server_Type) < 0)
        return NULL;

    m = PyModule_Create(&cfrds_module);
    if (m == NULL)
        return NULL;

    Py_INCREF(&cfrds_server_Type);
    if (PyModule_AddObject(m, "server", (PyObject *) &cfrds_server_Type) < 0) {
        Py_DECREF(&cfrds_server_Type);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
