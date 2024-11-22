#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"

// Function to duplicate a string
char* string_duplicate(const char* src) {
    if (src == NULL) {
        return NULL;
    }
    int len_str = strlen(src) + 1;
    char* copy = (char *)malloc(len_str); 
    if (copy != NULL) {
        strcpy(copy, src);
    }
    return copy;
}


void remove_decimal(char* str) {
    char* dot = strchr(str, '.');
    if (dot != NULL) {
        *dot = '\0';
    }
}

char *add_decimal(const char *str) {
    if (!str) {
        return NULL;
    }
    if (strchr(str, '.') != NULL) {
        return string_duplicate(str);  
    }
    size_t new_len = strlen(str) + 3; 
    char *new_str = (char *)malloc(new_len);
    if (!new_str) {
        return NULL;
    }

    snprintf(new_str, new_len, "%s.0", str); 
    return new_str;
}


char* construct_variable_name(const char* str1, const char* str2) {
    int len = strlen(str1) + strlen(str2) + (2*sizeof(char)); 
    char* result = (char*)malloc(len);
    if (result == NULL) {
        printf("Memory allocation failed\n");
        return NULL;
    }
    snprintf(result, len, "%s_%s", str1, str2);
    return result;
}

char* construct_builtin_name(const char* str1, const char* str2) {
    int len = strlen(str1) + strlen(str2) + (2*sizeof(char)); 
    char* result = (char*)malloc(len);
    if (result == NULL) {
        printf("Memory allocation failed\n");
        return NULL;
    }
    snprintf(result, len, "%s.%s", str1, str2);
    return result;
}