#include <stdio.h>
#include "error_code.h"



const char* get_error_message(int code) {
    for (int i = 0; error_table[i].message != NULL; i++) {
        if (error_table[i].code == code) {
            return error_table[i].message;
        }
    }
    return "Unknown error";
}
