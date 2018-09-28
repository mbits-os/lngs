#pragma once

#include <Python.h>
#if defined WIN32 || defined _WIN32
#include <windows.h>
#endif

#if PY_MAJOR_VERSION < 3
#define PY_TWO
#endif

#include <string>

template <typename Impl>
struct Type {
	struct _Impl {
		PyObject_HEAD;
		Impl payload;
	};

	static PyTypeObject*& pyType()
	{
		static PyTypeObject* ptr = nullptr;
		return ptr;
	}

	static bool check(PyObject* self)
	{
		return self->ob_type == pyType();
	}

	static Impl* extract(PyObject* self)
	{
		return std::addressof(((_Impl*)self)->payload);
	}

	template <typename... Args>
	static PyObject* create(Args&&... args)
	{
		auto pyObj = PyObject_New(_Impl, pyType());
		if (pyObj) {
			auto mem = std::addressof(pyObj->payload);
			new (mem) Impl { std::forward<Args>(args)... };
		}
		return (PyObject*)pyObj;
	}

	static void finalize(PyObject* self)
	{
		if (!check(self))
			return;

		extract(self)->~Impl();
	}

#ifdef PY_TWO
	static void dealloc(PyObject *self)
	{
		finalize(self);
		PyMem_DEL(self);
	}
#endif

	static PyObject* build(const std::string& module);
};

template <typename Impl>
struct Module {
	static PyObject*& pyModule()
	{
		static PyObject* ptr = nullptr;
		return ptr;
	}

	static bool create();

	template <typename Type>
	static bool add()
	{
		using PyType = typename Type::Type;
		auto type = PyType::build(Impl::py_name());
		if (!type)
			return false;
		Py_INCREF(type);
		PyModule_AddObject(pyModule(), Type::py_name(), type);
		return true;
	}

	static PyObject* init()
	{
		auto ret = Impl::__init__();
		if (!ret && pyModule())
			pyModule() = nullptr;
		return pyModule();
	}
};

template <typename Impl>
inline PyObject* Type<Impl>::build(const std::string& module)
{
	static auto klass = module + "." + Impl::py_name();
	static PyTypeObject TypeDef = {
		PyObject_HEAD_INIT(0)
#ifdef PY_TWO
		0,               // ob_size
#endif
		klass.c_str(), // tp_name
		sizeof(_Impl),   // tp_basicsize
	};

	TypeDef.tp_doc = Impl::py_doc();
	TypeDef.tp_methods = Impl::py_defs();
	TypeDef.tp_flags = Py_TPFLAGS_DEFAULT;
#ifdef PY_TWO
	TypeDef.tp_dealloc = dealloc;
#else
	TypeDef.tp_finalize = finalize;
#endif
	if (PyType_Ready(&TypeDef) < 0)
		return nullptr;

	pyType() = &TypeDef;

	return (PyObject*) pyType();
}

template <typename Impl>
inline bool Module<Impl>::create()
{
	PyObject* module = NULL;

#ifdef PY_TWO
	module = Py_InitModule(Impl::py_name(), Impl::py_defs());
#else
	static PyModuleDef ModDef = {
		PyModuleDef_HEAD_INIT,
		Impl::py_name(),
		NULL,
		-1,
		Impl::py_defs()
	};

	module = PyModule_Create(&ModDef);
#endif

	pyModule() = module;
	return !!module;
}

#define BEGIN_TYPE_MAP(name, doc_str) \
	static const char* py_name() { return name; } \
	static const char* py_doc() { return doc_str; } \
	static PyMethodDef* py_defs() \
	{ \
		static PyMethodDef methods[] = {

#define TYPE_DEF(name, call, doc_str) { name, call, METH_VARARGS, doc_str },
#define END_TYPE_MAP() \
			{ NULL, NULL, 0, NULL } \
		}; \
		return methods; \
	}

#define BEGIN_MOD_MAP(name) \
	static const char* py_name() { return #name; } \
	static PyMethodDef* py_defs() \
	{ \
		static PyMethodDef methods[] = {
#define MOD_DEF TYPE_DEF
#define END_MOD_MAP END_TYPE_MAP

#define DEF(method) static PyObject* method(PyObject* self, PyObject* args) \
	{\
		if (!Type::check(self)) \
			Py_RETURN_NONE; \
		return Type::extract(self)->packed_ ## method(args); \
	} \
	PyObject* packed_ ## method(PyObject* args)

#ifdef PY_TWO
#define PyMOD(type) \
	class type; \
	PyMODINIT_FUNC init##type(void) { Module<type>::init(); } \
    class type
#else
#define PyMOD(type) \
	class type; \
	PyMODINIT_FUNC PyInit_##type(void) { return Module<type>::init(); } \
	class type
#endif
