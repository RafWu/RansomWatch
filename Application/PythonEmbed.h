#pragma once

// this mess is just to avoid defining of the debug flag in Python.h
#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif

#include <cstdarg>          // allow unbounded number of arguments to build values
#include <string>
#include <unordered_map>    // store modules
#include <vector>           // return value of callFunction is vectorized if number of values > 1
#include <any>              // allow variant return type
#include <initializer_list> // allow unbounded number of arguments


// TODO? replace THROW macro with a formater for exceptions
#define THROW(__exp, __fmt, ...) do { \
                                char __buff[256]; \
                                _snprintf_s(__buff, 256, __fmt, __VA_ARGS__); \
                                throw __exp(__buff); \
                                } while(0)

// TODO: make the class thread-safe, since now multiple instances
//       of the class can mess things up, because Py_Initialize
//       and Py_FinalizeEx should be called only once per thread
//       (I mean... TODO only if this is actually needed)

// TODO: C++ wrapper to PyObject that allows auto conversion to C++ objects
//       and manages memory

class CPPython {
private:
	static std::size_t refcount;

	std::unordered_map<std::wstring, PyObject*> pModules;

	CPPython(CPPython&&) = delete;
	CPPython(const CPPython&) = delete;
	CPPython& operator=(const CPPython&) = delete;

	using PyType = enum pytype_t {
		// Primitves:
		Long,
		Float,
		Boolean,
		String,
		None,
		// Non-Primitive:
		Tuple,
		List
		// that's it for now...
	};

	// Returns the type of a PyObject
	static PyType getPyType(PyObject* object) {
		static const char rname[] = "CPPython::getPyType";

		if (!object) {
			THROW(std::runtime_error, "%s: object is NULL", rname);
		}

		if (PyLong_Check(object)) {
			return Long;
		}

		if (PyFloat_Check(object)) {
			return Float;
		}

		if (PyBool_Check(object)) {
			return Boolean;
		}

		if (PyUnicode_Check(object)) {
			return String;
		}

		if (object == Py_None) {
			return None;
		}

		if (PyTuple_Check(object)) {
			return Tuple;
		}

		if (PyList_Check(object)) {
			return List;
		}

		THROW(std::runtime_error, "%s: object is of unsupported type.", rname);
	}

	// Returns the C++ equivalent of a primitive PyObject
	static std::any getPrimitive(PyObject* object) {
		static const char rname[] = "CPPython::getPrimitive";

		PyType type = getPyType(object);
		switch (type) {
		case Long:
			return PyLong_AsLong(object);
		case Float:
			return PyFloat_AsDouble(object);
		case Boolean:
			return PyObject_IsTrue(object) ? true : false;
		case String:
		{
			wchar_t* ws = PyUnicode_AsWideCharString(object, 0);
			if (!ws) {
				THROW(std::runtime_error, "%s: Could not allocate wchat_t*.", rname);
			}
			std::wstring retval(ws);
			PyMem_Free(ws);
			return retval;
		}
		case None:
			// weird, but the only reasonable thing to do.
			return std::any();
		default:
			THROW(std::runtime_error, "%s: object is not primitive type.", rname);
		}
	}

	// Returns true if a PyObject is primitive, false otherwise
	static bool isPrimitiveType(PyObject* object) {
		PyType type = getPyType(object);
		return type == Long || type == Float || type == Boolean || type == String || type == None;
	}

	// Courtesy of Python source code.
	// Returns the number of argument to expect in a va_list
	// (or -1 in failure).
	static Py_ssize_t countformat(const char* format, char endchar) {
		Py_ssize_t count = 0;
		int level = 0;
		while (level > 0 || *format != endchar) {
			switch (*format) {
			case '\0':
				// Premature end
				PyErr_SetString(PyExc_SystemError,
					"unmatched paren in format");
				return -1;
			case '(':
			case '[':
			case '{':
				if (level == 0) {
					++count;
				}
				++level;
				break;
			case ')':
			case ']':
			case '}':
				--level;
				break;
			case '#':
			case '&':
			case ',':
			case ':':
			case ' ':
			case '\t':
				break;
			default:
				if (level == 0) {
					++count;
				}
			}
			++format;
		}
		return count;
	}

