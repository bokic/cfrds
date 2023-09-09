#include <cfrds.h>

#include <Python.h>
#include <structmember.h>
#include <stddef.h>

#define CHECK_FOR_ERORRS(function_call)                                                       \
    {                                                                                         \
    enum cfrds_status res = function_call;                                                    \
        if (res != CFRDS_STATUS_OK)                                                           \
        {                                                                                     \
            switch(res)                                                                       \
            {                                                                                 \
            case CFRDS_STATUS_PARAM_IS_NULL:                                                  \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_PARAM_IS_NULL");            \
                break;                                                                        \
            case CFRDS_STATUS_SERVER_IS_NULL:                                                 \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_SERVER_IS_NULL");           \
                break;                                                                        \
            case CFRDS_STATUS_COMMAND_FAILED:                                                 \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_COMMAND_FAILED");           \
                break;                                                                        \
            case CFRDS_STATUS_RESPONSE_ERROR:                                                 \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_RESPONSE_ERROR");           \
                break;                                                                        \
            case CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND:                                        \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND");  \
                break;                                                                        \
            case CFRDS_STATUS_DIR_ALREADY_EXISTS:                                             \
                PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_DIR_ALREADY_EXISTS");       \
                break;                                                                        \
            default:                                                                          \
                PyErr_SetString(PyExc_RuntimeError, "Unknown CFRDS error.");                  \
                break;                                                                        \
            }                                                                                 \
                                                                                              \
            goto exit;                                                                        \
        }                                                                                     \
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
    int port = 8500;
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
    
    self = (cfrds_server_Object *) type->tp_alloc(type, 0);
    if (!server)
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
    cfrds_browse_dir_t *dir = NULL;
    PyObject *ret = NULL;
    char *path = NULL;

    ret = PyList_New(0);

    if (!PyArg_ParseTuple(args, "s", &path))
    {
        PyErr_SetString(PyExc_RuntimeError, "path not set");
        goto exit;
    }

    CHECK_FOR_ERORRS(cfrds_command_browse_dir(self->server, path, &dir));

    if (dir)
    {
        for(size_t c = 0; c < dir->cnt; c++)
        {
            PyObject *item = PyDict_New();

            PyDict_SetItemString(item, "kind", PyUnicode_FromFormat("%c", dir->items[c].kind));
            PyDict_SetItemString(item, "name", PyUnicode_FromString(dir->items[c].name));
            PyDict_SetItemString(item, "permissions", PyLong_FromUnsignedLong(dir->items[c].permissions));
            PyDict_SetItemString(item, "size", PyLong_FromSize_t(dir->items[c].size));
            PyDict_SetItemString(item, "modified", PyLong_FromUnsignedLongLong(dir->items[c].modified));

            PyList_Append(ret, item);
        }
    }

exit:
    cfrds_buffer_browse_dir_free(dir);

    return ret;
}

static PyObject *
cfrds_server_file_read(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    char *filepath = NULL;
    cfrds_file_content_t *file_content = NULL;

    if (!PyArg_ParseTuple(args, "s", &filepath))
    {
        PyErr_SetString(PyExc_RuntimeError, "filepath not set");
        goto exit;
    }

    CHECK_FOR_ERORRS(cfrds_command_file_read(self->server, filepath, &file_content));

    ret = PyByteArray_FromStringAndSize(file_content->data, file_content->size);

exit:
    cfrds_buffer_file_content_free(file_content);

    return ret;
}

static PyObject *
cfrds_server_file_write(cfrds_server_Object *self, PyObject *args)
{
    char *filepath = NULL;
    PyObject *file_content = NULL;

    if (!PyArg_ParseTuple(args, "sY", &filepath, &file_content))
    {
        PyErr_SetString(PyExc_RuntimeError, "filepath or content parameter not set!");
        goto exit;
    }

    CHECK_FOR_ERORRS(cfrds_command_file_write(self->server, filepath, PyByteArray_AsString(file_content),PyByteArray_Size(file_content)));

exit:
    return Py_None;
}

static PyObject *
cfrds_server_file_rename(cfrds_server_Object *self, PyObject *args)
{
    char *filepath_from = NULL;
    char *filepath_to = NULL;

    if (!PyArg_ParseTuple(args, "ss", &filepath_from, &filepath_to))
    {
        PyErr_SetString(PyExc_RuntimeError, "filename from/to parameter(s) not set!");
        goto exit;
    }

    CHECK_FOR_ERORRS(cfrds_command_file_rename(self->server, filepath_from, filepath_to));

exit:
    return Py_None;
}

static PyObject *
cfrds_server_file_remove(cfrds_server_Object *self, PyObject *args)
{
    char *filepath = NULL;

    if (!PyArg_ParseTuple(args, "s", &filepath))
    {
        PyErr_SetString(PyExc_RuntimeError, "filename parameter not set!");
        goto exit;
    }

    CHECK_FOR_ERORRS(cfrds_command_file_remove_file(self->server, filepath));

exit:
    return Py_None;
}

static PyObject *
cfrds_server_dir_remove(cfrds_server_Object *self, PyObject *args)
{
    char *dirpath = NULL;

    if (!PyArg_ParseTuple(args, "s", &dirpath))
    {
        PyErr_SetString(PyExc_RuntimeError, "dirpath parameter not set!");
        goto exit;
    }

    CHECK_FOR_ERORRS(cfrds_command_file_remove_dir(self->server, dirpath));

exit:
    return Py_None;
}

static PyObject *
cfrds_server_file_exists(cfrds_server_Object *self, PyObject *args)
{
    char *pathname = NULL;
    bool exists = false;

    if (!PyArg_ParseTuple(args, "s", &pathname))
    {
        PyErr_SetString(PyExc_RuntimeError, "pathname parameter not set!");
        goto exit;
    }

    CHECK_FOR_ERORRS(cfrds_command_file_exists(self->server, pathname, &exists));

    return PyBool_FromLong(exists);

exit:
    return Py_None;
}

static PyObject *
cfrds_server_dir_create(cfrds_server_Object *self, PyObject *args)
{
    char *dirpath = NULL;

    if (!PyArg_ParseTuple(args, "s", &dirpath))
    {
        PyErr_SetString(PyExc_RuntimeError, "dirpath parameter not set!");
        goto exit;
    }

    CHECK_FOR_ERORRS(cfrds_command_file_create_dir(self->server, dirpath));

exit:
    return Py_None;
}

static PyObject *
cfrds_server_cf_root_dir(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    char *dirpath = NULL;

    CHECK_FOR_ERORRS(cfrds_command_file_get_root_dir(self->server, &dirpath));

    if (!dirpath)
        goto exit;

    ret = PyUnicode_DecodeUTF8(dirpath, strlen(dirpath), NULL);

    free(dirpath);

    return ret;

exit:
    return Py_None;
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
    //.tp_init = (initproc) Custom_init,
    .tp_dealloc = (destructor) cfrds_server_dealloc,
    //.tp_members = cfrds_server_members,
    .tp_methods = cfrds_server_methods,
};

// A struct contains the definition of a module
static PyModuleDef cfrds_module = {
    PyModuleDef_HEAD_INIT,
    "cfrds",
    "Module for accessing files via cfrds",
    -1,
};

// The module init function
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
