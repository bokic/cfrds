#include <cfrds.h>

#include <Python.h>
#include <structmember.h>
#include <stddef.h>


typedef struct {
    PyObject_HEAD
    PyObject *host;
    int port;
    PyObject *username;
    PyObject *password;
} cfrds_server_Object;

static PyObject *
cfrds_server_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    cfrds_server_Object *self = NULL;
    
    self = (cfrds_server_Object *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->host = PyUnicode_FromString("localhost");
        if (self->host == NULL) {
            Py_DECREF(self);
            return NULL;
        }

        self->port = 8500;

        self->username = PyUnicode_FromString("");
        if (self->username == NULL) {
            Py_DECREF(self);
            return NULL;
        }

        self->password = PyUnicode_FromString("");
        if (self->password == NULL) {
            Py_DECREF(self);
            return NULL;
        }
    }

    return (PyObject *) self;
}

static PyObject *
cfrds_server_browse_dir(cfrds_server_Object *self, PyObject *args)
{
    cfrds_browse_dir_t *dir = NULL;
    cfrds_server *server = NULL;
    PyObject *ret = NULL;
    char *path = NULL;

    ret = PyList_New(0);

    if (!PyArg_ParseTuple(args, "s", &path))
    {
        PyErr_SetString(PyExc_RuntimeError, "path not set");
        goto exit;
    }

    if (!cfrds_server_init(&server, PyUnicode_AsUTF8(self->host), self->port, PyUnicode_AsUTF8(self->username), PyUnicode_AsUTF8(self->password)))
    {
        PyErr_SetString(PyExc_RuntimeError, "cfrds_server_init function failed!");
        goto exit;
    }

    enum cfrds_status res = cfrds_command_browse_dir(server, path, &dir);
    if (res != CFRDS_STATUS_OK)
    {
        switch(res)
        {
        case CFRDS_STATUS_PARAM_IS_NULL:
            PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_PARAM_IS_NULL");
            break;
        case CFRDS_STATUS_SERVER_IS_NULL:
            PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_SERVER_IS_NULL");
            break;
        case CFRDS_STATUS_COMMAND_FAILED:
            PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_COMMAND_FAILED");
            break;
        case CFRDS_STATUS_RESPONSE_ERROR:
            PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_RESPONSE_ERROR");
            break;
        case CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND:
            PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND");
            break;
        case CFRDS_STATUS_DIR_ALREADY_EXISTS:
            PyErr_SetString(PyExc_RuntimeError, "CFRDS_STATUS_DIR_ALREADY_EXISTS");
            break;
        default:
            PyErr_SetString(PyExc_RuntimeError, "Unknown CFRDS error.");
            break;
        }

        goto exit;
    }

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
    cfrds_server_free(server);

    return ret;
}

static PyObject *
cfrds_server_file_read(cfrds_server_Object *self, PyObject *args)
{
    PyObject *ret = NULL;
    char *path = NULL;

    if (!PyArg_ParseTuple(args, "s", &path))
    {
        PyErr_SetString(PyExc_RuntimeError, "path not set");
        goto exit;
    }

exit:
    return ret;
}

static PyMemberDef cfrds_server_members[] = {
    {"host",     T_OBJECT_EX, offsetof(cfrds_server_Object, host),     0, "hostname"},
    {"port",     T_INT,       offsetof(cfrds_server_Object, port),     0, "port"},
    {"username", T_OBJECT_EX, offsetof(cfrds_server_Object, username), 0, "username"},
    {"password", T_OBJECT_EX, offsetof(cfrds_server_Object, password), 0, "password"},
    {NULL}  /* Sentinel */
};

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
    //.tp_dealloc = (destructor) Custom_dealloc,
    .tp_members = cfrds_server_members,
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
