#pragma once

///////////////////////////////////////////////////
/// Define function return values start
#define ERROR_CODES \
X(RES_SUCCESS,          0, "Success") \
    X(RES_ERR_IPUT_PARM,    1, "Input parameter error") \
    X(RES_ERR_NOPATH,       2, "No storage path") \
    X(RES_ERR_OPENFILE,     3, "Fail to open file") \
    X(RES_ERR_EXIT,         4, "There is a common goal") \
    X(RES_ERR_STARTUP,      5, "Program startup failed") \
    X(RES_ERR_CREATE,       6, "Create failed") \
    X(RES_ERR_NO_FIND,      7, "No find") \
    X(RES_ERR_OPEN_IO,    100, "Failed to open IO") \
    X(RES_ERR_OPT_IO,     101, "Operation IO failed") \
    X(RES_ERR_WRITE_IO,   102, "Writing failed") \
    X(RES_ERR_READ_IO,    103, "Reading failed") \
    X(RES_ERR_NO_MEM,     200, "No memory") \
    X(RES_ERR_OPT_MEM,    201, "Memory operation failed") \
    X(RES_ERR_LOGIN_FAIL, 401, "Login to system failed")

    /// Define function return values stop
    ///////////////////////////////////////////////////

    // Generate enum type
    typedef enum {
#define X(name, value, msg) name = value,
        ERROR_CODES
#undef X
    } ErrorCode;

// Generate error message table
typedef struct {
    int code;
    const char* message;
} ErrorEntry;

const ErrorEntry error_table[] = {
#define X(name, value, msg) {value, msg},
    ERROR_CODES
#undef X
    {-1, NULL} // End marker
};

const char* get_error_message(int code);
