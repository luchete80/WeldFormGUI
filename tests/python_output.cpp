#include <Python.h>
#include <iostream>
#include <cstring> // For strdup and strcpy

int main() {
    // Initialize the Python interpreter
    Py_Initialize();

    // Import the io module and get StringIO
    PyObject* ioModule = PyImport_ImportModule("io");
    if (!ioModule) {
        std::cerr << "Failed to import io module!" << std::endl;
        Py_Finalize();
        return 1;
    }
    PyObject* stringIOClass = PyObject_GetAttrString(ioModule, "StringIO");
    Py_DECREF(ioModule);
    if (!stringIOClass) {
        std::cerr << "Failed to get StringIO class!" << std::endl;
        Py_Finalize();
        return 1;
    }

    // Create a StringIO object
    PyObject* stringIO = PyObject_CallObject(stringIOClass, nullptr);
    Py_DECREF(stringIOClass);
    if (!stringIO) {
        std::cerr << "Failed to create StringIO object!" << std::endl;
        Py_Finalize();
        return 1;
    }

    // Redirect sys.stdout and sys.stderr to the StringIO object
    PyObject* sysModule = PyImport_ImportModule("sys");
    if (!sysModule) {
        std::cerr << "Failed to import sys module!" << std::endl;
        Py_DECREF(stringIO);
        Py_Finalize();
        return 1;
    }
    PyObject* originalStdout = PyObject_GetAttrString(sysModule, "stdout");
    PyObject* originalStderr = PyObject_GetAttrString(sysModule, "stderr");
    PyObject_SetAttrString(sysModule, "stdout", stringIO);
    PyObject_SetAttrString(sysModule, "stderr", stringIO);

    // Run Python code
    const char* pythonCode = R"(
print("Hello from Python!")
import sys
sys.stderr.write("This is an error message.\n")
)";
    PyRun_SimpleString(pythonCode);

    // Retrieve the contents of the StringIO buffer
    PyObject* getValueMethod = PyObject_GetAttrString(stringIO, "getvalue");
    PyObject* output = PyObject_CallObject(getValueMethod, nullptr);
    Py_DECREF(getValueMethod);

    char* outputBuffer = nullptr;
    if (output) {
        // Convert Python string to a C-style string
        const char* outputCStr = PyUnicode_AsUTF8(output);
        if (outputCStr) {
            // Allocate and copy the buffer
            outputBuffer = strdup(outputCStr); // Dynamically allocate memory for char*
        }
        Py_DECREF(output);
    } else {
        std::cerr << "Failed to retrieve output from StringIO!" << std::endl;
    }

    // Restore original sys.stdout and sys.stderr
    PyObject_SetAttrString(sysModule, "stdout", originalStdout);
    PyObject_SetAttrString(sysModule, "stderr", originalStderr);
    Py_DECREF(originalStdout);
    Py_DECREF(originalStderr);
    Py_DECREF(stringIO);
    Py_DECREF(sysModule);

    // Finalize the Python interpreter
    Py_Finalize();

    // Use the captured output
    if (outputBuffer) {
        std::cout << "Captured Output:\n" << outputBuffer << std::endl;
        free(outputBuffer); // Free the allocated memory
    } else {
        std::cerr << "No output captured!" << std::endl;
    }

    return 0;
}