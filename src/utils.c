#include <stdlib.h>
#include <string.h>
#include "utils.h"

// Function to duplicate a string
char* string_duplicate(const char* src) {
    if (src == NULL) {
        return NULL;
    }
    int len_str = strlen(src) + 1;
    char* copy = (char *)malloc(len_str); // +1 для нуль-терминатора
    if (copy != NULL) {
        strcpy(copy, src);
    }
    return copy;
}
