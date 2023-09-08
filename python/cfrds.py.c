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
cfrds_server_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    cfrds_server_Object *self = NULL;
    
    cfrds_server *server = NULL;
    const char *hostname = "127.0.0.1";
    int port = 8500;
    const char *username = "admin";
    const char *password = "";

    if (!PyArg_ParseTuple(args, "sHss", &hostname, &port, &username, &password))
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

static PyMethodDef cfrds_server_methods[] = {
    {"browse_dir", (PyCFunction) cfrds_server_browse_dir, METH_VARARGS, "List directory entries"},
    {"file_read",  (PyCFunction) cfrds_server_file_read,  METH_VARARGS, "Read file"},
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
