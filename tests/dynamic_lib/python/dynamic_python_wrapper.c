#include <Python.h>

// Initialize Python Interpreter
void init_py() {
    Py_Initialize();
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('.')");
}

// Finalize Python Interpreter
void finalize_py() {
    Py_Finalize();
}

// Function to multiply two numbers using the Python function
int callback_func() {
    PyObject *pName, *pModule, *pFunc;
    PyObject *pValue;
    int result = -1;

    init_py();

    pName = PyUnicode_DecodeFSDefault("pycode");
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, "callback_func");
        if (pFunc && PyCallable_Check(pFunc)) {
            pValue = PyObject_CallObject(pFunc, NULL);

            if (pValue != NULL) {
                result = (int) PyLong_AsLong(pValue);
                Py_DECREF(pValue);
            } else {
                PyErr_Print();
            }
        } else {
            if (PyErr_Occurred())
                PyErr_Print();
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    } else {
        PyErr_Print();
    }

    finalize_py();

    return result;
}

// C function to be exposed
int c_callback_func() {
    return callback_func();
}
