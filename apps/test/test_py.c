#include <Python.h>
#include <stdio.h>

void call_python_function(const char *script_path, const char *module_name, const char *function_name) {
    // Initialize the Python interpreter
    Py_Initialize();

    // Set the Python path to include the directory of the script
    PyObject *sys_path = PySys_GetObject("path");
    PyObject *path = PyUnicode_DecodeFSDefault(script_path);
    if (sys_path && path) {
        PyList_Append(sys_path, path);
        Py_DECREF(path);
    } else {
        PyErr_Print();
        fprintf(stderr, "Failed to set Python path\n");
        Py_Finalize();
        return;
    }

    // Import the module
    PyObject *pName = PyUnicode_DecodeFSDefault(module_name);
    PyObject *pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        // Get the function from the module
        PyObject *pFunc = PyObject_GetAttrString(pModule, function_name);
        if (pFunc && PyCallable_Check(pFunc)) {
            // Call the function
            PyObject *pValue = PyObject_CallObject(pFunc, NULL);
            if (pValue != NULL) {
                printf("Result of call: %ld\n", PyLong_AsLong(pValue));
                Py_DECREF(pValue);
            } else {
                PyErr_Print();
                fprintf(stderr, "Call failed\n");
            }
            Py_XDECREF(pFunc);
        } else {
            if (PyErr_Occurred())
                PyErr_Print();
            fprintf(stderr, "Cannot find function \"%s\"\n", function_name);
        }
        Py_DECREF(pModule);
    } else {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", module_name);
    }

    // Finalize the Python interpreter
    Py_Finalize();
}

int main() {
    const char *script_path = "/home/ubuntu/Nutcracker/apps/recommendation";
    const char *module_name = "server"; // Module name without the .py extension
    const char *function_name = "entrypoint";

    call_python_function(script_path, module_name, function_name);

    return 0;
}
