#include <Python.h>

int main(int argc, char *argv[]) {
    Py_SetProgramName(argv[0]);
    Py_Initialize();
    PyRun_SimpleString("import sys; sys.path.append('.')");
    PyRun_SimpleString("import server; server.app.run(host='0.0.0.0', port=5000)");
    Py_Finalize();
    return 0;
}
