#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

// Function to duplicate a string
char* string_duplicate(const char *str);
void remove_decimal(char* str);
char *add_decimal(const char *str);
char* construct_variable_name(const char* str1, const char* str2);
char* construct_builtin_name(const char* str1, const char* str2);

typedef struct {
    void** pointers; // Массив указателей
    size_t count;    // Текущее количество указателей
    size_t capacity; // Максимальная вместимость массива
} PointerStorage;

extern PointerStorage global_storage;

void init_pointers_storage(size_t initial_capacity);
void add_pointer_to_storage(void* ptr);
void *safe_malloc(size_t size);
void *safe_realloc(void *ptr, size_t new_size);
void safe_free(void *ptr);
void cleanup_pointers_storage(void);
#endif // UTILS_H
