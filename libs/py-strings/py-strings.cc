#undef _DEBUG
#include "py_utils.hh"

#include <locale/strings.h>

#include <algorithm>

class PyLangFile {
	HSTRINGS strings;
public:
	using Type = ::Type<PyLangFile>;
	PyLangFile(HSTRINGS strings) : strings(strings)
	{
	}

	~PyLangFile()
	{
		if (strings)
			CloseStrings(strings);
	}

	BEGIN_TYPE_MAP("langs_file", "Looks up translated strings in specified file.")
		TYPE_DEF("__call__", get, "Looks up the translation. Either self(id) or self(count, id), for strings with plural.")
		TYPE_DEF("get",      get, "Looks up the translation. Either get(id) or get(count, id), for strings with plural.")
	END_TYPE_MAP()

	DEF(get)
	{
		const char* lookup = 0;
		int id = 0;
		int count = 0;

		if (PyArg_ParseTuple(args, "ii", &count, &id)) {
			lookup = ReadStringPl(strings, count, id);
		} else if (PyArg_ParseTuple(args, "i", &id)) {
			PyErr_Clear(); // ParseTuple(ii) might have left us with args exception, clear.
			lookup = ReadString(strings, id);
		}

		if (!lookup)
			Py_RETURN_NONE;

		return Py_BuildValue("s", lookup);
	}
};

PyMOD(strings) {
public:
	using Module = ::Module<strings>;

	static bool __init__()
	{
		return Module::create() &&
			Module::add<PyLangFile>();
	}

	static PyObject* open(PyObject* self, PyObject* args)
	{
		HSTRINGS strings = 0;
		const char* path = 0;

		(void)self;

		if (!PyArg_ParseTuple(args, "s", &path))
			Py_RETURN_NONE;

		strings = OpenStrings(path);
		if (!strings) {
			// exception?
			Py_RETURN_NONE;
		}

		auto pyObj = PyLangFile::Type::create(strings);
		if (pyObj)
			return pyObj;

		if (strings)
			CloseStrings(strings);
		Py_RETURN_NONE;
	}

	BEGIN_MOD_MAP(strings)
		MOD_DEF("open", open, "Opens translation string based on path given.")
	END_MOD_MAP()
};
