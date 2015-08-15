#include <Python.h>
#include <strings.h>

typedef struct PyLangFile {
	PyObject_HEAD
	HSTRINGS strings;
} PyLangFile;

staticforward PyTypeObject PyLangFile_Type;

#define PyLangFile_Check(v)  ((v)->ob_type == &PyLangFile_Type)
#define PyLangFile_Strings(v)  (((PyLangFile *)(v))->strings)

static PyObject* strings_open(PyObject* self, PyObject* args)
{
	HSTRINGS strings = 0;
	PyLangFile* pyObj = 0;
	const char* path = 0;

	self;

	if (!PyArg_ParseTuple(args, "s", &path))
		goto cleanup;

	strings = OpenStrings(path);
	if (!strings) {
		// exception?
		goto cleanup;
	}

	pyObj = PyObject_NEW(PyLangFile, &PyLangFile_Type);
	if (!pyObj)
		goto cleanup;
	pyObj->strings = strings;
	return (PyObject*)pyObj;

cleanup:
	if (strings)
		CloseStrings(strings);
	if (pyObj)
		Py_DECREF(pyObj);
	Py_RETURN_NONE;
}

static void PyLangFile_dealloc(PyObject *self)
{
	if (PyLangFile_Check(self)) {
		HSTRINGS strings = PyLangFile_Strings(self);
		if (strings)
			CloseStrings(strings);
	}

	PyMem_DEL(self);
}

static PyObject* PyLangFile_get(PyObject* self, PyObject* args)
{
	HSTRINGS strings = 0;
	const char* lookup = 0;
	int id = 0;
	int count = 0;

	if (!PyLangFile_Check(self))
		Py_RETURN_NONE;

	strings = PyLangFile_Strings(self);

	if (PyArg_ParseTuple(args, "ii", &count, &id)) {
		lookup = ReadStringPl(strings, count, id);
	} else if (PyArg_ParseTuple(args, "i", &id)) {
		lookup = ReadString(strings, id);
	}

	if (!lookup)
		Py_RETURN_NONE;

	return PyString_FromString(lookup);
}

statichere PyTypeObject PyLangFile_Type = {
	PyObject_HEAD_INIT(0)
	0,                    // ob_size
	"strings.langs_file", // tp_name
	sizeof(PyLangFile),   // tp_basicsize
	0,                    // tp_itemsize
	PyLangFile_dealloc    // tp_dealloc
};

static PyMethodDef StringsMethods[] = {
	{ "open",  strings_open, METH_VARARGS, "Opens translation string based on path given." },
	{ NULL, NULL, 0, NULL } /* Sentinel */
};

static PyMethodDef PyLangFileMethods[] = {
	{ "__call__",  PyLangFile_get, METH_VARARGS, "Looks up the translation. Either self(id) or self(count, id), for strings with plural." },
	{ "get",  PyLangFile_get, METH_VARARGS, "Looks up the translation. Either get(id) or get(count, id), for strings with plural." },
	{ NULL, NULL, 0, NULL } /* Sentinel */
};

PyMODINIT_FUNC initstrings(void)
{
	PyObject* module = Py_InitModule("strings", StringsMethods);;
	PyLangFile_Type.tp_doc = "Looks up translated strings in specified file.";
	PyLangFile_Type.tp_methods = PyLangFileMethods;

	if (PyType_Ready(&PyLangFile_Type) < 0)
		return;

	Py_INCREF(&PyLangFile_Type);
	PyModule_AddObject(module, "langs_file", (PyObject *)&PyLangFile_Type);
}
