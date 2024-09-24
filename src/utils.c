#include <stdlib.h>
#include <string.h>
#include "utils.h"

// Function to duplicate a string
char* string_duplicate(const char* src) {
    if (src == NULL) {
        return NULL;
    }
    char* copy = malloc(strlen(src) + 1); // +1 для нуль-терминатора
    if (copy != NULL) {
        strcpy(copy, src);
    }
    return copy;
}