	// Returns the C++ equivalent of a PyObject (primitive or non-primitive)
	// borrows PyObject
	static std::any parsePyObject(PyObject* object) {
		static const char rname[] = "CPPython::parsePyObject";

		if (!object) {
			THROW(std::runtime_error, "%s: pArgs is NULL.", rname);
		}

		if (isPrimitiveType(object)) {
			return getPrimitive(object);
		}

		PyType type = getPyType(object);
		if (type != Tuple && type != List) {
			THROW(std::runtime_error, "%s: Unsupported type.", rname);
		}

		Py_ssize_t size = (type == Tuple) ? PyTuple_Size(object) : PyList_Size(object);
		std::vector<std::any> retval;
		retval.reserve(size);
		for (Py_ssize_t i = 0; i < size; ++i) {
			PyObject* pValue = (type == Tuple) ? PyTuple_GetItem(object, i) : PyList_GetItem(object, i);
			// pValue is borrowed, so no need to handle reference count
			if (!pValue) {
				THROW(std::runtime_error, "%s: object is NULL in index (%zd)", rname, i);
			}

			if (isPrimitiveType(pValue)) {
				retval.push_back(getPrimitive(pValue));
			}
			else {
				// parse recursively
				retval.push_back(parsePyObject(pValue));
			}
		}

		return retval;
	}

public:

	using any = std::any;

	CPPython() {
		if (!(refcount++)) {
			Py_SetPythonHome(L"Python37");
			Py_Initialize();
		}
	}

	// Arguments:
	//  - pNames: module names to load.
	//  - paths: absolute paths in which modules resides.
	explicit CPPython(std::initializer_list<std::wstring> pNames, std::initializer_list<std::wstring> paths = {}) {
		if (!(refcount++)) {
			Py_SetPythonHome(L"Python37");
			Py_Initialize();
		}

		try {
			addModulePath(paths);
			loadModule(pNames);
		}
		catch (std::exception & e) {
			if (!(--refcount)) {
				Py_FinalizeEx();
				// Py_FinalizeEx can fail.
				// In that case it returns int < 0.
				// Not much can be done.
				// For other caveats info:
				// https://docs.python.org/3/c-api/init.html#c.Py_FinalizeEx
			}

			throw e;
		}
	}

	~CPPython() {
		for (auto& pair : pModules) {
			Py_DECREF(pair.second);
		}

		if (!(--refcount)) {
			Py_FinalizeEx();
		}
		// TODO: check for errors and log it
	}

	// Loads modules. Returns the object which loads the modules,
	// in order to allow chaining operations.
	//
	// For example:
	//  CPPython o;
	//  o.loadModule({ L"a" }).callFunction(L"a", L"funcA", "i", 3);
	CPPython& loadModule(std::initializer_list<std::wstring> pNames) {
		static const char rname[] = "CPPython::loadModule";
		if (!Py_IsInitialized()) {
			THROW(std::runtime_error, "%s: Python interpeter not initialized.", rname);
		}

		for (auto& _pName : pNames) {
			PyObject* pName = PyUnicode_FromWideChar(_pName.c_str(), _pName.size());
			if (!pName) {
				THROW(std::runtime_error, "%s: Python interpeter not initialized.", rname);
			}

			PyObject* pModule = PyImport_Import(pName);
			Py_DECREF(pName);
			if (!pModule) {
				// TODO: log module not found.
				//       don't throw, so other modules
				//       in pNames will have change to load.
			}

			// pModules owns the reference
			pModules.emplace(_pName, pModule);  // may throw
		}

		return *this;
	}

