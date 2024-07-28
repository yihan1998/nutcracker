// main.c
#include <Python.h>

int main(int argc, char *argv[]) {
    Py_Initialize();
    PyRun_SimpleString("from server import entrypoint; entrypoint()");
    Py_Finalize();
    return 0;
}