	// Adds absoulte paths to where modules resides. Returns the object
	// to allow chaining, same as in loadModule.
	CPPython& addModulePath(std::initializer_list<std::wstring> absPaths) {
		static const char rname[] = "CPPython::addModulePath";

		if (!Py_IsInitialized()) {
			THROW(std::runtime_error, "%s: Python interpeter not initialized.", rname);
		}

		for (auto& absPath : absPaths) {
			PyObject* sysPath = PySys_GetObject("path");
			if (!sysPath) {
				THROW(std::runtime_error, "%s: Could not get system path.", rname);
			}

			PyObject* pPath = PyUnicode_FromWideChar(absPath.c_str(), absPath.size());
			if (!pPath) {
				THROW(std::runtime_error, "%s: Could not convert path string to PyUnicode.", rname);
			}

			PyList_Append(sysPath, pPath);
			Py_DECREF(pPath);
		}

		return *this;
	}

	// Calls a function from a Python module.
	// Returns the C++ equivalent of the Python return value.
	//
	// Arguments:
	//  - _modName: Module identifier in which _funcName resides
	//  - _funcName: Function identifier to call
	//  - fmt, ...: C Arguments for _funcName. The C arguments are described using
	//              a Py_BuildValue() style format string:
	//              http://dbpubs.stanford.edu:8091/~testbed/python/manual.1.4/ext/node11.html
	any callFunction(const std::wstring& _modName, const std::wstring& _funcName, const char* fmt = "", ...) {
		static const char rname[] = "CPPython::callFunction";

		if (pModules.find(_modName) == pModules.cend()) {
			THROW(std::out_of_range, "%s: module name not found.", rname);
		}

		// get the function from module
		PyObject* pFuncName = PyUnicode_FromWideChar(_funcName.c_str(), _funcName.size());
		if (!pFuncName) {
			THROW(std::runtime_error, "%s: Could not build function name.", rname);
		}

		PyObject* pDict = PyModule_GetDict(pModules.at(_modName));
		if (!pDict) {
			THROW(std::runtime_error, "%s: Could not get module dictionary.", rname);
		}

		PyObject* pFunc = PyDict_GetItem(pDict, pFuncName);
		if (!pFunc) {
			THROW(std::runtime_error, "%s: function name is not present in the module dictionary.", rname);
		}

		Py_DECREF(pFuncName);
		// pFunc and pDict are borrowed

		if (pFunc && PyCallable_Check(pFunc)) {
			// build the PyObject arguments to the function
			PyObject* pArgs;
			va_list va;
			Py_ssize_t n = countformat(fmt, '\0');
			if (n < 0) {
				THROW(std::invalid_argument, "%s: Wrong argument format.", rname);
			}

			if (n == 0) {
				pArgs = PyTuple_New(0);
			}
			else if (n == 1) {
				pArgs = PyTuple_New(1);
				va_start(va, fmt);
				PyObject* pValue = Py_VaBuildValue(fmt, va);
				if (!pValue) {
					Py_DECREF(pArgs);
					THROW(std::runtime_error, "%s: Could not build function arguments.", rname);
				}
				if (PyTuple_SetItem(pArgs, 0, pValue)) {
					Py_DECREF(pArgs);
					THROW(std::runtime_error, "%s: Could not set pValues.", rname);
				}
				va_end(va);
			}
			else {
				va_start(va, fmt);
				pArgs = Py_VaBuildValue(fmt, va);
				va_end(va);
			}

			if (!pArgs) {
				THROW(std::runtime_error, "%s: Could not allocate pArgs.", rname);  // TODO? should throw std::bad_alloc instead
			}

			// call pFunc with pArgs
			PyObject* pValue = PyObject_CallObject(pFunc, pArgs);
			Py_DECREF(pArgs);
			if (!pValue) {
				THROW(std::runtime_error, "%s: pValue is NULL. PyObject_CallObject failed.", rname);
			}

			// function call returned successfully and has a return value
			any retval;
			try {
				retval = parsePyObject(pValue);
			}
			catch (std::exception& e) {
				Py_DECREF(pValue);
				throw e;
			}

			Py_DECREF(pValue);
			return retval;
		}

		THROW(std::runtime_error, "%s: function does not exist or is not callable.", rname);
	}

	// casting a return type of callFunction to its actual C++ type
	template <class T>
	static T any_cast(const any& p) {
		return std::any_cast<T>(p);
	}
};
